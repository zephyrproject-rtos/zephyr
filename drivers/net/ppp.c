/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * PPP driver using uart_pipe. This is meant for network connectivity between
 * two network end points.
 */

#define LOG_LEVEL CONFIG_NET_PPP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(net_ppp, LOG_LEVEL);

#include <stdio.h>

#include <kernel.h>

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <net/ppp.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <sys/ring_buffer.h>
#include <sys/crc.h>
#include <drivers/uart.h>
#include <drivers/console/uart_mux.h>
#include <random/rand32.h>

#include "../../subsys/net/ip/net_stats.h"
#include "../../subsys/net/ip/net_private.h"

#define UART_BUF_LEN CONFIG_NET_PPP_UART_BUF_LEN

enum ppp_driver_state {
	STATE_HDLC_FRAME_START,
	STATE_HDLC_FRAME_ADDRESS,
	STATE_HDLC_FRAME_DATA,
};

#define PPP_WORKQ_PRIORITY CONFIG_NET_PPP_RX_PRIORITY
#define PPP_WORKQ_STACK_SIZE CONFIG_NET_PPP_RX_STACK_SIZE

K_KERNEL_STACK_DEFINE(ppp_workq, PPP_WORKQ_STACK_SIZE);

struct ppp_driver_context {
	const struct device *dev;
	struct net_if *iface;

	/* This net_pkt contains pkt that is being read */
	struct net_pkt *pkt;

	/* How much free space we have in the net_pkt */
	size_t available;

	/* ppp data is read into this buf */
	uint8_t buf[UART_BUF_LEN];

	/* ppp buf use when sending data */
	uint8_t send_buf[UART_BUF_LEN];

	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;

	/* Flag that tells whether this instance is initialized or not */
	atomic_t modem_init_done;

	/* Incoming data is routed via ring buffer */
	struct ring_buf rx_ringbuf;
	uint8_t rx_buf[CONFIG_NET_PPP_RINGBUF_SIZE];

	/* ISR function callback worker */
	struct k_work cb_work;
	struct k_work_q cb_workq;

#if defined(CONFIG_NET_STATISTICS_PPP)
	struct net_stats_ppp stats;
#endif
	enum ppp_driver_state state;

#if defined(CONFIG_PPP_CLIENT_CLIENTSERVER)
	/* correctly received CLIENT bytes */
	uint8_t client_index;
#endif

	uint8_t init_done : 1;
	uint8_t next_escaped : 1;
};

static struct ppp_driver_context ppp_driver_context_data;

static int ppp_save_byte(struct ppp_driver_context *ppp, uint8_t byte)
{
	int ret;

	if (!ppp->pkt) {
		ppp->pkt = net_pkt_rx_alloc_with_buffer(
			ppp->iface,
			CONFIG_NET_BUF_DATA_SIZE,
			AF_UNSPEC, 0, K_NO_WAIT);
		if (!ppp->pkt) {
			LOG_ERR("[%p] cannot allocate pkt", ppp);
			return -ENOMEM;
		}

		net_pkt_cursor_init(ppp->pkt);

		ppp->available = net_pkt_available_buffer(ppp->pkt);
	}

	/* Extra debugging can be enabled separately if really
	 * needed. Normally it would just print too much data.
	 */
	if (0) {
		LOG_DBG("Saving byte %02x", byte);
	}

	/* This is not very intuitive but we must allocate new buffer
	 * before we write a byte to last available cursor position.
	 */
	if (ppp->available == 1) {
		ret = net_pkt_alloc_buffer(ppp->pkt,
					   CONFIG_NET_BUF_DATA_SIZE,
					   AF_UNSPEC, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("[%p] cannot allocate new data buffer", ppp);
			goto out_of_mem;
		}

		ppp->available = net_pkt_available_buffer(ppp->pkt);
	}

	if (ppp->available) {
		ret = net_pkt_write_u8(ppp->pkt, byte);
		if (ret < 0) {
			LOG_ERR("[%p] Cannot write to pkt %p (%d)",
				ppp, ppp->pkt, ret);
			goto out_of_mem;
		}

		ppp->available--;
	}

	return 0;

out_of_mem:
	net_pkt_unref(ppp->pkt);
	ppp->pkt = NULL;
	return -ENOMEM;
}

