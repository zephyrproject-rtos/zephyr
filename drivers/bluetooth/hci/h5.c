/* uart_h5.c - UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>

#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#include "../util.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define DT_DRV_COMPAT zephyr_bt_hci_3wire_uart

#define HCI_3WIRE_ACK_PKT	0x00
#define HCI_COMMAND_PKT		0x01
#define HCI_ACLDATA_PKT		0x02
#define HCI_SCODATA_PKT		0x03
#define HCI_EVENT_PKT		0x04
#define HCI_ISODATA_PKT		0x05
#define HCI_3WIRE_LINK_PKT	0x0f
#define HCI_VENDOR_PKT		0xff

static bool reliable_packet(uint8_t type)
{
	switch (type) {
	case HCI_COMMAND_PKT:
	case HCI_ACLDATA_PKT:
	case HCI_EVENT_PKT:
	case HCI_ISODATA_PKT:
		return true;
	default:
		return false;
	}
}

/* FIXME: Correct timeout */
#define H5_RX_ACK_TIMEOUT	K_MSEC(250)
#define H5_TX_ACK_TIMEOUT	K_MSEC(250)

#define SLIP_DELIMITER	0xc0
#define SLIP_ESC	0xdb
#define SLIP_ESC_DELIM	0xdc
#define SLIP_ESC_ESC	0xdd

#define H5_RX_ESC	1
#define H5_TX_ACK_PEND	2

#define H5_HDR_SEQ(hdr)		((hdr)[0] & 0x07)
#define H5_HDR_ACK(hdr)		(((hdr)[0] >> 3) & 0x07)
#define H5_HDR_CRC(hdr)		(((hdr)[0] >> 6) & 0x01)
#define H5_HDR_RELIABLE(hdr)	(((hdr)[0] >> 7) & 0x01)
#define H5_HDR_PKT_TYPE(hdr)	((hdr)[1] & 0x0f)
#define H5_HDR_LEN(hdr)		((((hdr)[1] >> 4) & 0x0f) + ((hdr)[2] << 4))

#define H5_SET_SEQ(hdr, seq)	((hdr)[0] |= (seq))
#define H5_SET_ACK(hdr, ack)	((hdr)[0] |= (ack) << 3)
#define H5_SET_RELIABLE(hdr)	((hdr)[0] |= 1 << 7)
#define H5_SET_TYPE(hdr, type)	((hdr)[1] |= type)
#define H5_SET_LEN(hdr, len)	(((hdr)[1] |= ((len) & 0x0f) << 4), \
				 ((hdr)[2] |= (len) >> 4))

struct h5_data {
	/* Needed for delayed work callbacks */
	const struct device     *dev;

	bt_hci_recv_t           recv;

	struct net_buf		*rx_buf;

	struct k_fifo		tx_queue;
	struct k_fifo		rx_queue;
	struct k_fifo		unack_queue;

	struct k_work_delayable ack_work;
	struct k_work_delayable retx_work;

	uint8_t			tx_win;
	uint8_t			tx_ack;
	uint8_t			tx_seq;

	uint8_t			rx_ack;

	enum {
		UNINIT,
		INIT,
		ACTIVE,
	}			link_state;

	enum {
		START,
		HEADER,
		PAYLOAD,
		END,
	}			rx_state;

	uint8_t unack_queue_len;
};

struct h5_config {
	const struct device *uart;

	k_thread_stack_t *rx_stack;
	size_t rx_stack_size;
	struct k_thread *rx_thread;

	k_thread_stack_t *tx_stack;
	size_t tx_stack_size;
	struct k_thread *tx_thread;
};

static const uint8_t sync_req[] = { 0x01, 0x7e };
static const uint8_t sync_rsp[] = { 0x02, 0x7d };
/* Third byte may change */
static uint8_t conf_req[3] = { 0x03, 0xfc };
static const uint8_t conf_rsp[] = { 0x04, 0x7b };

