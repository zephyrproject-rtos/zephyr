/* h4.c - H:4 UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#include "common/bt_str.h"

#include "../util.h"

#define H4_NONE 0x00
#define H4_CMD  0x01
#define H4_ACL  0x02
#define H4_SCO  0x03
#define H4_EVT  0x04
#define H4_ISO  0x05

static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread rx_thread_data;

static struct {
	struct net_buf *buf;
	struct k_fifo   fifo;

	uint16_t    remaining;
	uint16_t    discard;

	bool     have_hdr;
	bool     discardable;

	uint8_t     hdr_len;

	uint8_t     type;
	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
		struct bt_hci_iso_hdr iso;
		uint8_t hdr[4];
	};
} rx = {
	.fifo = Z_FIFO_INITIALIZER(rx.fifo),
};

static struct {
	uint8_t type;
	struct net_buf *buf;
	struct k_fifo   fifo;
} tx = {
	.fifo = Z_FIFO_INITIALIZER(tx.fifo),
};

static const struct device *const h4_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_uart));

static inline void h4_get_type(void)
{
	/* Get packet type */
	if (uart_fifo_read(h4_dev, &rx.type, 1) != 1) {
		LOG_WRN("Unable to read H:4 packet type");
		rx.type = H4_NONE;
		return;
	}

	switch (rx.type) {
	case H4_EVT:
		rx.remaining = sizeof(rx.evt);
		rx.hdr_len = rx.remaining;
		break;
	case H4_ACL:
		rx.remaining = sizeof(rx.acl);
		rx.hdr_len = rx.remaining;
		break;
	case H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			rx.remaining = sizeof(rx.iso);
			rx.hdr_len = rx.remaining;
			break;
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown H:4 type 0x%02x", rx.type);
		rx.type = H4_NONE;
	}
}

static void h4_read_hdr(void)
{
	int bytes_read = rx.hdr_len - rx.remaining;
	int ret;

	ret = uart_fifo_read(h4_dev, rx.hdr + bytes_read, rx.remaining);
	if (unlikely(ret < 0)) {
		LOG_ERR("Unable to read from UART (ret %d)", ret);
	} else {
		rx.remaining -= ret;
	}
}