static const char *ppp_driver_state_str(enum ppp_driver_state state)
{
#if (CONFIG_NET_PPP_LOG_LEVEL >= LOG_LEVEL_DBG)
	switch (state) {
	case STATE_HDLC_FRAME_START:
		return "START";
	case STATE_HDLC_FRAME_ADDRESS:
		return "ADDRESS";
	case STATE_HDLC_FRAME_DATA:
		return "DATA";
	}
#else
	ARG_UNUSED(state);
#endif

	return "";
}

static void ppp_change_state(struct ppp_driver_context *ctx,
			     enum ppp_driver_state new_state)
{
	NET_ASSERT(ctx);

	if (ctx->state == new_state) {
		return;
	}

	NET_ASSERT(new_state >= STATE_HDLC_FRAME_START &&
		   new_state <= STATE_HDLC_FRAME_DATA);

	NET_DBG("[%p] state %s (%d) => %s (%d)",
		ctx, ppp_driver_state_str(ctx->state), ctx->state,
		ppp_driver_state_str(new_state), new_state);

	ctx->state = new_state;
}

static int ppp_send_flush(struct ppp_driver_context *ppp, int off)
{
	if (!IS_ENABLED(CONFIG_NET_TEST)) {
		uint8_t *buf = ppp->send_buf;

		while (off--) {
			uart_poll_out(ppp->dev, *buf++);
		}
	}

	return 0;
}

static int ppp_send_bytes(struct ppp_driver_context *ppp,
			  const uint8_t *data, int len, int off)
{
	int i;

	for (i = 0; i < len; i++) {
		ppp->send_buf[off++] = data[i];

		if (off >= sizeof(ppp->send_buf)) {
			off = ppp_send_flush(ppp, off);
		}
	}

	return off;
}

#if defined(CONFIG_PPP_CLIENT_CLIENTSERVER)

#define CLIENT "CLIENT"
#define CLIENTSERVER "CLIENTSERVER"

static void ppp_handle_client(struct ppp_driver_context *ppp, uint8_t byte)
{
	static const char *client = CLIENT;
	static const char *clientserver = CLIENTSERVER;
	int offset;

	if (ppp->client_index >= (sizeof(CLIENT) - 1)) {
		ppp->client_index = 0;
	}

	if (byte != client[ppp->client_index]) {
		ppp->client_index = 0;
		if (byte != client[ppp->client_index]) {
			return;
		}
	}

	++ppp->client_index;
	if (ppp->client_index >= (sizeof(CLIENT) - 1)) {
		LOG_DBG("Received complete CLIENT string");
		offset = ppp_send_bytes(ppp, clientserver,
					sizeof(CLIENTSERVER) - 1, 0);
		(void)ppp_send_flush(ppp, offset);
		ppp->client_index = 0;
	}

}
#endif

static int ppp_input_byte(struct ppp_driver_context *ppp, uint8_t byte)
{
	int ret = -EAGAIN;

	switch (ppp->state) {
	case STATE_HDLC_FRAME_START:
		/* Synchronizing the flow with HDLC flag field */
		if (byte == 0x7e) {
			/* Note that we do not save the sync flag */
			LOG_DBG("Sync byte (0x%02x) start", byte);
			ppp_change_state(ppp, STATE_HDLC_FRAME_ADDRESS);
#if defined(CONFIG_PPP_CLIENT_CLIENTSERVER)
		} else {
			ppp_handle_client(ppp, byte);
#endif
		}

		break;

	case STATE_HDLC_FRAME_ADDRESS:
		if (byte != 0xff) {
			/* Check if we need to sync again */
			if (byte == 0x7e) {
				/* Just skip to the start of the pkt byte */
				return -EAGAIN;
			}

			LOG_DBG("Invalid (0x%02x) byte, expecting Address",
				byte);

			/* If address is != 0xff, then ignore this
			 * frame. RFC 1662 ch 3.1
			 */
			ppp_change_state(ppp, STATE_HDLC_FRAME_START);
		} else {
			LOG_DBG("Address byte (0x%02x) start", byte);

			ppp_change_state(ppp, STATE_HDLC_FRAME_DATA);

			/* Save the address field so that we can calculate
			 * the FCS. The address field will not be passed
			 * to upper stack.
			 */
			ret = ppp_save_byte(ppp, byte);
			if (ret < 0) {
				ppp_change_state(ppp, STATE_HDLC_FRAME_START);
			}

			ret = -EAGAIN;
		}

		break;

	case STATE_HDLC_FRAME_DATA:
		/* If the next frame starts, then send this one
		 * up in the network stack.
		 */
		if (byte == 0x7e) {
			LOG_DBG("End of pkt (0x%02x)", byte);
			ppp_change_state(ppp, STATE_HDLC_FRAME_ADDRESS);
			ret = 0;
		} else {
			if (byte == 0x7d) {
				/* RFC 1662, ch. 4.2 */
				ppp->next_escaped = true;
				break;
			}

			if (ppp->next_escaped) {
				/* RFC 1662, ch. 4.2 */
				byte ^= 0x20;
				ppp->next_escaped = false;
			}

			ret = ppp_save_byte(ppp, byte);
			if (ret < 0) {
				ppp_change_state(ppp, STATE_HDLC_FRAME_START);
			}

			ret = -EAGAIN;
		}

		break;

	default:
		LOG_DBG("[%p] Invalid state %d", ppp, ppp->state);
		break;
	}

	return ret;
}

