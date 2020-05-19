/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <arch/cpu.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <sys/util.h>

#include <device.h>
#include <init.h>
#include <drivers/uart.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#define LOG_MODULE_NAME hci_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);


#define H4_NONE 0x0
#define H4_CMD 0x01
#define H4_ACL 0x02
#define H4_SCO 0x03
#define H4_EVT 0x04
#define H4_INV 0xff

/* Length of a discard/flush buffer.
 * This is sized to align with a BLE HCI packet:
 * 1 byte H:4 header + 32 bytes ACL/event data
 * Bigger values might overflow the stack since this is declared as a local
 * variable, smaller ones will force the caller to call into discard more
 * often.
 */
#define H4_DISCARD_LEN 33
#define RX_BUF_SIZE CONFIG_BT_HCI_UART_ASYNC_RX_POOL_BUF_SIZE
#define RX_BUF_COUNT CONFIG_BT_HCI_UART_ASYNC_RX_POOL_BUF_COUNT

static struct {
	struct net_buf *buf;

	uint16_t    remaining;
	uint16_t    discard;

	bool     have_hdr;
	uint8_t     hdr_len;
	uint8_t     type;

	union {
		struct bt_hci_cmd_hdr cmd;
		struct bt_hci_acl_hdr acl;
		uint8_t hdr[4];
	};
} rx;

struct queued_rx_evt {
	sys_snode_t node;
	struct uart_event_rx evt;
};

static struct device *hci_uart_dev;
static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

static struct k_spinlock rx_lock;
static struct k_sem rx_sem;
static volatile struct uart_event_rx current_rx_evt;

static uint8_t __aligned(sizeof(uint32_t)) rx_pool_buf[RX_BUF_SIZE *
							RX_BUF_COUNT];
static struct k_mem_slab rx_pool;

static struct k_mem_slab rx_qevt_pool;
static uint8_t __aligned(sizeof(uint32_t)) rx_qevt_pool_buf[
						sizeof(struct queued_rx_evt) *
						RX_BUF_COUNT];
static sys_slist_t rx_evt_queue;

static struct k_sem tx_sem;
static struct net_buf *tx_buf;

static inline int rx_read(struct uart_event_rx *rx_evt,
			  uint8_t *dst, int req_len)
{
	size_t len = MIN(req_len, rx_evt->len - rx_evt->offset);

	if (dst) {
		memcpy(dst, &rx_evt->buf[rx_evt->offset], len);
	}
	/* This is the only place where offset is modified in that event. UART
	 * ISR modifies length field. No protection is needed.
	 */
	rx_evt->offset += len;

	return len;
}

static size_t h4_discard(struct uart_event_rx *rx_evt, size_t len)
{
	uint8_t buf[H4_DISCARD_LEN];

	return rx_read(rx_evt, buf, MIN(len, sizeof(buf)));
}

static void h4_get_type(struct uart_event_rx *rx_evt)
{
	/* Get packet type */
	if (rx_read(rx_evt, &rx.type, 1) != 1) {
		LOG_WRN("Unable to read H:4 packet type");
		rx.type = H4_NONE;
		return;
	}

	switch (rx.type) {
	case H4_CMD:
		rx.remaining = sizeof(rx.cmd);
		rx.hdr_len = rx.remaining;
		break;
	case H4_ACL:
		rx.remaining = sizeof(rx.acl);
		rx.hdr_len = rx.remaining;
		break;
	default:
		LOG_ERR("Unknown H:4 type 0x%02x", rx.type);
		rx.type = H4_NONE;
	}
}

static void get_acl_hdr(struct uart_event_rx *rx_evt)
{
	struct bt_hci_acl_hdr *hdr = &rx.acl;
	int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= rx_read(rx_evt, (uint8_t *)hdr + to_read, rx.remaining);
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->len);
		LOG_DBG("Got ACL header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}
}