/* H5 signal buffers pool */
#define MAX_SIG_LEN	3
#define SIGNAL_COUNT	(2 * DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT))
#define SIG_BUF_SIZE (BT_BUF_RESERVE + MAX_SIG_LEN)
NET_BUF_POOL_DEFINE(h5_pool, SIGNAL_COUNT, SIG_BUF_SIZE, 0, NULL);

static void h5_reset_rx(struct h5_data *h5)
{
	if (h5->rx_buf) {
		net_buf_unref(h5->rx_buf);
		h5->rx_buf = NULL;
	}

	h5->rx_state = START;
}

static int h5_unslip_byte(const struct device *uart, uint8_t *byte)
{
	int count;

	if (*byte != SLIP_ESC) {
		return 0;
	}

	do {
		count = uart_fifo_read(uart, byte, sizeof(*byte));
	} while (!count);

	switch (*byte) {
	case SLIP_ESC_DELIM:
		*byte = SLIP_DELIMITER;
		break;
	case SLIP_ESC_ESC:
		*byte = SLIP_ESC;
		break;
	default:
		LOG_ERR("Invalid escape byte %x\n", *byte);
		return -EIO;
	}

	return 0;
}

static void process_unack(struct h5_data *h5)
{
	uint8_t next_seq = h5->tx_seq;
	uint8_t number_removed = h5->unack_queue_len;

	if (!h5->unack_queue_len) {
		return;
	}

	LOG_DBG("rx_ack %u tx_ack %u tx_seq %u unack_queue_len %u", h5->rx_ack, h5->tx_ack,
		h5->tx_seq, h5->unack_queue_len);

	while (h5->unack_queue_len > 0) {
		if (next_seq == h5->rx_ack) {
			/* Next sequence number is the same as last received
			 * ack number
			 */
			break;
		}

		number_removed--;
		/* Similar to (n - 1) % 8 with unsigned conversion */
		next_seq = (next_seq - 1) & 0x07;
	}

	if (next_seq != h5->rx_ack) {
		LOG_ERR("Wrong sequence: rx_ack %u tx_seq %u next_seq %u", h5->rx_ack,
			h5->tx_seq, next_seq);
	}

	LOG_DBG("Need to remove %u packet from the queue", number_removed);

	while (number_removed) {
		struct net_buf *buf = k_fifo_get(&h5->unack_queue, K_NO_WAIT);

		if (!buf) {
			LOG_ERR("Unack queue is empty");
			break;
		}

		/* TODO: print or do something with packet */
		LOG_DBG("Remove buf from the unack_queue");

		net_buf_unref(buf);
		h5->unack_queue_len--;
		number_removed--;
	}
}

static void h5_print_header(const uint8_t *hdr, const char *str)
{
	if (H5_HDR_RELIABLE(hdr)) {
		LOG_DBG("%s REL: seq %u ack %u crc %u type %u len %u", str, H5_HDR_SEQ(hdr),
			H5_HDR_ACK(hdr), H5_HDR_CRC(hdr), H5_HDR_PKT_TYPE(hdr), H5_HDR_LEN(hdr));
	} else {
		LOG_DBG("%s UNREL: ack %u crc %u type %u len %u", str, H5_HDR_ACK(hdr),
			H5_HDR_CRC(hdr), H5_HDR_PKT_TYPE(hdr), H5_HDR_LEN(hdr));
	}
}

