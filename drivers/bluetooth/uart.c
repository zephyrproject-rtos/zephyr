/* uart.c - UART based Bluetooth driver */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <nanokernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/driver.h>

#if !defined(CONFIG_BLUETOOTH_DEBUG_UART)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define H4_CMD		0x01
#define H4_ACL		0x02
#define H4_SCO		0x03
#define H4_EVT		0x04

static int bt_uart_read(struct device *uart, uint8_t *buf,
			size_t len, size_t min)
{
	int total = 0;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			BT_DBG("Got zero bytes from UART\n");
			if (total < min) {
				continue;
			}
			break;
		}

		BT_DBG("read %d remaining %d\n", rx, len - rx);
		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static size_t bt_uart_discard(struct device *uart, size_t len)
{
	uint8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

static struct net_buf *bt_uart_evt_recv(int *remaining)
{
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	bt_uart_read(BT_UART_DEV, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	*remaining = hdr.len;

	buf = bt_buf_get_evt();
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available event buffers!\n");
	}

	BT_DBG("len %u\n", hdr.len);

	return buf;
}

static struct net_buf *bt_uart_acl_recv(int *remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	/* We can ignore the return value since we pass len == min */
	bt_uart_read(BT_UART_DEV, (void *)&hdr, sizeof(hdr), sizeof(hdr));

	buf = bt_buf_get_acl();
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available ACL buffers!\n");
	}

	*remaining = sys_le16_to_cpu(hdr.len);

	BT_DBG("len %u\n", *remaining);

	return buf;
}

void bt_uart_isr(void *unused)
{
	static struct net_buf *buf;
	static int remaining;

	ARG_UNUSED(unused);

	while (uart_irq_update(BT_UART_DEV) &&
	       uart_irq_is_pending(BT_UART_DEV)) {
		int read;

		if (!uart_irq_rx_ready(BT_UART_DEV)) {
			if (uart_irq_tx_ready(BT_UART_DEV)) {
				BT_DBG("transmit ready\n");
			} else {
				BT_DBG("spurious interrupt\n");
			}
			continue;
		}

		/* Beginning of a new packet */
		if (!remaining) {
			uint8_t type;

			/* Get packet type */
			read = bt_uart_read(BT_UART_DEV, &type,
					    sizeof(type), 0);
			if (read != sizeof(type)) {
				BT_WARN("Unable to read H4 packet type\n");
				continue;
			}

			switch (type) {
			case H4_EVT:
				buf = bt_uart_evt_recv(&remaining);
				break;
			case H4_ACL:
				buf = bt_uart_acl_recv(&remaining);
				break;
			default:
				BT_ERR("Unknown H4 type %u\n", type);
				return;
			}

			BT_DBG("need to get %u bytes\n", remaining);

			if (buf && remaining > net_buf_tailroom(buf)) {
				BT_ERR("Not enough space in buffer\n");
				net_buf_unref(buf);
				buf = NULL;
			}
		}

		if (!buf) {
			read = bt_uart_discard(BT_UART_DEV, remaining);
			BT_WARN("Discarded %d bytes\n", read);
			remaining -= read;
			continue;
		}

		read = bt_uart_read(BT_UART_DEV, net_buf_tail(buf),
				    remaining, 0);

		buf->len += read;
		remaining -= read;

		BT_DBG("received %d bytes\n", read);

		if (!remaining) {
			BT_DBG("full packet received\n");

			/* Pass buffer to the stack */
			bt_recv(buf);
			buf = NULL;
		}
	}
}

static int bt_uart_send(enum bt_buf_type buf_type, struct net_buf *buf)
{
	if (buf_type == BT_ACL_OUT) {
		uart_poll_out(BT_UART_DEV, H4_ACL);
	} else if (buf_type == BT_CMD) {
		uart_poll_out(BT_UART_DEV, H4_CMD);
	} else {
		return -EINVAL;
	}

	while (buf->len) {
		uart_poll_out(BT_UART_DEV, buf->data[0]);
		net_buf_pull(buf, 1);
	}

	net_buf_unref(buf);

	return 0;
}

IRQ_CONNECT_STATIC(bluetooth, CONFIG_BLUETOOTH_UART_IRQ,
		   CONFIG_BLUETOOTH_UART_INT_PRI, bt_uart_isr, 0,
		   UART_IRQ_FLAGS);

static void bt_uart_setup(struct device *uart, struct uart_init_info *info)
{
	BT_DBG("\n");

	uart_init(uart, info);

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	IRQ_CONFIG(bluetooth, uart_irq_get(uart), 0);
	irq_enable(uart_irq_get(uart));

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart)) {
		unsigned char c;

		uart_fifo_read(uart, &c, 1);
	}

	uart_irq_rx_enable(uart);
}

static int bt_uart_open(void)
{
	struct uart_init_info info = {
		.options = 0,
		.sys_clk_freq = CONFIG_BLUETOOTH_UART_FREQ,
		.baud_rate = CONFIG_BLUETOOTH_UART_BAUDRATE,
		.irq_pri = CONFIG_BLUETOOTH_UART_INT_PRI,
	};

	bt_uart_setup(BT_UART_DEV, &info);

	return 0;
}

static struct bt_driver drv = {
	.open		= bt_uart_open,
	.send		= bt_uart_send,
};

static int _bt_uart_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_driver_register(&drv);

	return DEV_OK;
}

DECLARE_DEVICE_INIT_CONFIG(bt_uart, "", _bt_uart_init, NULL);
SYS_DEFINE_DEVICE(bt_uart, NULL, NANOKERNEL,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