static void get_cmd_hdr(struct uart_event_rx *rx_evt)
{
	struct bt_hci_cmd_hdr *hdr = &rx.cmd;
	int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= rx_read(rx_evt, (uint8_t *)hdr + to_read, rx.remaining);
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->param_len);
		LOG_DBG("Got Command header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}
}

static void reset_rx(void)
{
	rx.type = H4_NONE;
	rx.remaining = 0U;
	rx.have_hdr = false;
	rx.hdr_len = 0U;
	net_buf_unref(rx.buf);
}

static inline void read_header(struct uart_event_rx *rx_evt)
{
	LOG_DBG("read header, type: %d", rx.type);
	switch (rx.type) {
	case H4_NONE:
		h4_get_type(rx_evt);
		return;
	case H4_CMD:
		get_cmd_hdr(rx_evt);
		break;
	case H4_ACL:
		get_acl_hdr(rx_evt);
		break;
	default:
		CODE_UNREACHABLE;
		return;
	}

	if (rx.have_hdr) {
		rx.buf = bt_buf_get_tx(BT_BUF_H4, K_FOREVER, &rx.type,
					    sizeof(rx.type));
		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			LOG_ERR("Not enough space in buffer");
			rx.discard = rx.remaining;
			reset_rx();
		} else {
			net_buf_add_mem(rx.buf, rx.hdr, rx.hdr_len);
		}
	}
}

static void read_payload(struct uart_event_rx *rx_evt)
{
	int read = rx_read(rx_evt, net_buf_tail(rx.buf), rx.remaining);

	rx.buf->len += read;
	rx.remaining -= read;
}

static void complete_rx_buf(void)
{
	int err;

	/* Put buffer into TX queue, thread will dequeue */
	err = bt_send(rx.buf);
	if (err < 0) {
		LOG_ERR("Unable to send (err %d)", err);
	}

	reset_rx();
}

static void process_rx(struct uart_event_rx *rx_evt)
{
	LOG_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u",
	       rx.remaining, rx.discard, rx.have_hdr, rx.buf,
	       rx.buf ? rx.buf->len : 0);

	if (rx.discard) {
		LOG_WRN("discard: %d bytes", rx.discard);
		rx.discard -= h4_discard(rx_evt, rx.discard);
		return;
	}

	if (!rx.have_hdr) {
		read_header(rx_evt);
	}

	if (rx.have_hdr) {
		read_payload(rx_evt);
		if (!rx.remaining) {
			complete_rx_buf();
		}
	}
}

static int rx_enable(void)
{
	int err;
	uint8_t *buf;

	err = k_mem_slab_init(&rx_pool, rx_pool_buf, RX_BUF_SIZE, RX_BUF_COUNT);
	__ASSERT_NO_MSG(err == 0);

	err = k_mem_slab_init(&rx_qevt_pool, rx_qevt_pool_buf,
			      sizeof(struct queued_rx_evt), RX_BUF_COUNT);
	__ASSERT_NO_MSG(err == 0);

	sys_slist_init(&rx_evt_queue);

	err = k_mem_slab_alloc(&rx_pool, (void **)&buf, K_NO_WAIT);
	__ASSERT_NO_MSG(err == 0);

	err = uart_rx_enable(hci_uart_dev, buf, RX_BUF_SIZE, 1);
	if (err < 0) {
		return -EIO;
	}

	return 0;
}

