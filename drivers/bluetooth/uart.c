/* uart.c - UART based Bluetooth driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stddef.h>

#include <nanokernel.h>
#include <nanokernel/cpu.h>

#include <board.h>
#include <drivers/uart.h>
#include <misc/byteorder.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define H4_HEADER_SIZE	1

#define H4_CMD		0x01
#define H4_ACL		0x02
#define H4_SCO		0x03
#define H4_EVT		0x04

#define UART CONFIG_BLUETOOTH_UART_INDEX

static int bt_uart_read(int uart, uint8_t *buf, size_t len, size_t min)
{
	int total = 0;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			BT_DBG("Got zero bytes from UART\n");
			if (total < min)
				continue;
			break;
		}

		BT_DBG("read %d remaining %d\n", rx, len - rx);
		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static size_t bt_uart_discard(int uart, size_t len)
{
	uint8_t buf[33];

	if (len > sizeof(buf))
		len = sizeof(buf);

	return uart_fifo_read(uart, buf, len);
}

static struct bt_buf *bt_uart_evt_recv(int *remaining)
{
	struct bt_hci_evt_hdr hdr;
	struct bt_buf *buf;
	int read;

	read = bt_uart_read(UART, (void *)&hdr, sizeof(hdr), sizeof(hdr));
	if (read != sizeof(hdr)) {
		BT_ERR("Cannot read event header\n");
		*remaining = -EIO;
		return NULL;
	}

	*remaining = hdr.len;

	buf = bt_buf_get(BT_EVT, 0);
	if (buf)
		memcpy(bt_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	else
		BT_ERR("No available event buffers!\n");

	BT_DBG("len %u\n", hdr.len);

	return buf;
}

static struct bt_buf *bt_uart_acl_recv(int *remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct bt_buf *buf;
	int read;

	read = bt_uart_read(UART, (void *)&hdr, sizeof(hdr), sizeof(hdr));
	if (read != sizeof(hdr)) {
		BT_ERR("Cannot read ACL header\n");
		*remaining = -EIO;
		return NULL;
	}

	buf = bt_buf_get(BT_ACL_IN, 0);
	if (buf)
		memcpy(bt_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	else
		BT_ERR("No available ACL buffers!\n");

	*remaining = sys_le16_to_cpu(hdr.len);

	BT_DBG("len %u\n", *remaining);

	return buf;
}

void bt_uart_isr(void *unused)
{
	static struct bt_buf *buf;
	static int remaining;

	ARG_UNUSED(unused);

	while (uart_irq_update(UART) && uart_irq_is_pending(UART)) {
		int read;

		if (!uart_irq_rx_ready(UART)) {
			if (uart_irq_tx_ready(UART))
				BT_DBG("transmit ready\n");
			else
				BT_DBG("spurious interrupt\n");
			continue;
		}

		/* Beginning of a new packet */
		if (!remaining) {
			uint8_t type;

			/* Get packet type */
			read = bt_uart_read(UART, &type, sizeof(type), 0);
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

			if (remaining < 0) {
				BT_ERR("Corrupted data received\n");
				remaining = 0;
				return;
			}

			if (remaining > bt_buf_tailroom(buf)) {
				BT_ERR("Not enough space in buffer\n");
				goto failed;
			}

			BT_DBG("need to get %u bytes\n", remaining);
		}

		if (!buf) {
			read = bt_uart_discard(UART, remaining);
			BT_WARN("Discarded %d bytes\n", read);
			remaining -= read;
			continue;
		}

		read = bt_uart_read(UART, bt_buf_tail(buf), remaining, 0);

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

	return;

failed:
	bt_buf_put(buf);
	remaining = 0;
	buf = NULL;
}

static int bt_uart_send(struct bt_buf *buf)
{
	uint8_t *type;

	if (bt_buf_headroom(buf) < H4_HEADER_SIZE) {
		BT_ERR("Not enough headroom in buffer\n");
		return -EINVAL;
	}

	type = bt_buf_push(buf, 1);

	switch (buf->type) {
	case BT_CMD:
		*type = H4_CMD;
		break;
	case BT_ACL_OUT:
		*type = H4_ACL;
		break;
	case BT_EVT:
		*type = H4_EVT;
		break;
	default:
		BT_ERR("Unknown buf type %u\n", buf->type);
		return -EINVAL;
	}

	return uart_fifo_fill(UART, buf->data, buf->len);
}

static void bt_uart_setup(int uart, struct uart_init_info *info)
{
	BT_DBG("\n");

	uart_init(uart, info);

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	uart_int_connect(uart, bt_uart_isr, NULL, NULL);

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart)) {
		unsigned char c;
		uart_fifo_read(uart, &c, 1);
	}

	uart_irq_rx_enable(uart);
}

static int bt_uart_open()
{
	struct uart_init_info info = {
		.options = 0,
		.sys_clk_freq = CONFIG_BLUETOOTH_UART_FREQ,
		.baud_rate = CONFIG_BLUETOOTH_UART_BAUDRATE,
		.regs = CONFIG_BLUETOOTH_UART_REGS,
		.irq = CONFIG_BLUETOOTH_UART_IRQ,
		.int_pri = CONFIG_BLUETOOTH_UART_INT_PRI,
	};

	bt_uart_setup(CONFIG_BLUETOOTH_UART_INDEX, &info);

	return 0;
}

static struct bt_driver drv = {
	.head_reserve	= H4_HEADER_SIZE,
	.open		= bt_uart_open,
	.send		= bt_uart_send,
};

void bt_uart_init(void)
{
	bt_driver_register(&drv);
}
