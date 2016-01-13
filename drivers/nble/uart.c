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

#include <board.h>
#include <init.h>
#include <uart.h>
#include <string.h>

#include <net/buf.h>

#include <bluetooth/log.h>

#include "uart.h"

/* TODO: check size */
#define NBLE_IPC_COUNT	1
#define NBLE_BUF_SIZE	100

static struct nano_fifo rx;
static NET_BUF_POOL(rx_pool, NBLE_IPC_COUNT, NBLE_BUF_SIZE, &rx, NULL, 0);


enum {
	STATUS_TX_IDLE = 0,
	STATUS_TX_BUSY,
	STATUS_TX_DONE,
};

enum {
	STATUS_RX_IDLE = 0,
	STATUS_RX_HDR,
	STATUS_RX_DATA
};

/**
 * Describes the uart IPC to handle
 */
struct ipc_uart_info {
	int uart_num;		/* UART device to use */
	uint32_t irq_vector;	/* IRQ number */
	uint32_t irq_mask;	/* IRQ mask */

	/* callback to be called to set wake state when TX is starting
	 * or ending
	 */
	void (*tx_cb)(bool wake_state, void*);
	void *tx_cb_param;	/* tx_cb function parameter */
};

struct ipc_uart {
	uint8_t *tx_data;
	uint8_t *rx_ptr;
	struct ipc_uart_channels channels[IPC_UART_MAX_CHANNEL];
	struct ipc_uart_header tx_hdr;
	struct ipc_uart_header rx_hdr;
	uint16_t send_counter;
	uint16_t rx_size;
	uint8_t tx_state;
	uint8_t rx_state;
	uint8_t uart_enabled;
	/* protect against multiple wakelock and wake assert calls */
	uint8_t tx_wakelock_acquired;
	/* TODO: remove once IRQ will take a parameter */
	struct device *device;
};

static struct ipc_uart ipc;

static struct device *nble_dev;

static void uart_frame_recv(uint16_t len, uint8_t *p_data)
{
	BT_DBG("rcv: len: %d data len %d src %d channel %d",
	       ipc.rx_hdr.len, len, ipc.rx_hdr.src_cpu_id, ipc.rx_hdr.channel);

	if ((ipc.rx_hdr.channel < IPC_UART_MAX_CHANNEL) &&
	    (ipc.channels[ipc.rx_hdr.channel].cb != NULL)) {
		ipc.channels[ipc.rx_hdr.channel].cb(ipc.rx_hdr.channel,
						    IPC_MSG_TYPE_MESSAGE,
						    len,
						    p_data);
	} else {
		BT_ERR("uart_ipc: bad channel %d", ipc.rx_hdr.channel);
	}
}

static int nble_read(struct device *uart, uint8_t *buf,
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
#if 0
			} else {
				memcpy(net_buf_add(buf, sizeof(hdr)), &hdr,
				       sizeof(hdr));
			}
#endif

			BT_DBG("need to get %u bytes", remaining);

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

		BT_DBG("received %d bytes", read);

		if (!remaining) {
			BT_DBG("full packet received");

			/* Pass buffer to the stack */
			uart_frame_recv(buf->len, buf->data);
			net_buf_unref(buf);
			buf = NULL;
		}
	}
}

void *ipc_uart_channel_open(int channel_id,
			    int (*cb)(int, int, int, void *))
{
	struct ipc_uart_channels *chan;

	if (channel_id > (IPC_UART_MAX_CHANNEL - 1))
		return NULL;

	chan = &ipc.channels[channel_id];

	if (chan->state != IPC_CHANNEL_STATE_CLOSED)
		return NULL;

	chan->state = IPC_CHANNEL_STATE_OPEN;
	chan->cb = cb;

	ipc.uart_enabled = 1;

	return chan;
}

void ipc_uart_close_channel(int channel_id)
{
	ipc.channels[channel_id].state = IPC_CHANNEL_STATE_CLOSED;
	ipc.channels[channel_id].cb = NULL;
	ipc.channels[channel_id].index = channel_id;

	ipc.uart_enabled = 0;
}

static void uart_poll_bytes(uint8_t *buf, size_t len)
{
	while (len--) {
		uart_poll_out(nble_dev, *buf++);
	}
}

int ipc_uart_ns16550_send_pdu(struct device *dev, void *handle, int len,
			      void *p_data)
{
	struct ipc_uart_channels *chan = (struct ipc_uart_channels *)handle;
	struct ipc_uart_header hdr;

	if (ipc.tx_state == STATUS_TX_BUSY) {
		return IPC_UART_TX_BUSY;
	}

	/* It is eventually possible to be in DONE state
	 * (sending last bytes of previous message),
	 * so we move immediately to BUSY and configure the next frame
	 */

	/* FIXME: needed? */
	ipc.tx_state = STATUS_TX_BUSY;

	/* Using polling for transmit */

	/* Send header */
	hdr.len = len;
	hdr.channel = chan->index;
	hdr.src_cpu_id = 0;

	uart_poll_bytes((uint8_t *)&hdr, sizeof(hdr));

	/* Send data */
	uart_poll_bytes(p_data, len);

	return IPC_UART_ERROR_OK;
}

void ipc_uart_ns16550_set_tx_cb(struct device *dev, void (*cb)(bool, void*),
				void *param)
{
	struct ipc_uart_info *info = dev->driver_data;

	info->tx_cb = cb;
	info->tx_cb_param = param;
}

static int ipc_uart_ns16550_init(struct device *dev)
{
	struct ipc_uart_info *info = dev->driver_data;
	int i;

	/* Fail init if no info defined */
	if (!info) {
		BT_ERR("No driver data found");
		return -1;
	}

	for (i = 0; i < IPC_UART_MAX_CHANNEL; i++) {
		ipc_uart_close_channel(i);
	}

	/* Set dev used in irq handler */
	ipc.device = dev;

	ipc.uart_enabled = 0;

	/* Initialize the reception pointer */
	ipc.rx_size = sizeof(ipc.rx_hdr);
	ipc.rx_ptr = (uint8_t *)&ipc.rx_hdr;
	ipc.rx_state = STATUS_RX_IDLE;

	return 0;
}

int nble_open(void)
{
	BT_DBG("");

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

struct ipc_uart_info info;

static int _bt_nble_init(struct device *unused)
{
	ARG_UNUSED(unused);

	nble_dev = device_get_binding(CONFIG_NBLE_UART_ON_DEV_NAME);
	if (!nble_dev) {
		return DEV_INVALID_CONF;
	}

	net_buf_pool_init(rx_pool);

	nble_dev->driver_data = &info;
	ipc_uart_ns16550_init(nble_dev);
	/* TODO: Register nble driver */

	return DEV_OK;
}

DEVICE_INIT(bt_nble, "", _bt_nble_init, NULL, NULL, NANOKERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