static void on_rx_rdy(struct uart_event_rx *evt)
{
	int err;
	sys_snode_t *node;
	struct queued_rx_evt *qevt;

	/* UART reports new data if timeout occured or when rx buffer is full.
	 * Because of that multiple event can have same physical buffer but with
	 * progressing offset. There is no point to enqueue each event. It is
	 * enough to increase number of available bytes in the buffer is buffer
	 * address is the same as in previous event.
	 *
	 * Start by checking if given uart buffer is already in the queue.
	 * If yes, then increase number of available bytes in that buffer.
	 */
	node = sys_slist_peek_head(&rx_evt_queue);
	while (node) {
		qevt = CONTAINER_OF(node, struct queued_rx_evt, node);
		if (evt->buf == qevt->evt.buf) {
			qevt->evt.len = evt->len + evt->offset;
			goto finalize;
		}
		node = sys_slist_peek_next_no_check(node);
	}

	/* If buffer was not found then allocate new queued event and put into
	 * the list.
	 */
	err = k_mem_slab_alloc(&rx_qevt_pool, (void **)&qevt, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Failed to allocate from pool");
		return;
	}

	qevt->evt = *evt;
	/* No lock is needed because there are only 2 contexts which access
	 * that queue: uart isr (here) and rx thread.
	 */
	sys_slist_append(&rx_evt_queue, &qevt->node);

finalize:
	/* Wake up RX thread */
	k_sem_give(&rx_sem);
}

static void uart_async_callback(struct uart_event *evt, void *user_data)
{

	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_sem);
		break;
	case UART_RX_RDY:
		on_rx_rdy(&evt->data.rx);
		break;
	case UART_RX_BUF_REQUEST:
	{
		uint8_t *buf;

		err = k_mem_slab_alloc(&rx_pool, (void **)&buf,
					K_NO_WAIT);
		if (err < 0) {
			LOG_ERR("Failed to allocate new RX buffer");
		} else {
			err = uart_rx_buf_rsp(hci_uart_dev, buf,
				     CONFIG_BT_HCI_UART_ASYNC_RX_POOL_BUF_SIZE);
			__ASSERT_NO_MSG(err == 0);
		}
		break;
	}
	case UART_RX_BUF_RELEASED:
	{
		/* buffers are released by the RX thread. On special cases when
		 * RX is disabled, pool is reinitialized when enabling RX.
		 */
		break;
	}
	case UART_RX_STOPPED:
		LOG_ERR("RX error occured, reason: %d",
			evt->data.rx_stop.reason);
		break;
	case UART_RX_DISABLED:
		LOG_WRN("Unexpected disable (rx error?). Reenabling");
		err = rx_enable();
		__ASSERT(!err, "Failed to enable RX (err: %d)", err);
		break;
	default:
		LOG_ERR("Unexpected UART event: %d", evt->type);
		break;
	}
}

/* Called when whole data from RX buffer has been drained. Rx buffer can be
 * freed together with rx event.
 */
static void on_curr_rx_buf_complete(void)
{
	sys_snode_t *node;
	struct queued_rx_evt *qevt;
	k_spinlock_key_t key;

	/* List is filled in UART ISR and needs protection. */
	key = k_spin_lock(&rx_lock);
	node = sys_slist_get(&rx_evt_queue);
	k_spin_unlock(&rx_lock, key);

	qevt = CONTAINER_OF(node, struct queued_rx_evt, node);

	/* Free rx buffer */
	k_mem_slab_free(&rx_pool, (void **)&qevt->evt.buf);
	/* Free queued rx event */
	k_mem_slab_free(&rx_qevt_pool, (void **)&qevt);
}

static struct uart_event_rx *get_next_raw_rx_evt(void)
{
	sys_snode_t *node = sys_slist_peek_head(&rx_evt_queue);

	if (node == NULL) {
		return NULL;
	}

	return &CONTAINER_OF(node, struct queued_rx_evt, node)->evt;
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	volatile struct uart_event_rx *curr_rx = NULL;
	bool idle;

	while (1) {
		idle = false;

		while (curr_rx && (curr_rx->offset < curr_rx->len)) {
			process_rx((struct uart_event_rx *)curr_rx);
		}

		if (curr_rx && (curr_rx->offset ==
		    CONFIG_BT_HCI_UART_ASYNC_RX_POOL_BUF_SIZE)) {
			on_curr_rx_buf_complete();
			curr_rx = NULL;
		}

		if (curr_rx == NULL) {
			/* get next */
			curr_rx = (volatile struct uart_event_rx *)
							get_next_raw_rx_evt();
		}

		k_spinlock_key_t key = k_spin_lock(&rx_lock);

		if (curr_rx && (curr_rx->offset < curr_rx->len)) {
			/* new data */
			idle = false;
		} else {
			k_sem_init(&rx_sem, 0, 1);
			idle = true;
		}

		k_spin_unlock(&rx_lock, key);

		if (idle) {
			k_sem_take(&rx_sem, K_FOREVER);
		}
	}
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	uint32_t len = 0U, pos = 0U;


