/* h4.c - H:4 UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr.h>
#include <arch/cpu.h>

#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>

#include "../util.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#if defined(CONFIG_BLUETOOTH_NRF51_PM)
#include "../nrf51_pm.h"
#endif

#define H4_CMD		0x01
#define H4_ACL		0x02
#define H4_SCO		0x03
#define H4_EVT		0x04

static struct device *h4_dev;

static int h4_read(struct device *uart, uint8_t *buf,
		   size_t len, size_t min)
{
	int total = 0;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			BT_DBG("Got zero bytes from UART");
			if (total < min) {
				continue;
			}
			break;
		}

		BT_DBG("read %d remaining %d", rx, len - rx);
		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static size_t h4_discard(struct device *uart, size_t len)
{
	uint8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

static struct net_buf *h4_evt_recv(int *remaining)
{
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	h4_read(h4_dev, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	*remaining = hdr.len;

	buf = bt_buf_get_evt(hdr.evt);
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available event buffers!");
	}

	BT_DBG("len %u", hdr.len);

	return buf;
}

static struct net_buf *h4_acl_recv(int *remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	h4_read(h4_dev, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	buf = bt_buf_get_acl();
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available ACL buffers!");
	}

	*remaining = sys_le16_to_cpu(hdr.len);

	BT_DBG("len %u", *remaining);

	return buf;
}

static void bt_uart_isr(struct device *unused)
{
	static struct net_buf *buf;
	static int remaining;

	ARG_UNUSED(unused);

	while (uart_irq_update(h4_dev) && uart_irq_is_pending(h4_dev)) {
		int read;

		if (!uart_irq_rx_ready(h4_dev)) {
			if (uart_irq_tx_ready(h4_dev)) {
				BT_DBG("transmit ready");
			} else {
				BT_DBG("spurious interrupt");
			}
			/* Only the UART RX path is interrupt-enabled */
			break;
		}

		/* Beginning of a new packet */
		if (!remaining) {
			uint8_t type;

			/* Get packet type */
			read = h4_read(h4_dev, &type, sizeof(type), 0);
			if (read != sizeof(type)) {
				BT_WARN("Unable to read H4 packet type");
				continue;
			}

			switch (type) {
			case H4_EVT:
				buf = h4_evt_recv(&remaining);
				break;
			case H4_ACL:
				buf = h4_acl_recv(&remaining);
				break;
			default:
				BT_ERR("Unknown H4 type %u", type);
				return;
			}

			BT_DBG("need to get %u bytes", remaining);

			if (buf && remaining > net_buf_tailroom(buf)) {
				BT_ERR("Not enough space in buffer");
				net_buf_unref(buf);
				buf = NULL;
			}
		}

		if (!buf) {
			read = h4_discard(h4_dev, remaining);
			BT_WARN("Discarded %d bytes", read);
			remaining -= read;
			continue;
		}

		read = h4_read(h4_dev, net_buf_tail(buf), remaining, 0);

		buf->len += read;
		remaining -= read;

		BT_DBG("received %d bytes", read);

		if (!remaining) {
			BT_DBG("full packet received");

			/* Pass buffer to the stack */
			bt_recv(buf);
			buf = NULL;
		}
	}
}

static int h4_send(struct net_buf *buf)
{
	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		uart_poll_out(h4_dev, H4_ACL);
		break;
	case BT_BUF_CMD:
		uart_poll_out(h4_dev, H4_CMD);
		break;
	default:
		return -EINVAL;
	}

	while (buf->len) {
		uart_poll_out(h4_dev, net_buf_pull_u8(buf));
	}

	net_buf_unref(buf);

	return 0;
}

static int h4_open(void)
{
	BT_DBG("");

	uart_irq_rx_disable(h4_dev);
	uart_irq_tx_disable(h4_dev);

#if defined(CONFIG_BLUETOOTH_NRF51_PM)
	if (nrf51_init(h4_dev) < 0) {
		return -EIO;
	}
#else
	bt_uart_drain(h4_dev);
#endif

	uart_irq_callback_set(h4_dev, bt_uart_isr);

	uart_irq_rx_enable(h4_dev);

	return 0;
}

static struct bt_hci_driver drv = {
	.name		= "H:4",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= h4_open,
	.send		= h4_send,
};

static int _bt_uart_init(struct device *unused)
{
	ARG_UNUSED(unused);

	h4_dev = device_get_binding(CONFIG_BLUETOOTH_UART_ON_DEV_NAME);
	if (!h4_dev) {
		return -EINVAL;
	}

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(_bt_uart_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
