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
#include <misc/byteorder.h>
#include <logging/sys_log.h>
#include <misc/util.h>

#include <device.h>
#include <init.h>
#include <uart.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#include "common/log.h"

static struct device *hci_uart_dev;
static BT_STACK_NOINIT(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

/* HCI command buffers */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(cmd_tx_pool, CONFIG_BT_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    BT_BUF_USER_DATA_MIN, NULL);

#if defined(CONFIG_BT_CTLR)
#define BT_L2CAP_MTU (CONFIG_BT_CTLR_TX_BUFFER_SIZE - BT_L2CAP_HDR_SIZE)
#else
#define BT_L2CAP_MTU 65 /* 64-byte public key + opcode */
#endif /* CONFIG_BT_CTLR */

/** Data size needed for ACL buffers */
#define BT_BUF_ACL_SIZE BT_L2CAP_BUF_SIZE(BT_L2CAP_MTU)

#if defined(CONFIG_BT_CTLR_TX_BUFFERS)
#define TX_BUF_COUNT CONFIG_BT_CTLR_TX_BUFFERS
#else
#define TX_BUF_COUNT 6
#endif

NET_BUF_POOL_DEFINE(acl_tx_pool, TX_BUF_COUNT, BT_BUF_ACL_SIZE,
		    BT_BUF_USER_DATA_MIN, NULL);

static K_FIFO_DEFINE(tx_queue);

#define H4_CMD 0x01
#define H4_ACL 0x02
#define H4_SCO 0x03
#define H4_EVT 0x04

/* Length of a discard/flush buffer.
 * This is sized to align with a BLE HCI packet:
 * 1 byte H:4 header + 32 bytes ACL/event data
 * Bigger values might overflow the stack since this is declared as a local
 * variable, smaller ones will force the caller to call into discard more
 * often.
 */
#define H4_DISCARD_LEN 33

static int h4_read(struct device *uart, u8_t *buf,
		   size_t len, size_t min)
{
	int total = 0;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			SYS_LOG_DBG("Got zero bytes from UART");
			if (total < min) {
				continue;
			}
			break;
		}

		SYS_LOG_DBG("read %d remaining %d", rx, len - rx);
		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static size_t h4_discard(struct device *uart, size_t len)
{
	u8_t buf[H4_DISCARD_LEN];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

static struct net_buf *h4_cmd_recv(int *remaining)
{
	struct bt_hci_cmd_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	h4_read(hci_uart_dev, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	*remaining = hdr.param_len;

	buf = net_buf_alloc(&cmd_tx_pool, K_NO_WAIT);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_CMD);
		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		SYS_LOG_ERR("No available command buffers!");
	}

	SYS_LOG_DBG("len %u", hdr.param_len);

	return buf;
}

static struct net_buf *h4_acl_recv(int *remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	h4_read(hci_uart_dev, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	buf = net_buf_alloc(&acl_tx_pool, K_NO_WAIT);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_ACL_OUT);
		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		SYS_LOG_ERR("No available ACL buffers!");
	}

	*remaining = sys_le16_to_cpu(hdr.len);

	SYS_LOG_DBG("len %u", *remaining);

	return buf;
}

static void bt_uart_isr(struct device *unused)
{
	static struct net_buf *buf;
	static int remaining;

	ARG_UNUSED(unused);

	while (uart_irq_update(hci_uart_dev) &&
	       uart_irq_is_pending(hci_uart_dev)) {
		int read;

		if (!uart_irq_rx_ready(hci_uart_dev)) {
			if (uart_irq_tx_ready(hci_uart_dev)) {
				SYS_LOG_DBG("transmit ready");
			} else {
				SYS_LOG_DBG("spurious interrupt");
			}
			/* Only the UART RX path is interrupt-enabled */
			break;
		}

		/* Beginning of a new packet */
		if (!remaining) {
			u8_t type;

			/* Get packet type */
			read = h4_read(hci_uart_dev, &type, sizeof(type), 0);
			if (read != sizeof(type)) {
				SYS_LOG_WRN("Unable to read H4 packet type");
				continue;
			}

			switch (type) {
			case H4_CMD:
				buf = h4_cmd_recv(&remaining);
				break;
			case H4_ACL:
				buf = h4_acl_recv(&remaining);
				break;
			default:
				SYS_LOG_ERR("Unknown H4 type %u", type);
				return;
			}

			SYS_LOG_DBG("need to get %u bytes", remaining);

			if (buf && remaining > net_buf_tailroom(buf)) {
				SYS_LOG_ERR("Not enough space in buffer");
				net_buf_unref(buf);
				buf = NULL;
			}
		}

		if (!buf) {
			read = h4_discard(hci_uart_dev, remaining);
			SYS_LOG_WRN("Discarded %d bytes", read);
			remaining -= read;
			continue;
		}

		read = h4_read(hci_uart_dev, net_buf_tail(buf), remaining, 0);

		buf->len += read;
		remaining -= read;

		SYS_LOG_DBG("received %d bytes", read);

		if (!remaining) {
			SYS_LOG_DBG("full packet received");

			/* Put buffer into TX queue, thread will dequeue */
			net_buf_put(&tx_queue, buf);
			buf = NULL;
		}
	}
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct net_buf *buf;
		int err;

		/* Wait until a buffer is available */
		buf = net_buf_get(&tx_queue, K_FOREVER);
		/* Pass buffer to the stack */
		err = bt_send(buf);
		if (err) {
			SYS_LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}

		/* Give other threads a chance to run if tx_queue keeps getting
		 * new data all the time.
		 */
		k_yield();
	}
}

static int h4_send(struct net_buf *buf)
{
	SYS_LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
		    buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		uart_poll_out(hci_uart_dev, H4_ACL);
		break;
	case BT_BUF_EVT:
		uart_poll_out(hci_uart_dev, H4_EVT);
		break;
	default:
		SYS_LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}

	while (buf->len) {
		uart_poll_out(hci_uart_dev, net_buf_pull_u8(buf));
	}

	net_buf_unref(buf);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, u32_t line)
{
	u32_t len = 0, pos = 0;

	/* Disable interrupts, this is unrecoverable */
	(void)irq_lock();

	uart_irq_rx_disable(hci_uart_dev);
	uart_irq_tx_disable(hci_uart_dev);

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

	while (1) {
	}
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

static int hci_uart_init(struct device *unused)
{
	SYS_LOG_DBG("");

	hci_uart_dev = device_get_binding(CONFIG_BT_CTLR_TO_HOST_UART_DEV_NAME);
	if (!hci_uart_dev) {
		return -EINVAL;
	}

	uart_irq_rx_disable(hci_uart_dev);
	uart_irq_tx_disable(hci_uart_dev);

	uart_irq_callback_set(hci_uart_dev, bt_uart_isr);

	uart_irq_rx_enable(hci_uart_dev);

	return 0;
}

DEVICE_INIT(hci_uart, "hci_uart", &hci_uart_init, NULL, NULL,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

void main(void)
{
	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);
	int err;

	SYS_LOG_DBG("Start");

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);
	/* Spawn the TX thread and start feeding commands and data to the
	 * controller
	 */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	while (1) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);
		err = h4_send(buf);
		if (err) {
			SYS_LOG_ERR("Failed to send");
		}
	}
}