	if (file) {
		while (file[len] != '\0') {
			if (file[len] == '/') {
				pos = len + 1;
			}
			len++;
		}

		file += pos;
		len -= pos;
	}

	uart_poll_out(hci_uart_dev, H4_EVT);
	/* Vendor-Specific debug event */
	uart_poll_out(hci_uart_dev, 0xff);
	/* 0xAA + strlen + \0 + 32-bit line number */
	uart_poll_out(hci_uart_dev, 1 + len + 1 + 4);
	uart_poll_out(hci_uart_dev, 0xAA);

	if (len) {
		while (*file != '\0') {
			uart_poll_out(hci_uart_dev, *file);
			file++;
		}
		uart_poll_out(hci_uart_dev, 0x00);
	}

	uart_poll_out(hci_uart_dev, line >> 0 & 0xff);
	uart_poll_out(hci_uart_dev, line >> 8 & 0xff);
	uart_poll_out(hci_uart_dev, line >> 16 & 0xff);
	uart_poll_out(hci_uart_dev, line >> 24 & 0xff);

	/* Disable interrupts, this is unrecoverable */
	(void)k_spin_lock(&rx_lock);
	while (1) {
	}
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

static int hci_uart_init(struct device *unused)
{
	LOG_DBG("");

	/* Derived from DT's bt-c2h-uart chosen node */
	hci_uart_dev = device_get_binding(CONFIG_BT_CTLR_TO_HOST_UART_DEV_NAME);
	if (!hci_uart_dev) {
		return -EINVAL;
	}

	uart_callback_set(hci_uart_dev, uart_async_callback, NULL);

	return 0;
}

DEVICE_INIT(hci_uart, "hci_uart", &hci_uart_init, NULL, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

void main(void)
{
	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);
	int err;

	LOG_DBG("Start");
	__ASSERT(hci_uart_dev, "UART device is NULL");

	err = rx_enable();
	__ASSERT_NO_MSG(err >= 0);

	k_sem_init(&tx_sem, 0, 1);

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);

	if (IS_ENABLED(CONFIG_BT_WAIT_NOP)) {
		/* Issue a Command Complete with NOP */
		const struct {
			const uint8_t h4;
			const struct bt_hci_evt_hdr hdr;
			const struct bt_hci_evt_cmd_complete cc;
		} __packed cc_evt = {
			.h4 = H4_EVT,
			.hdr = {
				.evt = BT_HCI_EVT_CMD_COMPLETE,
				.len = sizeof(struct bt_hci_evt_cmd_complete),
			},
			.cc = {
				.ncmd = 1,
				.opcode = sys_cpu_to_le16(BT_OP_NOP),
			},
		};

		err = uart_tx(hci_uart_dev, (const uint8_t *)&cc_evt,
				sizeof(cc_evt), 1000);
		if (err < 0) {
			LOG_ERR("Failed to send (err: %d)", err);
		} else {
			k_sem_take(&tx_sem, K_FOREVER);
		}
	}

	/* Spawn the TX thread and start feeding commands and data to the
	 * controller
	 */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	while (1) {
		tx_buf = net_buf_get(&rx_queue, K_FOREVER);
		LOG_DBG("buf %p type %u len %u",
				tx_buf, bt_buf_get_type(tx_buf), tx_buf->len);
		err = uart_tx(hci_uart_dev, tx_buf->data, tx_buf->len,
					1000);
		if (err < 0) {
			LOG_ERR("Failed to send (err: %d)", err);
		} else {
			k_sem_take(&tx_sem, K_FOREVER);
		}
		net_buf_unref(tx_buf);
	}
}