#if defined(CONFIG_BT_HCI_DRIVER_LOG_LEVEL_DBG)
static void hexdump(const char *str, const uint8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printk("%s zero-length signal packet\n", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}
#else
#define hexdump(str, packet, length)
#endif

static uint8_t h5_slip_byte(const struct device *uart, uint8_t byte)
{
	switch (byte) {
	case SLIP_DELIMITER:
		uart_poll_out(uart, SLIP_ESC);
		uart_poll_out(uart, SLIP_ESC_DELIM);
		return 2;
	case SLIP_ESC:
		uart_poll_out(uart, SLIP_ESC);
		uart_poll_out(uart, SLIP_ESC_ESC);
		return 2;
	default:
		uart_poll_out(uart, byte);
		return 1;
	}
}

static void h5_send(const struct device *dev, const uint8_t *payload, uint8_t type, int len)
{
	const struct h5_config *cfg = dev->config;
	struct h5_data *h5 = dev->data;
	uint8_t hdr[4];
	int i;

	hexdump("<= ", payload, len);

	(void)memset(hdr, 0, sizeof(hdr));

	/* Set ACK for outgoing packet and stop delayed work */
	H5_SET_ACK(hdr, h5->tx_ack);
	/* If cancel fails we may ack the same seq number twice, this is OK. */
	(void)k_work_cancel_delayable(&h5->ack_work);

	if (reliable_packet(type)) {
		H5_SET_RELIABLE(hdr);
		H5_SET_SEQ(hdr, h5->tx_seq);
		h5->tx_seq = (h5->tx_seq + 1) % 8;
	}

	H5_SET_TYPE(hdr, type);
	H5_SET_LEN(hdr, len);

	/* Calculate CRC */
	hdr[3] = ~((hdr[0] + hdr[1] + hdr[2]) & 0xff);

	h5_print_header(hdr, "TX: <");

	uart_poll_out(cfg->uart, SLIP_DELIMITER);

	for (i = 0; i < 4; i++) {
		h5_slip_byte(cfg->uart, hdr[i]);
	}

	for (i = 0; i < len; i++) {
		h5_slip_byte(cfg->uart, payload[i]);
	}

	uart_poll_out(cfg->uart, SLIP_DELIMITER);
}

/* Delayed work taking care about retransmitting packets */
static void retx_timeout(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct h5_data *h5 = CONTAINER_OF(delayable, struct h5_data, retx_work);

	LOG_DBG("unack_queue_len %u", h5->unack_queue_len);

	if (h5->unack_queue_len) {
		struct k_fifo tmp_queue;
		struct net_buf *buf;

		k_fifo_init(&tmp_queue);

		/* Queue to temporary queue */
		while ((buf = k_fifo_get(&h5->tx_queue, K_NO_WAIT))) {
			k_fifo_put(&tmp_queue, buf);
		}

		/* Queue unack packets to the beginning of the queue */
		while ((buf = k_fifo_get(&h5->unack_queue, K_NO_WAIT))) {
			/* include also packet type */
			net_buf_push(buf, sizeof(uint8_t));
			k_fifo_put(&h5->tx_queue, buf);
			h5->tx_seq = (h5->tx_seq - 1) & 0x07;
			h5->unack_queue_len--;
		}

		/* Queue saved packets from temp queue */
		while ((buf = k_fifo_get(&tmp_queue, K_NO_WAIT))) {
			k_fifo_put(&h5->tx_queue, buf);
		}
	}
}

static void ack_timeout(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct h5_data *h5 = CONTAINER_OF(delayable, struct h5_data, ack_work);

	LOG_DBG("");

	h5_send(h5->dev, NULL, HCI_3WIRE_ACK_PKT, 0);
}

static void h5_process_complete_packet(const struct device *dev, uint8_t *hdr)
{
	struct h5_data *h5 = dev->data;
	struct net_buf *buf;

	LOG_DBG("");

	/* rx_ack should be in every packet */
	h5->rx_ack = H5_HDR_ACK(hdr);

	if (reliable_packet(H5_HDR_PKT_TYPE(hdr))) {
		/* For reliable packet increment next transmit ack number */
		h5->tx_ack = (h5->tx_ack + 1) % 8;
		/* Submit delayed work to ack the packet */
		k_work_reschedule(&h5->ack_work, H5_RX_ACK_TIMEOUT);
	}

	h5_print_header(hdr, "RX: >");

	process_unack(h5);

	buf = h5->rx_buf;
	h5->rx_buf = NULL;

	switch (H5_HDR_PKT_TYPE(hdr)) {
	case HCI_3WIRE_ACK_PKT:
		net_buf_unref(buf);
		break;
	case HCI_3WIRE_LINK_PKT:
		k_fifo_put(&h5->rx_queue, buf);
		break;
	case HCI_EVENT_PKT:
	case HCI_ACLDATA_PKT:
	case HCI_ISODATA_PKT:
		hexdump("=> ", buf->data, buf->len);
		h5->recv(dev, buf);
		break;
	}
}

static inline struct net_buf *get_evt_buf(uint8_t evt)
{
	return bt_buf_get_evt(evt, false, K_NO_WAIT);
}

static void bt_uart_isr(const struct device *uart, void *user_data)
{
	const struct device *dev = user_data;
	struct h5_data *h5 = dev->data;
	static int remaining;
	uint8_t byte;
	int ret;
	static uint8_t hdr[4];
	size_t buf_tailroom;

	while (uart_irq_update(uart) &&
	       uart_irq_is_pending(uart)) {

		if (!uart_irq_rx_ready(uart)) {
			if (uart_irq_tx_ready(uart)) {
				LOG_DBG("transmit ready");
			} else {
				LOG_DBG("spurious interrupt");
			}
			/* Only the UART RX path is interrupt-enabled */
			break;
		}

		ret = uart_fifo_read(uart, &byte, sizeof(byte));
		if (!ret) {
			continue;
		}

		switch (h5->rx_state) {
		case START:
			if (byte == SLIP_DELIMITER) {
				h5->rx_state = HEADER;
				remaining = sizeof(hdr);
			}
			break;
		case HEADER:
			/* In a case we confuse ending slip delimiter
			 * with starting one.
			 */
			if (byte == SLIP_DELIMITER) {
				remaining = sizeof(hdr);
				continue;
			}

			if (h5_unslip_byte(uart, &byte) < 0) {
				h5_reset_rx(h5);
				continue;
			}

			memcpy(&hdr[sizeof(hdr) - remaining], &byte, 1);
			remaining--;

			if (remaining) {
				break;
			}

			remaining = H5_HDR_LEN(hdr);

			switch (H5_HDR_PKT_TYPE(hdr)) {
			case HCI_EVENT_PKT:
				/* The buffer is allocated only once we know
				 * the exact event type.
				 */
				h5->rx_state = PAYLOAD;
				break;
			case HCI_ACLDATA_PKT:
				h5->rx_buf = bt_buf_get_rx(BT_BUF_ACL_IN,
							   K_NO_WAIT);
				if (!h5->rx_buf) {
					LOG_WRN("No available data buffers");
					h5_reset_rx(h5);
					continue;
				}

				h5->rx_state = PAYLOAD;
				break;
			case HCI_ISODATA_PKT:
				h5->rx_buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
				if (!h5->rx_buf) {
					LOG_WRN("No available data buffers");
					h5_reset_rx(h5);
					continue;
				}

				h5->rx_state = PAYLOAD;
				break;
			case HCI_3WIRE_LINK_PKT:
			case HCI_3WIRE_ACK_PKT:
				h5->rx_buf = net_buf_alloc(&h5_pool, K_NO_WAIT);
				if (!h5->rx_buf) {
					LOG_WRN("No available signal buffers");
					h5_reset_rx(h5);
					continue;
				}

				h5->rx_state = PAYLOAD;
				break;
			default:
				LOG_ERR("Wrong packet type %u", H5_HDR_PKT_TYPE(hdr));
				h5->rx_state = END;
				break;
			}
			if (!remaining) {
				h5->rx_state = END;
			}
			break;
		case PAYLOAD:
			if (h5_unslip_byte(uart, &byte) < 0) {
				h5_reset_rx(h5);
				continue;
			}

			/* Allocate HCI event buffer now that we know the
			 * exact event type.
			 */
			if (!h5->rx_buf) {
				h5->rx_buf = get_evt_buf(byte);
				if (!h5->rx_buf) {
					LOG_WRN("No available event buffers");
					h5_reset_rx(h5);
					continue;
				}
			}

			buf_tailroom = net_buf_tailroom(h5->rx_buf);
			if (buf_tailroom < sizeof(byte)) {
				LOG_ERR("Not enough space in buffer %zu/%zu", sizeof(byte),
					buf_tailroom);
				h5_reset_rx(h5);
				break;
			}

			net_buf_add_mem(h5->rx_buf, &byte, sizeof(byte));
			remaining--;
			if (!remaining) {
				h5->rx_state = END;
			}
			break;
		case END:
			if (byte != SLIP_DELIMITER) {
				LOG_ERR("Missing ending SLIP_DELIMITER");
				h5_reset_rx(h5);
				break;
			}

			LOG_DBG("Received full packet: type %u", H5_HDR_PKT_TYPE(hdr));

			/* Check when full packet is received, it can be done
			 * when parsing packet header but we need to receive
			 * full packet anyway to clear UART.
			 */
			if (H5_HDR_RELIABLE(hdr) &&
			    H5_HDR_SEQ(hdr) != h5->tx_ack) {
				LOG_ERR("Seq expected %u got %u. Drop packet", h5->tx_ack,
					H5_HDR_SEQ(hdr));
				h5_reset_rx(h5);
				break;
			}

			h5_process_complete_packet(dev, hdr);
			h5->rx_state = START;
			break;
		}
	}
}

static uint8_t h5_get_type(struct net_buf *buf)
{
	return net_buf_pull_u8(buf);
}

static int h5_queue(const struct device *dev, struct net_buf *buf)
{
	struct h5_data *h5 = dev->data;
	uint8_t type;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD:
		type = HCI_COMMAND_PKT;
		break;
	case BT_BUF_ACL_OUT:
		type = HCI_ACLDATA_PKT;
		break;
	case BT_BUF_ISO_OUT:
		type = HCI_ISODATA_PKT;
		break;
	default:
		LOG_ERR("Unknown packet type %u", bt_buf_get_type(buf));
		return -1;
	}

	memcpy(net_buf_push(buf, sizeof(type)), &type, sizeof(type));

	k_fifo_put(&h5->tx_queue, buf);

	return 0;
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct h5_data *h5 = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("");

	/* FIXME: make periodic sending */
	h5_send(dev, sync_req, HCI_3WIRE_LINK_PKT, sizeof(sync_req));

	while (true) {
		struct net_buf *buf;
		uint8_t type;

		LOG_DBG("link_state %u", h5->link_state);

		switch (h5->link_state) {
		case UNINIT:
			/* FIXME: send sync */
			k_sleep(K_MSEC(100));
			break;
		case INIT:
			/* FIXME: send conf */
			k_sleep(K_MSEC(100));
			break;
		case ACTIVE:
			buf = k_fifo_get(&h5->tx_queue, K_FOREVER);
			type = h5_get_type(buf);

			h5_send(dev, buf->data, type, buf->len);

			/* buf is dequeued from tx_queue and queued to unack
			 * queue.
			 */
			k_fifo_put(&h5->unack_queue, buf);
			h5->unack_queue_len++;

			k_work_reschedule(&h5->retx_work, H5_TX_ACK_TIMEOUT);

			break;
		}
	}
}