static bool ppp_check_fcs(struct ppp_driver_context *ppp)
{
	struct net_buf *buf;
	uint16_t crc;

	buf = ppp->pkt->buffer;
	if (!buf) {
		return false;
	}

	crc = crc16_ccitt(0xffff, buf->data, buf->len);

	buf = buf->frags;

	while (buf) {
		crc = crc16_ccitt(crc, buf->data, buf->len);
		buf = buf->frags;
	}

	if (crc != 0xf0b8) {
		LOG_DBG("Invalid FCS (0x%x)", crc);
#if defined(CONFIG_NET_STATISTICS_PPP)
		ppp->stats.chkerr++;
#endif
		return false;
	}

	return true;
}

static void ppp_process_msg(struct ppp_driver_context *ppp)
{
	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		net_pkt_hexdump(ppp->pkt, "recv ppp");
	}

	if (IS_ENABLED(CONFIG_NET_PPP_VERIFY_FCS) && !ppp_check_fcs(ppp)) {
#if defined(CONFIG_NET_STATISTICS_PPP)
		ppp->stats.drop++;
		ppp->stats.pkts.rx++;
#endif
		net_pkt_unref(ppp->pkt);
	} else {
		/* Remove the Address (0xff), Control (0x03) and
		 * FCS fields (16-bit) as the PPP L2 layer does not need
		 * those bytes.
		 */
		uint16_t addr_and_ctrl = net_buf_pull_be16(ppp->pkt->buffer);

		/* Currently we do not support compressed Address and Control
		 * fields so they must always be present.
		 */
		if (addr_and_ctrl != (0xff << 8 | 0x03)) {
#if defined(CONFIG_NET_STATISTICS_PPP)
			ppp->stats.drop++;
			ppp->stats.pkts.rx++;
#endif
			net_pkt_unref(ppp->pkt);
		} else {
			/* Skip FCS bytes (2) */
			net_buf_frag_last(ppp->pkt->buffer)->len -= 2;

			/* Make sure we now start reading from PPP header in
			 * PPP L2 recv()
			 */
			net_pkt_cursor_init(ppp->pkt);
			net_pkt_set_overwrite(ppp->pkt, true);

			if (net_recv_data(ppp->iface, ppp->pkt) < 0) {
				net_pkt_unref(ppp->pkt);
			}
		}
	}

	ppp->pkt = NULL;
}

#if defined(CONFIG_NET_TEST)
static uint8_t *ppp_recv_cb(uint8_t *buf, size_t *off)
{
	struct ppp_driver_context *ppp =
		CONTAINER_OF(buf, struct ppp_driver_context, buf);
	size_t i, len = *off;

	for (i = 0; i < *off; i++) {
		if (0) {
			/* Extra debugging can be enabled separately if really
			 * needed. Normally it would just print too much data.
			 */
			LOG_DBG("[%zd] %02x", i, buf[i]);
		}

		if (ppp_input_byte(ppp, buf[i]) == 0) {
			/* Ignore empty or too short frames */
			if (ppp->pkt && net_pkt_get_len(ppp->pkt) > 3) {
				ppp_process_msg(ppp);
				break;
			}
		}
	}

	if (i == *off) {
		*off = 0;
	} else {
		*off = len - i - 1;

		memmove(&buf[0], &buf[i + 1], *off);
	}

	return buf;
}