static inline void get_acl_hdr(void)
{
	h4_read_hdr();

	if (!rx.remaining) {
		struct bt_hci_acl_hdr *hdr = &rx.acl;

		rx.remaining = sys_le16_to_cpu(hdr->len);
		LOG_DBG("Got ACL header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}
}

static inline void get_iso_hdr(void)
{
	h4_read_hdr();

	if (!rx.remaining) {
		struct bt_hci_iso_hdr *hdr = &rx.iso;

		rx.remaining = bt_iso_hdr_len(sys_le16_to_cpu(hdr->len));
		LOG_DBG("Got ISO header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}
}

static inline void get_evt_hdr(void)
{
	struct bt_hci_evt_hdr *hdr = &rx.evt;

	h4_read_hdr();

	if (rx.hdr_len == sizeof(*hdr) && rx.remaining < sizeof(*hdr)) {
		switch (rx.evt.evt) {
		case BT_HCI_EVT_LE_META_EVENT:
			rx.remaining++;
			rx.hdr_len++;
			break;
#if defined(CONFIG_BT_BREDR)
		case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
		case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
			rx.discardable = true;
			break;
#endif
		}
	}

	if (!rx.remaining) {
		if (rx.evt.evt == BT_HCI_EVT_LE_META_EVENT &&
		    (rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
			LOG_DBG("Marking adv report as discardable");
			rx.discardable = true;
		}

		rx.remaining = hdr->len - (rx.hdr_len - sizeof(*hdr));
		LOG_DBG("Got event header. Payload %u bytes", hdr->len);
		rx.have_hdr = true;
	}
}


static inline void copy_hdr(struct net_buf *buf)
{
	net_buf_add_mem(buf, rx.hdr, rx.hdr_len);
}

static void reset_rx(void)
{
	rx.type = H4_NONE;
	rx.remaining = 0U;
	rx.have_hdr = false;
	rx.hdr_len = 0U;
	rx.discardable = false;
}

static struct net_buf *get_rx(k_timeout_t timeout)
{
	LOG_DBG("type 0x%02x, evt 0x%02x", rx.type, rx.evt.evt);

	switch (rx.type) {
	case H4_EVT:
		return bt_buf_get_evt(rx.evt.evt, rx.discardable, timeout);
	case H4_ACL:
		return bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
	case H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
		}
	}

	return NULL;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	struct net_buf *buf;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("started");

	while (1) {
		LOG_DBG("rx.buf %p", rx.buf);

		/* We can only do the allocation if we know the initial
		 * header, since Command Complete/Status events must use the
		 * original command buffer (if available).
		 */
		if (rx.have_hdr && !rx.buf) {
			rx.buf = get_rx(K_FOREVER);
			LOG_DBG("Got rx.buf %p", rx.buf);
			if (rx.remaining > net_buf_tailroom(rx.buf)) {
				LOG_ERR("Not enough space in buffer");
				rx.discard = rx.remaining;
				reset_rx();
			} else {
				copy_hdr(rx.buf);
			}
		}

		/* Let the ISR continue receiving new packets */
		uart_irq_rx_enable(h4_dev);

		buf = net_buf_get(&rx.fifo, K_FOREVER);
		do {
			uart_irq_rx_enable(h4_dev);

			LOG_DBG("Calling bt_recv(%p)", buf);
			bt_recv(buf);

			/* Give other threads a chance to run if the ISR
			 * is receiving data so fast that rx.fifo never
			 * or very rarely goes empty.
			 */
			k_yield();

			uart_irq_rx_disable(h4_dev);
			buf = net_buf_get(&rx.fifo, K_NO_WAIT);
		} while (buf);
	}
}

static size_t h4_discard(const struct device *uart, size_t len)
{
	uint8_t buf[33];
	int err;

	err = uart_fifo_read(uart, buf, MIN(len, sizeof(buf)));
	if (unlikely(err < 0)) {
		LOG_ERR("Unable to read from UART (err %d)", err);
		return 0;
	}

	return err;
}

static inline void read_payload(void)
{
	struct net_buf *buf;
	uint8_t evt_flags;
	int read;

	if (!rx.buf) {
		size_t buf_tailroom;

		rx.buf = get_rx(K_NO_WAIT);
		if (!rx.buf) {
			if (rx.discardable) {
				LOG_WRN("Discarding event 0x%02x", rx.evt.evt);
				rx.discard = rx.remaining;
				reset_rx();
				return;
			}

			LOG_WRN("Failed to allocate, deferring to rx_thread");
			uart_irq_rx_disable(h4_dev);
			return;
		}

		LOG_DBG("Allocated rx.buf %p", rx.buf);

		buf_tailroom = net_buf_tailroom(rx.buf);
		if (buf_tailroom < rx.remaining) {
			LOG_ERR("Not enough space in buffer %u/%zu", rx.remaining, buf_tailroom);
			rx.discard = rx.remaining;
			reset_rx();
			return;
		}

		copy_hdr(rx.buf);
	}

	read = uart_fifo_read(h4_dev, net_buf_tail(rx.buf), rx.remaining);
	if (unlikely(read < 0)) {
		LOG_ERR("Failed to read UART (err %d)", read);
		return;
	}

	net_buf_add(rx.buf, read);
	rx.remaining -= read;

	LOG_DBG("got %d bytes, remaining %u", read, rx.remaining);
	LOG_DBG("Payload (len %u): %s", rx.buf->len, bt_hex(rx.buf->data, rx.buf->len));

	if (rx.remaining) {
		return;
	}

	buf = rx.buf;
	rx.buf = NULL;

	if (rx.type == H4_EVT) {
		evt_flags = bt_hci_evt_get_flags(rx.evt.evt);
		bt_buf_set_type(buf, BT_BUF_EVT);
	} else {
		evt_flags = BT_HCI_EVT_FLAG_RECV;
		bt_buf_set_type(buf, BT_BUF_ACL_IN);
	}

	reset_rx();

	if (IS_ENABLED(CONFIG_BT_RECV_BLOCKING) &&
	    (evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO)) {
		LOG_DBG("Calling bt_recv_prio(%p)", buf);
		bt_recv_prio(buf);
	}

	if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
		LOG_DBG("Putting buf %p to rx fifo", buf);
		net_buf_put(&rx.fifo, buf);
	}
}

static inline void read_header(void)
{
	switch (rx.type) {
	case H4_NONE:
		h4_get_type();
		return;
	case H4_EVT:
		get_evt_hdr();
		break;
	case H4_ACL:
		get_acl_hdr();
		break;
	case H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			get_iso_hdr();
			break;
		}
		__fallthrough;
	default:
		CODE_UNREACHABLE;
		return;
	}

	if (rx.have_hdr && rx.buf) {
		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			LOG_ERR("Not enough space in buffer");
			rx.discard = rx.remaining;
			reset_rx();
		} else {
			copy_hdr(rx.buf);
		}
	}
}

static inline void process_tx(void)
{
	int bytes;

	if (!tx.buf) {
		tx.buf = net_buf_get(&tx.fifo, K_NO_WAIT);
		if (!tx.buf) {
			LOG_ERR("TX interrupt but no pending buffer!");
			uart_irq_tx_disable(h4_dev);
			return;
		}
	}

	if (!tx.type) {
		switch (bt_buf_get_type(tx.buf)) {
		case BT_BUF_ACL_OUT:
			tx.type = H4_ACL;
			break;
		case BT_BUF_CMD:
			tx.type = H4_CMD;
			break;
		case BT_BUF_ISO_OUT:
			if (IS_ENABLED(CONFIG_BT_ISO)) {
				tx.type = H4_ISO;
				break;
			}
			__fallthrough;
		default:
			LOG_ERR("Unknown buffer type");
			goto done;
		}

		bytes = uart_fifo_fill(h4_dev, &tx.type, 1);
		if (bytes != 1) {
			LOG_WRN("Unable to send H:4 type");
			tx.type = H4_NONE;
			return;
		}
	}

	bytes = uart_fifo_fill(h4_dev, tx.buf->data, tx.buf->len);
	if (unlikely(bytes < 0)) {
		LOG_ERR("Unable to write to UART (err %d)", bytes);
	} else {
		net_buf_pull(tx.buf, bytes);
	}

	if (tx.buf->len) {
		return;
	}

done:
	tx.type = H4_NONE;
	net_buf_unref(tx.buf);
	tx.buf = net_buf_get(&tx.fifo, K_NO_WAIT);
	if (!tx.buf) {
		uart_irq_tx_disable(h4_dev);
	}
}

static inline void process_rx(void)
{
	LOG_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u", rx.remaining, rx.discard,
		rx.have_hdr, rx.buf, rx.buf ? rx.buf->len : 0);

	if (rx.discard) {
		rx.discard -= h4_discard(h4_dev, rx.discard);
		return;
	}

	if (rx.have_hdr) {
		read_payload();
	} else {
		read_header();
	}
}