static void h5_set_txwin(struct h5_data *h5, uint8_t *conf)
{
	conf[2] = h5->tx_win & 0x07;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct h5_data *h5 = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("");

	while (true) {
		struct net_buf *buf;

		buf = k_fifo_get(&h5->rx_queue, K_FOREVER);

		hexdump("=> ", buf->data, buf->len);

		if (!memcmp(buf->data, sync_req, sizeof(sync_req))) {
			if (h5->link_state == ACTIVE) {
				/* TODO Reset H5 */
			}

			h5_send(dev, sync_rsp, HCI_3WIRE_LINK_PKT, sizeof(sync_rsp));
		} else if (!memcmp(buf->data, sync_rsp, sizeof(sync_rsp))) {
			if (h5->link_state == ACTIVE) {
				/* TODO Reset H5 */
			}

			h5->link_state = INIT;
			h5_set_txwin(h5, conf_req);
			h5_send(dev, conf_req, HCI_3WIRE_LINK_PKT, sizeof(conf_req));
		} else if (!memcmp(buf->data, conf_req, 2)) {
			/*
			 * The Host sends Config Response messages without a
			 * Configuration Field.
			 */
			h5_send(dev, conf_rsp, HCI_3WIRE_LINK_PKT, sizeof(conf_rsp));

			/* Then send Config Request with Configuration Field */
			h5_set_txwin(h5, conf_req);
			h5_send(dev, conf_req, HCI_3WIRE_LINK_PKT, sizeof(conf_req));
		} else if (!memcmp(buf->data, conf_rsp, 2)) {
			h5->link_state = ACTIVE;
			if (buf->len > 2) {
				/* Configuration field present */
				h5->tx_win = (buf->data[2] & 0x07);
			}

			LOG_DBG("Finished H5 configuration, tx_win %u", h5->tx_win);
		} else {
			LOG_ERR("Not handled yet %x %x", buf->data[0], buf->data[1]);
		}

		net_buf_unref(buf);

		/* Make sure we don't hog the CPU if the rx_queue never
		 * gets empty.
		 */
		k_yield();
	}
}