void ppp_driver_feed_data(uint8_t *data, int data_len)
{
	struct ppp_driver_context *ppp = &ppp_driver_context_data;
	size_t recv_off = 0;

	/* We are expecting that the tests are feeding data in large
	 * chunks so we can reset the uart buffer here.
	 */
	memset(ppp->buf, 0, UART_BUF_LEN);

	ppp_change_state(ppp, STATE_HDLC_FRAME_START);

	while (data_len > 0) {
		int data_to_copy = MIN(data_len, UART_BUF_LEN);
		int remaining;

		LOG_DBG("Feeding %d bytes", data_to_copy);

		memcpy(ppp->buf, data, data_to_copy);

		recv_off = data_to_copy;

		(void)ppp_recv_cb(ppp->buf, &recv_off);

		remaining = data_to_copy - recv_off;

		LOG_DBG("We copied %d bytes", remaining);

		data_len -= remaining;
		data += remaining;
	}
}
#endif

static bool calc_fcs(struct net_pkt *pkt, uint16_t *fcs, uint16_t protocol)
{
	struct net_buf *buf;
	uint16_t crc;
	uint16_t c;

	buf = pkt->buffer;
	if (!buf) {
		return false;
	}

	/* HDLC Address and Control fields */
	c = sys_cpu_to_be16(0xff << 8 | 0x03);

	crc = crc16_ccitt(0xffff, (const uint8_t *)&c, sizeof(c));

	if (protocol > 0) {
		crc = crc16_ccitt(crc, (const uint8_t *)&protocol,
				  sizeof(protocol));
	}

	while (buf) {
		crc = crc16_ccitt(crc, buf->data, buf->len);
		buf = buf->frags;
	}

	crc ^= 0xffff;
	*fcs = crc;

	return true;
}

static uint16_t ppp_escape_byte(uint8_t byte, int *offset)
{
	if (byte == 0x7e || byte == 0x7d || byte < 0x20) {
		*offset = 0;
		return (0x7d << 8) | (byte ^ 0x20);
	}

	*offset = 1;
	return byte;
}

static int ppp_send(const struct device *dev, struct net_pkt *pkt)
{
	struct ppp_driver_context *ppp = dev->data;
	struct net_buf *buf = pkt->buffer;
	uint16_t protocol = 0;
	int send_off = 0;
	uint32_t sync_addr_ctrl;
	uint16_t fcs, escaped;
	uint8_t byte;
	int i, offset;

#if defined(CONFIG_NET_TEST)
	return 0;
#endif

	ARG_UNUSED(dev);

	if (!buf) {
		/* No data? */
		return -ENODATA;
	}

	/* If the packet is a normal network packet, we must add the protocol
	 * value here.
	 */
	if (!net_pkt_is_ppp(pkt)) {
		if (net_pkt_family(pkt) == AF_INET) {
			protocol = htons(PPP_IP);
		} else if (net_pkt_family(pkt) == AF_INET6) {
			protocol = htons(PPP_IPV6);
		} else {
			return -EPROTONOSUPPORT;
		}
	}

	if (!calc_fcs(pkt, &fcs, protocol)) {
		return -ENOMEM;
	}

	/* Sync, Address & Control fields */
	sync_addr_ctrl = sys_cpu_to_be32(0x7e << 24 | 0xff << 16 |
					 0x7d << 8 | 0x23);
	send_off = ppp_send_bytes(ppp, (const uint8_t *)&sync_addr_ctrl,
				  sizeof(sync_addr_ctrl), send_off);

	if (protocol > 0) {
		escaped = htons(ppp_escape_byte(protocol, &offset));
		send_off = ppp_send_bytes(ppp, (uint8_t *)&escaped + offset,
					  offset ? 1 : 2,
					  send_off);

		escaped = htons(ppp_escape_byte(protocol >> 8, &offset));
		send_off = ppp_send_bytes(ppp, (uint8_t *)&escaped + offset,
					  offset ? 1 : 2,
					  send_off);
	}

	/* Note that we do not print the first four bytes and FCS bytes at the
	 * end so that we do not need to allocate separate net_buf just for
	 * that purpose.
	 */
	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		net_pkt_hexdump(pkt, "send ppp");
	}

	while (buf) {
		for (i = 0; i < buf->len; i++) {
			/* Escape illegal bytes */
			escaped = htons(ppp_escape_byte(buf->data[i], &offset));
			send_off = ppp_send_bytes(ppp,
						  (uint8_t *)&escaped + offset,
						  offset ? 1 : 2,
						  send_off);
		}

		buf = buf->frags;
	}

	escaped = htons(ppp_escape_byte(fcs, &offset));
	send_off = ppp_send_bytes(ppp, (uint8_t *)&escaped + offset,
				  offset ? 1 : 2,
				  send_off);

	escaped = htons(ppp_escape_byte(fcs >> 8, &offset));
	send_off = ppp_send_bytes(ppp, (uint8_t *)&escaped + offset,
				  offset ? 1 : 2,
				  send_off);

	byte = 0x7e;
	send_off = ppp_send_bytes(ppp, &byte, 1, send_off);

	(void)ppp_send_flush(ppp, send_off);

	return 0;
}

