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

#define H4_NONE 0x00
#define H4_CMD  0x01
#define H4_ACL  0x02
#define H4_SCO  0x03
#define H4_EVT  0x04

static BT_STACK_NOINIT(rx_thread_stack, CONFIG_BLUETOOTH_RX_STACK_SIZE);

static struct {
	struct net_buf *buf;
	struct k_sem    sem;

	uint16_t remaining;
	uint16_t discard;

	bool     have_hdr;

	uint8_t  type;
	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
	};
} rx = {
	.sem = K_SEM_INITIALIZER(rx.sem, 0, 1),
};

static struct device *h4_dev;

static inline void h4_get_type(void)
{
	/* Get packet type */
	if (uart_fifo_read(h4_dev, &rx.type, 1) != 1) {
		BT_WARN("Unable to read H:4 packet type");
		rx.type = H4_NONE;
		return;
	}

	switch (rx.type) {
	case H4_EVT:
		rx.remaining = sizeof(rx.evt);
		break;
	case H4_ACL:
		rx.remaining = sizeof(rx.acl);
		break;
	default:
		BT_ERR("Unknown H:4 type 0x%02x", rx.type);
		rx.type = H4_NONE;
	}
}

static inline void get_acl_hdr(void)
{
	struct bt_hci_acl_hdr *hdr = &rx.acl;
	int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= uart_fifo_read(h4_dev, (uint8_t *)hdr + to_read,
				       rx.remaining);
	if (!rx.remaining) {
		BT_DBG("Got ACL header");
		rx.remaining = sys_le16_to_cpu(hdr->len);
		rx.have_hdr = true;
	}
}

static inline void get_evt_hdr(void)
{
	struct bt_hci_evt_hdr *hdr = &rx.evt;
	int to_read = sizeof(*hdr) - rx.remaining;

	rx.remaining -= uart_fifo_read(h4_dev, (uint8_t *)hdr + to_read,
				       rx.remaining);
	if (!rx.remaining) {
		BT_DBG("Got event header");
		rx.remaining = hdr->len;
		rx.have_hdr = true;
	}
}


static inline void buf_set_type(struct net_buf *buf)
{
	if (rx.type == H4_EVT) {
		bt_buf_set_type(buf, BT_BUF_EVT);
	} else {
		bt_buf_set_type(buf, BT_BUF_ACL_IN);
	}
}

static inline void copy_hdr(struct net_buf *buf)
{
	if (rx.type == H4_EVT) {
		net_buf_add_mem(buf, &rx.evt, sizeof(rx.evt));
	} else {
		net_buf_add_mem(buf, &rx.acl, sizeof(rx.acl));
	}
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	struct net_buf *buf;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	BT_DBG("started");

	while (1) {
		BT_DBG("waiting for semaphore");
		k_sem_take(&rx.sem, K_FOREVER);
		BT_DBG("got semaphore, rx.buf %p have_hdr %u",
		       rx.buf, rx.have_hdr);

		if (rx.buf) {
			buf = rx.buf;
			buf_set_type(buf);
			rx.buf = NULL;
			rx.have_hdr = false;
			rx.type = H4_NONE;

			uart_irq_rx_enable(h4_dev);

			BT_DBG("Calling bt_recv()");
			bt_recv(buf);
		} else {
			rx.buf = bt_buf_get_rx(K_FOREVER);
			if (rx.remaining > net_buf_tailroom(rx.buf)) {
				BT_ERR("Not enough space in buffer");
				rx.discard = rx.remaining;
				rx.remaining = 0;
				rx.have_hdr = 0;
			} else {
				copy_hdr(rx.buf);
			}
			uart_irq_rx_enable(h4_dev);
		}
	}
}

static size_t h4_discard(struct device *uart, size_t len)
{
	uint8_t buf[33];

	return uart_fifo_read(uart, buf, min(len, sizeof(buf)));
}

/* Returns false if RX IRQ was disabled and control given to the RX thread */
static inline bool read_payload(void)
{
	int read;

	if (!rx.buf) {
		rx.buf = bt_buf_get_rx(K_NO_WAIT);
		if (!rx.buf) {
			uart_irq_rx_disable(h4_dev);
			BT_DBG("giving semaphore");
			k_sem_give(&rx.sem);
			return false;
		}

		copy_hdr(rx.buf);
	}

	read = uart_fifo_read(h4_dev, net_buf_tail(rx.buf), rx.remaining);
	net_buf_add(rx.buf, read);
	rx.remaining -= read;

	BT_DBG("got %d bytes, remaining %u", read, rx.remaining);
	BT_DBG("Payload (len %u): %s", rx.buf->len,
	       bt_hex(rx.buf->data, rx.buf->len));

	if (rx.remaining) {
		return true;
	}

	if (rx.type == H4_EVT && bt_hci_evt_is_prio(rx.evt.evt)) {
		buf_set_type(rx.buf);
		BT_DBG("Calling bt_recv()");
		bt_recv(rx.buf);
		rx.buf = NULL;
		rx.have_hdr = false;
		rx.type = H4_NONE;
		return true;
	} else {
		uart_irq_rx_disable(h4_dev);
		k_sem_give(&rx.sem);
		return false;
	}
}

static inline void read_header(void)
{
	switch (rx.type) {
	case H4_NONE:
		h4_get_type();
		return;
	case H4_EVT:
		get_evt_hdr();
		break;
	case H4_ACL:
		get_acl_hdr();
		break;
	default:
		CODE_UNREACHABLE;
		return;
	}

	if (rx.have_hdr && rx.buf) {
		if (rx.remaining > net_buf_tailroom(rx.buf)) {
			BT_ERR("Not enough space in buffer");
			rx.discard = rx.remaining;
			rx.remaining = 0;
			rx.have_hdr = 0;
		} else {
			copy_hdr(rx.buf);
		}
	}
}

static void bt_uart_isr(struct device *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(h4_dev) && uart_irq_is_pending(h4_dev)) {
		if (!uart_irq_rx_ready(h4_dev)) {
			if (uart_irq_tx_ready(h4_dev)) {
				BT_DBG("transmit ready");
			} else {
				BT_DBG("spurious interrupt");
			}
			/* Only the UART RX path is interrupt-enabled */
			break;
		}

		BT_DBG("remaining %u discard %u have_hdr %u buf %p len %u",
		       rx.remaining, rx.discard, rx.have_hdr, rx.buf,
		       rx.buf->len);

		if (rx.discard) {
			rx.discard -= h4_discard(h4_dev, rx.discard);
			continue;
		}

		if (!rx.have_hdr) {
			read_header();
			continue;
		}

		if (!read_payload()) {
			return;
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
	h4_discard(h4_dev, 32);
#endif

	uart_irq_callback_set(h4_dev, bt_uart_isr);

	uart_irq_rx_enable(h4_dev);

	k_thread_spawn(rx_thread_stack, sizeof(rx_thread_stack), rx_thread,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

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
