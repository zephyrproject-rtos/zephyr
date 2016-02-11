/* uart.c - Nordic BLE UART based Bluetooth driver */

/*
 * Copyright (c) 2016 Intel Corporation
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

#include <nanokernel.h>
#include <sections.h>

#include <board.h>
#include <init.h>
#include <uart.h>
#include <string.h>

#include <net/buf.h>

#include <bluetooth/log.h>

#include "uart.h"
#include "rpc.h"

/**
 * @note this structure must be self-aligned and self-packed
 */
struct ipc_uart_header {
	uint16_t len;		/**< Length of IPC message. */
	uint8_t channel;	/**< Channel number of IPC message. */
	uint8_t src_cpu_id;	/**< CPU id of IPC sender. */
} __packed;

/* TODO: check size */
#define NBLE_TX_BUF_COUNT	2
#define NBLE_RX_BUF_COUNT	8
#define NBLE_BUF_SIZE		384

static struct nano_fifo rx;
static NET_BUF_POOL(rx_pool, NBLE_RX_BUF_COUNT, NBLE_BUF_SIZE, &rx, NULL, 0);

static struct nano_fifo tx;
static NET_BUF_POOL(tx_pool, NBLE_TX_BUF_COUNT, NBLE_BUF_SIZE, &tx, NULL, 0);

static BT_STACK_NOINIT(rx_fiber_stack, 2048);

static struct device *nble_dev;

static struct nano_fifo rx_queue;

static void rx_fiber(void)
{
	BT_DBG("Started");

	while (true) {
		struct net_buf *buf;

		buf = nano_fifo_get(&rx_queue, TICKS_UNLIMITED);
		BT_DBG("Got buf %p", buf);

		rpc_deserialize(buf->data, buf->len);

		net_buf_unref(buf);
	}
}

struct net_buf *rpc_alloc_cb(uint16_t length)
{
	struct net_buf *buf;

	BT_DBG("length %u", length);

	buf = net_buf_get(&tx, sizeof(struct ipc_uart_header));
	if (!buf) {
		BT_ERR("Unable to get tx buffer");
		return NULL;
	}

	if (length > net_buf_tailroom(buf)) {
		BT_ERR("Too big tx buffer requested");
		net_buf_unref(buf);
		return NULL;
	}

	return buf;
}

void rpc_transmit_cb(struct net_buf *buf)
{
	struct ipc_uart_header *hdr;

	BT_DBG("buf %p length %u", buf, buf->len);

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = buf->len - sizeof(*hdr);
	hdr->channel = 0;
	hdr->src_cpu_id = 0;

	while (buf->len) {
		uart_poll_out(nble_dev, buf->data[0]);
		net_buf_pull(buf, 1);
	}

	net_buf_unref(buf);
}

static int nble_read(struct device *uart, uint8_t *buf,
		     size_t len, size_t min)
{
	int total = 0;
	int tries = 10;

	while (len) {
		int rx;

		rx = uart_fifo_read(uart, buf, len);
		if (rx == 0) {
			BT_DBG("Got zero bytes from UART");
			if (total < min && tries--) {
				continue;
			}
			break;
		}

		len -= rx;
		total += rx;
		buf += rx;
	}

	return total;
}

static size_t nble_discard(struct device *uart, size_t len)
{
	/* FIXME: correct size for nble */
	uint8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

void bt_uart_isr(void *unused)
{
	static struct net_buf *buf;
	static int remaining;

	ARG_UNUSED(unused);

	while (uart_irq_update(nble_dev) && uart_irq_is_pending(nble_dev)) {
		int read;

		if (!uart_irq_rx_ready(nble_dev)) {
			if (uart_irq_tx_ready(nble_dev)) {
				BT_DBG("transmit ready");
				/*
				 * Implementing ISR based transmit requires
				 * extra API for uart such as
				 * uart_line_status(), etc. The support was
				 * removed from the recent code, using polling
				 * for transmit for now.
				 */
			} else {
				BT_DBG("spurious interrupt");
			}
			continue;
		}

		/* Beginning of a new packet */
		if (!remaining) {
			struct ipc_uart_header hdr;

			/* Get packet type */
			read = nble_read(nble_dev, (uint8_t *)&hdr,
					 sizeof(hdr), sizeof(hdr));
			if (read != sizeof(hdr)) {
				BT_WARN("Unable to read NBLE header");
				continue;
			}

			remaining = hdr.len;

			buf = net_buf_get(&rx, 0);
			if (!buf) {
				BT_ERR("No available IPC buffers");
			}

			if (buf && remaining > net_buf_tailroom(buf)) {
				BT_ERR("Not enough space in buffer");
				net_buf_unref(buf);
				buf = NULL;
			}
		}

		if (!buf) {
			read = nble_discard(nble_dev, remaining);
			BT_WARN("Discarded %d bytes", read);
			remaining -= read;
			continue;
		}

		read = nble_read(nble_dev, net_buf_tail(buf), remaining, 0);

		buf->len += read;
		remaining -= read;

		if (!remaining) {
			BT_DBG("full packet received");

			/* Pass buffer to the stack */
			nano_fifo_put(&rx_queue, buf);
		}
	}
}

int nble_open(void)
{
	BT_DBG("");

	/* Initialize receive queue and start rx_fiber */
	nano_fifo_init(&rx_queue);
	fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
		    (nano_fiber_entry_t)rx_fiber, 0, 0, 7, 0);

	uart_irq_rx_disable(nble_dev);
	uart_irq_tx_disable(nble_dev);

	IRQ_CONNECT(CONFIG_NBLE_UART_IRQ, CONFIG_NBLE_UART_IRQ_PRI,
		    bt_uart_isr, 0, UART_IRQ_FLAGS);
	irq_enable(CONFIG_NBLE_UART_IRQ);

	/* Drain the fifo */
	while (uart_irq_rx_ready(nble_dev)) {
		unsigned char c;

		uart_fifo_read(nble_dev, &c, 1);
	}

	uart_irq_rx_enable(nble_dev);

	return 0;
}

static int _bt_nble_init(struct device *unused)
{
	ARG_UNUSED(unused);

	nble_dev = device_get_binding(CONFIG_NBLE_UART_ON_DEV_NAME);
	if (!nble_dev) {
		return DEV_INVALID_CONF;
	}

	net_buf_pool_init(rx_pool);
	net_buf_pool_init(tx_pool);

	return DEV_OK;
}

DEVICE_INIT(bt_nble, "", _bt_nble_init, NULL, NULL, NANOKERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
