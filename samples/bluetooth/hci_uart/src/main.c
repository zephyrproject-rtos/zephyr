/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
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
#include <drivers/bluetooth/h4_uart.h>

LOG_MODULE_REGISTER(hci_uart, LOG_LEVEL_DBG);

/* Length of a discard/flush buffer.
 * This is sized to align with a BLE HCI packet:
 * 1 byte H:4 header + 32 bytes ACL/event data
 * Bigger values might overflow the stack since this is declared as a local
 * variable, smaller ones will force the caller to call into discard more
 * often.
 */
#define H4_DISCARD_LEN 33

struct h4_uart transport;

static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);

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

static struct device *hci_uart_dev;

static void h4_get_type(struct h4_uart *transport)
{
	/* Get packet type */
	if (h4_uart_read(transport, &rx.type, 1) != 1) {
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

static void get_acl_hdr(struct h4_uart *transport)
{
	struct bt_hci_acl_hdr *hdr = &rx.acl;
	int to_read = sizeof(*hdr) - rx.remaining;
	size_t read;

	read = h4_uart_read(transport, (uint8_t *)hdr + to_read, rx.remaining);
	rx.remaining -= read;
	if (!rx.remaining) {
		rx.remaining = sys_le16_to_cpu(hdr->len);
		LOG_DBG("Got ACL header. Payload %u bytes", rx.remaining);
		rx.have_hdr = true;
	}
}

static void get_cmd_hdr(struct h4_uart *transport)
{
	struct bt_hci_cmd_hdr *hdr = &rx.cmd;
	int to_read = sizeof(*hdr) - rx.remaining;
	size_t read;

	read = h4_uart_read(transport, (uint8_t *)hdr + to_read, rx.remaining);
	rx.remaining -= read;
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

static void read_header(struct h4_uart *transport)
{
	LOG_DBG("read header, type: %d", rx.type);
	switch (rx.type) {
	case H4_NONE:
		h4_get_type(transport);
		break;
	case H4_CMD:
		get_cmd_hdr(transport);
		break;
	case H4_ACL:
		get_acl_hdr(transport);
		break;
	default:
		CODE_UNREACHABLE;
		break;
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

static void read_payload(struct h4_uart *transport)
{
	int read = h4_uart_read(transport, net_buf_tail(rx.buf), rx.remaining);

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

static void process_rx(struct h4_uart *transport)
{
	LOG_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u",
		rx.remaining, rx.discard, rx.have_hdr, rx.buf,
		rx.buf ? rx.buf->len : 0);

	if (rx.discard) {
		LOG_WRN("discard: %d bytes", rx.discard);
		size_t read = h4_uart_read(transport, NULL, rx.discard);
		rx.discard -= read;
		return;
	}

	if (!rx.have_hdr) {
		read_header(transport);
	}

	if (rx.have_hdr) {
		read_payload(transport);
		if (!rx.remaining) {
			complete_rx_buf();
		}
	}
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	uint32_t len = 0U, pos = 0U;
	struct k_spinlock lock = {};

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
	(void)k_spin_lock(&lock);
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

	return 0;
}

DEVICE_INIT(hci_uart, "hci_uart", &hci_uart_init, NULL, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

void main(void)
{
	static const struct h4_uart_config config = {
		.rx = {
			.process = process_rx,
			.stack = rx_thread_stack,
			.rx_thread_stack_size =
				K_THREAD_STACK_SIZEOF(rx_thread_stack),
			.thread_prio = K_PRIO_COOP(7)
		},
		.tx = {
			.timeout = 1000,
			.add_type = false
		}
	};
	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);
	int err;

	LOG_DBG("Start");
	__ASSERT(hci_uart_dev, "UART device is NULL");

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);

	err =  h4_uart_init(&transport, hci_uart_dev, &config);
	__ASSERT(err >= 0, "Unexpected error: %d", err);

	while (1) {
		err = h4_uart_write(&transport,
					   net_buf_get(&rx_queue, K_FOREVER));
		__ASSERT(err >= 0, "Unexpected error: %d", err);
	}
}
