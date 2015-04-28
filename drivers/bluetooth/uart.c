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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define H4_HEADER_SIZE	1

#define H4_CMD		0x01
#define H4_ACL		0x02
#define H4_SCO		0x03
#define H4_EVT		0x04

#define UART CONFIG_BLUETOOTH_UART_INDEX

static int bt_uart_read(int uart, uint8_t *buf, size_t len)
{
	int total = 0;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			BT_DBG("Got zero bytes from UART\n");
			continue;
		}

		BT_DBG("read %d remaining %d\n", rx, len - rx);
		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static int bt_uart_evt_recv(struct bt_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;
	int read;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));

	read = bt_uart_read(UART, (void *)hdr, sizeof(*hdr));
	if (read != sizeof(*hdr)) {
		BT_ERR("Cannot read event header\n");
		return -EIO;
	}

	BT_DBG("len %u\n", hdr->len);

	return hdr->len;
}

static int bt_uart_acl_recv(struct bt_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t len;
	int read;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));

	read = bt_uart_read(UART, (void *)hdr, sizeof(*hdr));
	if (read != sizeof(*hdr)) {
		BT_ERR("Cannot read ACL header\n");
		return -EIO;
	}

	len = sys_le16_to_cpu(hdr->len);

	BT_DBG("len %u\n", len);

	return len;
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

		/* Character(s) have been received */
		if (!buf) {
			uint8_t type;

			/* Get packet type */
			read = bt_uart_read(UART, &type, sizeof(type));
			if (read != sizeof(type)) {
				BT_ERR("Error reading UART\n");
				return;
			}

			switch (type) {
				case H4_EVT:
					buf = bt_buf_get(BT_EVT, 0);
					if (!buf) {
						BT_ERR("No event buffers!\n");
						return;
					}
					remaining = bt_uart_evt_recv(buf);
					break;
				case H4_ACL:
					buf = bt_buf_get(BT_ACL_IN, 0);
					if (!buf) {
						BT_ERR("No ACL buffers!\n");
						return;
					}
					remaining = bt_uart_acl_recv(buf);
					break;
				default:
					BT_ERR("Unknown H4 type %u\n", type);
					goto failed;
			}

			if (remaining < 0) {
				BT_ERR("Corrupted data received\n");
				goto failed;
			}

			if (remaining > bt_buf_tailroom(buf)) {
				BT_ERR("Not enough space in buffer\n");
				goto failed;
			}

			BT_DBG("need to get %u bytes\n", remaining);
		}

		read = bt_uart_read(UART, bt_buf_tail(buf), remaining);
		if (read < 0) {
			BT_ERR("Error reading UART\n");
			goto failed;
		}

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
