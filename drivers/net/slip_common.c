/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * SLIP framing on top of a UART device, shared by the SLIP network drivers.
 * This is meant for network connectivity between host and qemu. The host will
 * need to run tunslip process.
 */

#define LOG_MODULE_NAME slip
#define LOG_LEVEL CONFIG_SLIP_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>

#include <zephyr/kernel.h>

#include <errno.h>
#include <stddef.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_core.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>

#include "slip.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

enum slip_state {
	STATE_GARBAGE,
	STATE_OK,
	STATE_ESC,
};

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
#define SLIP_FRAG_LEN(slip) CONFIG_NET_BUF_DATA_SIZE
#else
#define SLIP_FRAG_LEN(slip) net_if_get_mtu((slip)->iface)
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

static inline void slip_writeb(const struct device *uart, unsigned char c)
{
	uart_poll_out(uart, c);
}

/**
 *  @brief Write byte to SLIP, escape if it is END or ESC character
 *
 *  @param c  a byte to write
 */
static void slip_writeb_esc(const struct device *uart, unsigned char c)
{
	switch (c) {
	case SLIP_END:
		/* If it's the same code as an END character,
		 * we send a special two character code so as
		 * not to make the receiver think we sent
		 * an END.
		 */
		slip_writeb(uart, SLIP_ESC);
		slip_writeb(uart, SLIP_ESC_END);
		break;
	case SLIP_ESC:
		/* If it's the same code as an ESC character,
		 * we send a special two character code so as
		 * not to make the receiver think we sent
		 * an ESC.
		 */
		slip_writeb(uart, SLIP_ESC);
		slip_writeb(uart, SLIP_ESC_ESC);
		break;
	default:
		slip_writeb(uart, c);
	}
}

int slip_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct slip_config *cfg = dev->config;
	struct net_buf *buf;
	uint8_t *ptr;
	uint16_t i;
	uint8_t c;

	if (!pkt->buffer) {
		/* No data? */
		return -ENODATA;
	}

	slip_writeb(cfg->uart, SLIP_END);

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		ptr = buf->data;
		for (i = 0U; i < buf->len; ++i) {
			c = *ptr++;
			slip_writeb_esc(cfg->uart, c);
		}

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			LOG_DBG("sent data %d bytes", buf->len);

			if (buf->len) {
				LOG_HEXDUMP_DBG(buf->data,
						buf->len, "<slip ");
			}
		}
	}

	slip_writeb(cfg->uart, SLIP_END);

	return 0;
}

static struct net_pkt *slip_poll_handler(struct slip_context *slip)
{
	if (slip->last && slip->last->len) {
		return slip->rx;
	}

	return NULL;
}

static void process_msg(struct slip_context *slip)
{
	struct net_pkt *pkt;

	pkt = slip_poll_handler(slip);
	if (!pkt || !pkt->buffer) {
		return;
	}

	if (net_recv_data(slip->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	slip->rx = NULL;
	slip->last = NULL;
}

static inline int slip_input_byte(struct slip_context *slip,
				  unsigned char c)
{
	switch (slip->state) {
	case STATE_GARBAGE:
		if (c == SLIP_END) {
			slip->state = STATE_OK;
		}

		return 0;
	case STATE_ESC:
		if (c == SLIP_ESC_END) {
			c = SLIP_END;
		} else if (c == SLIP_ESC_ESC) {
			c = SLIP_ESC;
		} else {
			slip->state = STATE_GARBAGE;
			SLIP_STATS(slip->garbage++);
			return 0;
		}

		slip->state = STATE_OK;

		break;
	case STATE_OK:
		if (c == SLIP_ESC) {
			slip->state = STATE_ESC;
			return 0;
		}

		if (c == SLIP_END) {
			slip->state = STATE_OK;
			slip->first = false;

			if (slip->rx) {
				return 1;
			}

			return 0;
		}

		if (slip->first && !slip->rx) {
			/* Must have missed buffer allocation on first byte. */
			return 0;
		}

		if (!slip->first) {
			slip->first = true;

			slip->rx = net_pkt_rx_alloc_on_iface(slip->iface,
							     K_NO_WAIT);
			if (!slip->rx) {
				LOG_ERR("[%p] cannot allocate pkt", slip);
				return 0;
			}

			slip->last = net_pkt_get_frag(slip->rx, SLIP_FRAG_LEN(slip),
						      K_NO_WAIT);
			if (!slip->last) {
				LOG_ERR("[%p] cannot allocate 1st data buffer",
					slip);
				net_pkt_unref(slip->rx);
				slip->rx = NULL;
				return 0;
			}

			net_pkt_append_buffer(slip->rx, slip->last);
			slip->ptr = net_pkt_ip_data(slip->rx);
		}

		break;
	}

	/* It is possible that slip->last is not set during the startup
	 * of the device. If this happens do not continue and overwrite
	 * some random memory.
	 */
	if (!slip->last) {
		return 0;
	}

	if (!net_buf_tailroom(slip->last)) {
		/* We need to allocate a new buffer */
		struct net_buf *buf;

		buf = net_pkt_get_reserve_rx_data(SLIP_FRAG_LEN(slip), K_NO_WAIT);
		if (!buf) {
			LOG_ERR("[%p] cannot allocate next data buf", slip);
			net_pkt_unref(slip->rx);
			slip->rx = NULL;
			slip->last = NULL;

			return 0;
		}

		net_buf_frag_insert(slip->last, buf);
		slip->last = buf;
		slip->ptr = slip->last->data;
	}

	/* The net_buf_add_u8() cannot add data to ll header so we need
	 * a way to do it.
	 */
	if (slip->ptr < slip->last->data) {
		*slip->ptr = c;
	} else {
		slip->ptr = net_buf_add_u8(slip->last, c);
	}

	slip->ptr++;

	return 0;
}

static void slip_uart_isr(const struct device *uart, void *user_data)
{
	const struct device *dev = user_data;
	struct slip_context *slip = dev->data;
	uint8_t c;

	uart_irq_update(uart);

	if (uart_irq_rx_ready(uart) <= 0) {
		return;
	}

	/* The interrupt may be an edge, so read until the FIFO is empty. */
	while (uart_fifo_read(uart, &c, 1) > 0) {
		if (slip_input_byte(slip, c) == 0) {
			continue;
		}

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			struct net_buf *rx_buf = slip->rx->buffer;
			int bytes = net_buf_frags_len(rx_buf);
			int count = 0;

			while (bytes && rx_buf) {
				char msg[6 + 10 + 1];

				snprintk(msg, sizeof(msg), ">slip %2d", count);

				LOG_HEXDUMP_DBG(rx_buf->data, rx_buf->len, msg);

				rx_buf = rx_buf->frags;
				count++;
			}

			LOG_DBG("[%p] received data %d bytes", slip, bytes);
		}

		process_msg(slip);
	}
}

static void slip_uart_setup(const struct device *dev)
{
	const struct slip_config *cfg = dev->config;

	uart_irq_rx_disable(cfg->uart);
	uart_irq_tx_disable(cfg->uart);

	uart_irq_callback_user_data_set(cfg->uart, slip_uart_isr, (void *)dev);
	uart_irq_rx_enable(cfg->uart);
}

void slip_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct slip_context *slip = dev->data;
	uint8_t mac_addr[6];

	slip->iface = iface;
	slip->state = STATE_OK;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x00;
	mac_addr[2] = 0x5E;
	mac_addr[3] = 0x00;
	mac_addr[4] = 0x53;
	mac_addr[5] = sys_rand8_get();

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);

	slip_uart_setup(dev);
}