static void h5_init(const struct device *dev)
{
	const struct h5_config *cfg = dev->config;
	struct h5_data *h5 = dev->data;
	k_tid_t tid;

	LOG_DBG("");

	h5->link_state = UNINIT;
	h5->rx_state = START;
	h5->tx_win = 4U;

	/* TX thread */
	k_fifo_init(&h5->tx_queue);
	tid = k_thread_create(cfg->tx_thread, cfg->tx_stack, cfg->tx_stack_size,
			      tx_thread, (void *)dev, NULL, NULL,
			      K_PRIO_COOP(CONFIG_BT_HCI_TX_PRIO),
			      0, K_NO_WAIT);
	k_thread_name_set(tid, "tx_thread");

	k_fifo_init(&h5->rx_queue);
	tid = k_thread_create(cfg->rx_thread, cfg->rx_stack, cfg->rx_stack_size,
			      rx_thread, (void *)dev, NULL, NULL,
			      K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			      0, K_NO_WAIT);
	k_thread_name_set(tid, "rx_thread");

	/* Unack queue */
	k_fifo_init(&h5->unack_queue);

	/* Init delayed work */
	k_work_init_delayable(&h5->ack_work, ack_timeout);
	k_work_init_delayable(&h5->retx_work, retx_timeout);
}