#if !defined(CONFIG_NET_TEST)
static int ppp_consume_ringbuf(struct ppp_driver_context *ppp)
{
	uint8_t *data;
	size_t len, tmp;
	int ret;

	len = ring_buf_get_claim(&ppp->rx_ringbuf, &data,
				 CONFIG_NET_PPP_RINGBUF_SIZE);
	if (len == 0) {
		LOG_DBG("Ringbuf %p is empty!", &ppp->rx_ringbuf);
		return 0;
	}

	/* This will print too much data, enable only if really needed */
	if (0) {
		LOG_HEXDUMP_DBG(data, len, ppp->dev->name);
	}

	tmp = len;

	do {
		if (ppp_input_byte(ppp, *data++) == 0) {
			/* Ignore empty or too short frames */
			if (ppp->pkt && net_pkt_get_len(ppp->pkt) > 3) {
				ppp_process_msg(ppp);
			}
		}
	} while (--tmp);

	ret = ring_buf_get_finish(&ppp->rx_ringbuf, len);
	if (ret < 0) {
		LOG_DBG("Cannot flush ring buffer (%d)", ret);
	}

	return -EAGAIN;
}

static void ppp_isr_cb_work(struct k_work *work)
{
	struct ppp_driver_context *ppp =
		CONTAINER_OF(work, struct ppp_driver_context, cb_work);
	int ret = -EAGAIN;

	while (ret == -EAGAIN) {
		ret = ppp_consume_ringbuf(ppp);
	}
}
#endif /* !CONFIG_NET_TEST */

static int ppp_driver_init(const struct device *dev)
{
	struct ppp_driver_context *ppp = dev->data;

	LOG_DBG("[%p] dev %p", ppp, dev);

#if !defined(CONFIG_NET_TEST)
	ring_buf_init(&ppp->rx_ringbuf, sizeof(ppp->rx_buf), ppp->rx_buf);
	k_work_init(&ppp->cb_work, ppp_isr_cb_work);

	k_work_q_start(&ppp->cb_workq, ppp_workq,
		       K_KERNEL_STACK_SIZEOF(ppp_workq),
		       K_PRIO_COOP(PPP_WORKQ_PRIORITY));
	k_thread_name_set(&ppp->cb_workq.thread, "ppp_workq");
#endif

	ppp->pkt = NULL;
	ppp_change_state(ppp, STATE_HDLC_FRAME_START);
#if defined(CONFIG_PPP_CLIENT_CLIENTSERVER)
	ppp->client_index = 0;
#endif

	return 0;
}

static inline struct net_linkaddr *ppp_get_mac(struct ppp_driver_context *ppp)
{
	ppp->ll_addr.addr = ppp->mac_addr;
	ppp->ll_addr.len = sizeof(ppp->mac_addr);

	return &ppp->ll_addr;
}

static void ppp_iface_init(struct net_if *iface)
{
	struct ppp_driver_context *ppp = net_if_get_device(iface)->data;
	struct net_linkaddr *ll_addr;

	LOG_DBG("[%p] iface %p", ppp, iface);

	net_ppp_init(iface);

	if (ppp->init_done) {
		return;
	}

	ppp->init_done = true;
	ppp->iface = iface;

	/* The mac address is not really used but network interface expects
	 * to find one.
	 */
	ll_addr = ppp_get_mac(ppp);

	if (CONFIG_PPP_MAC_ADDR[0] != 0) {
		if (net_bytes_from_str(ppp->mac_addr, sizeof(ppp->mac_addr),
				       CONFIG_PPP_MAC_ADDR) < 0) {
			goto use_random_mac;
		}
	} else {
use_random_mac:
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		ppp->mac_addr[0] = 0x00;
		ppp->mac_addr[1] = 0x00;
		ppp->mac_addr[2] = 0x5E;
		ppp->mac_addr[3] = 0x00;
		ppp->mac_addr[4] = 0x53;
		ppp->mac_addr[5] = sys_rand32_get();
	}

	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);

	memset(ppp->buf, 0, sizeof(ppp->buf));

	/* If we have a GSM modem with PPP support, then do not start the
	 * interface automatically but only after the modem is ready.
	 */
	if (IS_ENABLED(CONFIG_MODEM_GSM_PPP)) {
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}
}