static void bt_uart_isr(const struct device *unused, void *user_data)
{
	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	while (uart_irq_update(h4_dev) && uart_irq_is_pending(h4_dev)) {
		if (uart_irq_tx_ready(h4_dev)) {
			process_tx();
		}

		if (uart_irq_rx_ready(h4_dev)) {
			process_rx();
		}
	}
}

static int h4_send(struct net_buf *buf)
{
	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	net_buf_put(&tx.fifo, buf);
	uart_irq_tx_enable(h4_dev);

	return 0;
}

/** Setup the HCI transport, which usually means to reset the Bluetooth IC
  *
  * @param dev The device structure for the bus connecting to the IC
  *
  * @return 0 on success, negative error value on failure
  */
int __weak bt_hci_transport_setup(const struct device *dev)
{
	h4_discard(h4_dev, 32);
	return 0;
}

static int h4_open(void)
{
	int ret;
	k_tid_t tid;

	LOG_DBG("");

	uart_irq_rx_disable(h4_dev);
	uart_irq_tx_disable(h4_dev);

	ret = bt_hci_transport_setup(h4_dev);
	if (ret < 0) {
		return -EIO;
	}

	uart_irq_callback_set(h4_dev, bt_uart_isr);

	tid = k_thread_create(&rx_thread_data, rx_thread_stack,
			      K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			      rx_thread, NULL, NULL, NULL,
			      K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			      0, K_NO_WAIT);
	k_thread_name_set(tid, "bt_rx_thread");

	return 0;
}

#if defined(CONFIG_BT_HCI_SETUP)
static int h4_setup(void)
{
	/* Extern bt_h4_vnd_setup function.
	 * This function executes vendor-specific commands sequence to
	 * initialize BT Controller before BT Host executes Reset sequence.
	 * bt_h4_vnd_setup function must be implemented in vendor-specific HCI
	 * extansion module if CONFIG_BT_HCI_SETUP is enabled.
	 */
	extern int bt_h4_vnd_setup(const struct device *dev);

	return bt_h4_vnd_setup(h4_dev);
}
#endif

static const struct bt_hci_driver drv = {
	.name		= "H:4",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= h4_open,
	.send		= h4_send,
#if defined(CONFIG_BT_HCI_SETUP)
	.setup		= h4_setup
#endif
};

static int bt_uart_init(void)
{

	if (!device_is_ready(h4_dev)) {
		return -ENODEV;
	}

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(bt_uart_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