static int h5_open(const struct device *dev, bt_hci_recv_t recv)
{
	const struct h5_config *cfg = dev->config;
	struct h5_data *h5 = dev->data;

	LOG_DBG("");

	/* This is needed so that we can access the device struct from within the
	 * delayed work callbacks.
	 */
	h5->dev = dev;

	h5->recv = recv;

	uart_irq_rx_disable(cfg->uart);
	uart_irq_tx_disable(cfg->uart);

	bt_uart_drain(cfg->uart);

	uart_irq_callback_user_data_set(cfg->uart, bt_uart_isr, (void *)dev);

	h5_init(dev);

	uart_irq_rx_enable(cfg->uart);

	return 0;
}

static DEVICE_API(bt_hci, h5_driver_api) = {
	.open = h5_open,
	.send = h5_queue,
};

#define BT_UART_DEVICE_INIT(inst) \
	static K_KERNEL_STACK_DEFINE(rx_thread_stack_##inst, CONFIG_BT_DRV_RX_STACK_SIZE); \
	static struct k_thread rx_thread_##inst; \
	static K_KERNEL_STACK_DEFINE(tx_thread_stack_##inst, CONFIG_BT_DRV_TX_STACK_SIZE); \
	static struct k_thread tx_thread_##inst; \
	static const struct h5_config h5_config_##inst = { \
		.uart = DEVICE_DT_GET(DT_INST_PARENT(inst)), \
		.rx_stack = rx_thread_stack_##inst, \
		.rx_stack_size = K_KERNEL_STACK_SIZEOF(rx_thread_stack_##inst), \
		.rx_thread = &rx_thread_##inst, \
		.tx_stack = tx_thread_stack_##inst, \
		.tx_stack_size = K_KERNEL_STACK_SIZEOF(tx_thread_stack_##inst), \
		.tx_thread = &tx_thread_##inst, \
	}; \
	static struct h5_data h5_##inst; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &h5_##inst, &h5_config_##inst, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &h5_driver_api)


DT_INST_FOREACH_STATUS_OKAY(BT_UART_DEVICE_INIT)
