/* h4.c - H:4 UART based Bluetooth driver */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/bluetooth/h4_uart.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>
#include <sys/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_driver
#include "common/log.h"

#include "../util.h"

static struct h4_uart transport;

static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);

static struct {
	struct net_buf *buf;

	uint16_t    remaining;
	uint16_t    discard;

	bool     have_hdr;
	bool     discardable;

	uint8_t     hdr_len;

	uint8_t     type;
	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
		uint8_t hdr[4];
	};
} rx;

static void reset_rx(void)
{
	rx.type = H4_NONE;
	rx.remaining = 0U;
	rx.have_hdr = false;
	rx.hdr_len = 0U;
	rx.discardable = false;
}

static struct net_buf *get_rx(void)
{
	struct net_buf *buf;
	k_timeout_t timeout = K_FOREVER;

	BT_DBG("type 0x%02x, evt 0x%02x", rx.type, rx.evt.evt);

	buf = (rx.type == H4_EVT) ?
		bt_buf_get_evt(rx.evt.evt, rx.discardable, timeout) :
		bt_buf_get_rx(BT_BUF_ACL_IN, timeout);

	__ASSERT_NO_MSG(buf);

	if (rx.remaining > net_buf_tailroom(buf)) {
		BT_ERR("Not enough space in buffer");
		rx.discard = rx.remaining;
		net_buf_unref(buf);
		reset_rx();
		buf = NULL;
	}

	return buf;
}

static void push_rx(void)
{
	uint8_t evt_flags;

	bt_buf_set_type(rx.buf, (rx.type == H4_EVT) ?
				BT_BUF_EVT : BT_BUF_ACL_IN);
	evt_flags = (rx.type == H4_EVT) ? bt_hci_evt_get_flags(rx.evt.evt) :
					  BT_HCI_EVT_FLAG_RECV;

	if (evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO) {
		BT_DBG("Calling bt_recv_prio(%p)", rx.buf);
		bt_recv_prio(rx.buf);
	}

	if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
		BT_DBG("Calling bt_recv_prio(%p)", rx.buf);
		bt_recv(rx.buf);
	}

	rx.buf = NULL;
	reset_rx();
}

static void copy_hdr(struct net_buf *buf)
{
	net_buf_add_mem(buf, rx.hdr, rx.hdr_len);
}

static bool read_payload(struct h4_uart *transport)
{
	size_t read;

	if (!rx.buf) {
		rx.buf = get_rx();
		if (!rx.buf) {
			return false;
		}

		copy_hdr(rx.buf);
	}

	read = h4_uart_read(transport, net_buf_tail(rx.buf), rx.remaining);
	net_buf_add(rx.buf, read);
	rx.remaining -= read;

	BT_DBG("got %d bytes, remaining %u", read, rx.remaining);
	BT_DBG("Payload (len %u): %s", rx.buf->len,
	       bt_hex(rx.buf->data, rx.buf->len));

	if (rx.remaining) {
		return (read == 0);
	}

	push_rx();

	return false;
}

static bool h4_get_type(struct h4_uart *transport)
{
	/* Get packet type */
	if (h4_uart_read(transport, &rx.type, 1) != 1) {
		return true;
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
	default:
		BT_ERR("Unknown H:4 type 0x%02x", rx.type);
		rx.type = H4_NONE;
	}

	return false;
}

static bool get_acl_hdr(struct h4_uart *transport)
{
	struct bt_hci_acl_hdr *hdr = &rx.acl;
	int to_read = sizeof(*hdr) - rx.remaining;
	size_t read;

	read = h4_uart_read(transport, (uint8_t *)hdr + to_read, rx.remaining);
	rx.remaining -= read;
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->len);
		BT_DBG("Got ACL header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}

	return (read == 0);
}

static bool get_evt_hdr(struct h4_uart *transport)
{
	struct bt_hci_evt_hdr *hdr = &rx.evt;
	int to_read = rx.hdr_len - rx.remaining;
	size_t read;

	read = h4_uart_read(transport, (uint8_t *)hdr + to_read, rx.remaining);
	rx.remaining -= read;
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
		    rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
			BT_DBG("Marking adv report as discardable");
			rx.discardable = true;
		}

		rx.remaining = hdr->len - (rx.hdr_len - sizeof(*hdr));
		BT_DBG("Got event header. Payload %u bytes", hdr->len);
		rx.have_hdr = true;
	}

	return (read == 0);
}


static bool read_header(struct h4_uart *transport)
{
	bool no_data = false;

	switch (rx.type) {
	case H4_NONE:
		no_data = h4_get_type(transport);
		break;
	case H4_EVT:
		no_data = get_evt_hdr(transport);
		break;
	case H4_ACL:
		no_data = get_acl_hdr(transport);
		break;
	default:
		CODE_UNREACHABLE;
		break;
	}

	return no_data;
}

void process(struct h4_uart *transport)
{
	bool no_data;

	/*BT_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u",
	rx.remaining, rx.discard, rx.have_hdr, rx.buf,
	rx.buf ? rx.buf->len : 0);*/

	do {

		if (rx.discard) {
			LOG_WRN("discard: %d bytes", rx.discard);
			size_t read = h4_uart_read(transport, NULL, rx.discard);
			rx.discard -= read;
			no_data = read == 0;
			continue;
		}

		no_data =  (rx.have_hdr) ? read_payload(transport) :
					   read_header(transport);
	} while (!no_data);
}

static int h4_send(struct net_buf *buf)
{
	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	return h4_uart_write(&transport, buf);
}

static int h4_open(void)
{
	struct device *dev;

	BT_DBG("");

	static const struct h4_uart_config config = {
		.rx = {
			.process = process,
			.stack = rx_thread_stack,
			.rx_thread_stack_size =
				K_THREAD_STACK_SIZEOF(rx_thread_stack),
			.thread_prio = K_PRIO_COOP(CONFIG_BT_RX_PRIO)
		},
		.tx = {
			.timeout = 1000,
			.add_type = true
		}
	};

	dev = device_get_binding(CONFIG_BT_UART_ON_DEV_NAME);
	if (!dev) {
		return -EINVAL;
	}

	return h4_uart_init(&transport, dev, &config);
}

static const struct bt_hci_driver drv = {
	.name		= "H:4",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= h4_open,
	.send		= h4_send,
};


static int bt_uart_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(bt_uart_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
