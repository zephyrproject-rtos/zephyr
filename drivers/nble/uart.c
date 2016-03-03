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

#if !defined(CONFIG_BLUETOOTH_DEBUG_DRIVER)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

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

static BT_STACK_NOINIT(rx_fiber_stack, CONFIG_BLUETOOTH_RX_STACK_SIZE);

static struct device *nble_dev;

static struct nano_fifo rx_queue;

static void rx_fiber(void)
{
	BT_DBG("Started");

	while (true) {
		struct net_buf *buf;

		buf = nano_fifo_get(&rx_queue, TICKS_UNLIMITED);
		BT_DBG("Got buf %p", buf);

		rpc_deserialize(buf);

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
		uart_poll_out(nble_dev, net_buf_pull_u8(buf));
	}

	net_buf_unref(buf);
}

static size_t nble_discard(struct device *uart, size_t len)
{
	/* FIXME: correct size for nble */
	uint8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

static void bt_uart_isr(struct device *unused)
{
	static struct net_buf *buf;

	ARG_UNUSED(unused);

	while (uart_irq_update(nble_dev) && uart_irq_is_pending(nble_dev)) {
		static struct ipc_uart_header hdr;
		static uint8_t hdr_bytes;
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

		if (hdr_bytes < sizeof(hdr)) {
			/* Get packet type */
			hdr_bytes += uart_fifo_read(nble_dev,
						    (uint8_t *)&hdr + hdr_bytes,
						    sizeof(hdr) - hdr_bytes);
			if (hdr_bytes < sizeof(hdr)) {
				continue;
			}

			if (hdr.len > NBLE_BUF_SIZE) {
				BT_ERR("Too much data to fit buffer");
				buf = NULL;
			} else {
				buf = net_buf_get(&rx, 0);
				if (!buf) {
					BT_ERR("No available IPC buffers");
				}
			}
		}

		if (!buf) {
			hdr.len -= nble_discard(nble_dev, hdr.len);
			if (!hdr.len) {
				hdr_bytes = 0;
			}
			continue;
		}

		read = uart_fifo_read(nble_dev, net_buf_tail(buf), hdr.len);

		buf->len += read;
		hdr.len -= read;

		if (!hdr.len) {
			BT_DBG("full packet received");
			hdr_bytes = 0;
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

	/* Drain the fifo */
	while (uart_irq_rx_ready(nble_dev)) {
		unsigned char c;

		uart_fifo_read(nble_dev, &c, 1);
	}

	uart_irq_callback_set(nble_dev, bt_uart_isr);

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