#if defined(CONFIG_NET_STATISTICS_PPP)
static struct net_stats_ppp *ppp_get_stats(const struct device *dev)
{
	struct ppp_driver_context *context = dev->data;

	return &context->stats;
}
#endif

#if !defined(CONFIG_NET_TEST)
static void ppp_uart_flush(const struct device *dev)
{
	uint8_t c;

	while (uart_fifo_read(dev, &c, 1) > 0) {
		continue;
	}
}

static void ppp_uart_isr(const struct device *uart, void *user_data)
{
	struct ppp_driver_context *context = user_data;
	int rx = 0, ret;

	/* get all of the data off UART as fast as we can */
	while (uart_irq_update(uart) && uart_irq_rx_ready(uart)) {
		rx = uart_fifo_read(uart, context->buf, sizeof(context->buf));
		if (rx <= 0) {
			continue;
		}

		ret = ring_buf_put(&context->rx_ringbuf, context->buf, rx);
		if (ret < rx) {
			LOG_ERR("Rx buffer doesn't have enough space. "
				"Bytes pending: %d, written: %d",
				rx, ret);
			break;
		}

		k_work_submit_to_queue(&context->cb_workq, &context->cb_work);
	}
}
#endif /* !CONFIG_NET_TEST */

static int ppp_start(const struct device *dev)
{
	struct ppp_driver_context *context = dev->data;

	/* Init the PPP UART only once. This should only be done after
	 * the GSM muxing is setup and enabled. GSM modem will call this
	 * after everything is ready to be connected.
	 */
#if !defined(CONFIG_NET_TEST)
	if (atomic_cas(&context->modem_init_done, false, true)) {
		const char *dev_name = NULL;

		/* Now try to figure out what device to open. If GSM muxing
		 * is enabled, then use it. If not, then check if modem
		 * configuration is enabled, and use that. If none are enabled,
		 * then use our own config.
		 */
#if IS_ENABLED(CONFIG_GSM_MUX)
		const struct device *mux;

		mux = uart_mux_find(CONFIG_GSM_MUX_DLCI_PPP);
		if (mux == NULL) {
			LOG_ERR("Cannot find GSM mux dev for DLCI %d",
				CONFIG_GSM_MUX_DLCI_PPP);
			return -ENOENT;
		}

		dev_name = mux->name;
#elif IS_ENABLED(CONFIG_MODEM_GSM_PPP)
		dev_name = CONFIG_MODEM_GSM_UART_NAME;
#else
		dev_name = CONFIG_NET_PPP_UART_NAME;
#endif
		if (dev_name == NULL || dev_name[0] == '\0') {
			LOG_ERR("UART configuration is wrong!");
			return -EINVAL;
		}

		LOG_DBG("Initializing PPP to use %s", dev_name);

		context->dev = device_get_binding(dev_name);
		if (!context->dev) {
			LOG_ERR("Cannot find dev %s", dev_name);
			return -ENODEV;
		}

		uart_irq_rx_disable(context->dev);
		uart_irq_tx_disable(context->dev);
		ppp_uart_flush(context->dev);
		uart_irq_callback_user_data_set(context->dev, ppp_uart_isr,
						context);
		uart_irq_rx_enable(context->dev);
	}
#endif /* !CONFIG_NET_TEST */

	net_ppp_carrier_on(context->iface);

	return 0;
}

static int ppp_stop(const struct device *dev)
{
	struct ppp_driver_context *context = dev->data;

	net_ppp_carrier_off(context->iface);

	return 0;
}

static const struct ppp_api ppp_if_api = {
	.iface_api.init = ppp_iface_init,

	.send = ppp_send,
	.start = ppp_start,
	.stop = ppp_stop,
#if defined(CONFIG_NET_STATISTICS_PPP)
	.get_stats = ppp_get_stats,
#endif
};

NET_DEVICE_INIT(ppp, CONFIG_NET_PPP_DRV_NAME, ppp_driver_init,
		device_pm_control_nop, &ppp_driver_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ppp_if_api,
		PPP_L2, NET_L2_GET_CTX_TYPE(PPP_L2), PPP_MTU);
