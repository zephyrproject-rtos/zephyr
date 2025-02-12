/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * SLIP driver using uart_pipe. This is meant for network connectivity between
 * host and qemu. The host will need to run tunslip process.
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
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/dummy.h>
#include <zephyr/drivers/uart_pipe.h>
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
#define SLIP_FRAG_LEN CONFIG_NET_BUF_DATA_SIZE
#else
#define SLIP_FRAG_LEN _SLIP_MTU
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

static inline void slip_writeb(unsigned char c)
{
	uint8_t buf[1] = { c };

	uart_pipe_send(&buf[0], 1);
}

/**
 *  @brief Write byte to SLIP, escape if it is END or ESC character
 *
 *  @param c  a byte to write
 */
static void slip_writeb_esc(unsigned char c)
{
	switch (c) {
	case SLIP_END:
		/* If it's the same code as an END character,
		 * we send a special two character code so as
		 * not to make the receiver think we sent
		 * an END.
		 */
		slip_writeb(SLIP_ESC);
		slip_writeb(SLIP_ESC_END);
		break;
	case SLIP_ESC:
		/* If it's the same code as an ESC character,
		 * we send a special two character code so as
		 * not to make the receiver think we sent
		 * an ESC.
		 */
		slip_writeb(SLIP_ESC);
		slip_writeb(SLIP_ESC_ESC);
		break;
	default:
		slip_writeb(c);
	}
}

int slip_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_buf *buf;
	uint8_t *ptr;
	uint16_t i;
	uint8_t c;

	ARG_UNUSED(dev);

	if (!pkt->buffer) {
		/* No data? */
		return -ENODATA;
	}

	slip_writeb(SLIP_END);

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		ptr = buf->data;
		for (i = 0U; i < buf->len; ++i) {
			c = *ptr++;
			slip_writeb_esc(c);
		}

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			LOG_DBG("sent data %d bytes", buf->len);

			if (buf->len) {
				LOG_HEXDUMP_DBG(buf->data,
						buf->len, "<slip ");
			}
		}
	}

	slip_writeb(SLIP_END);

	return 0;
}

static struct net_pkt *slip_poll_handler(struct slip_context *slip)
{
	if (slip->last && slip->last->len) {
		return slip->rx;
	}

	return NULL;
}

static inline struct net_if *get_iface(struct slip_context *context,
				       uint16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(context->iface, vlan_tag);
	if (!iface) {
		return context->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return context->iface;
#endif
}

static void process_msg(struct slip_context *slip)
{
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_pkt *pkt;

	pkt = slip_poll_handler(slip);
	if (!pkt || !pkt->buffer) {
		return;
	}

#if defined(CONFIG_NET_VLAN)
	{
		struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

		if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
			struct net_eth_vlan_hdr *hdr_vlan =
				(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

			net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
			vlan_tag = net_pkt_vlan_tag(pkt);
		}
	}
#endif

	if (net_recv_data(get_iface(slip, vlan_tag), pkt) < 0) {
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

			slip->last = net_pkt_get_frag(slip->rx, SLIP_FRAG_LEN,
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

		buf = net_pkt_get_reserve_rx_data(SLIP_FRAG_LEN, K_NO_WAIT);
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

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct slip_context *slip =
		CONTAINER_OF(buf, struct slip_context, buf[0]);
	size_t i;

	if (!slip->init_done) {
		*off = 0;
		return buf;
	}

	for (i = 0; i < *off; i++) {
		if (slip_input_byte(slip, buf[i])) {

			if (LOG_LEVEL >= LOG_LEVEL_DBG) {
				struct net_buf *rx_buf = slip->rx->buffer;
				int bytes = net_buf_frags_len(rx_buf);
				int count = 0;

				while (bytes && rx_buf) {
					char msg[6 + 10 + 1];

					snprintk(msg, sizeof(msg),
						 ">slip %2d", count);

					LOG_HEXDUMP_DBG(rx_buf->data, rx_buf->len,
							msg);

					rx_buf = rx_buf->frags;
					count++;
				}

				LOG_DBG("[%p] received data %d bytes", slip,
					bytes);
			}

			process_msg(slip);
			break;
		}
	}

	*off = 0;

	return buf;
}

int slip_init(const struct device *dev)
{
	struct slip_context *slip = dev->data;

	LOG_DBG("[%p] dev %p", slip, dev);

	slip->state = STATE_OK;
	slip->rx = NULL;
	slip->first = false;

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_IPV4)
	LOG_DBG("ARP enabled");
#endif

	uart_pipe_register(slip->buf, sizeof(slip->buf), recv_cb);

	return 0;
}

static inline struct net_linkaddr *slip_get_mac(struct slip_context *slip)
{
	slip->ll_addr.addr = slip->mac_addr;
	slip->ll_addr.len = sizeof(slip->mac_addr);

	return &slip->ll_addr;
}

void slip_iface_init(struct net_if *iface)
{
	struct slip_context *slip = net_if_get_device(iface)->data;
	struct net_linkaddr *ll_addr;

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_L2_ETHERNET)
	ethernet_init(iface);
#endif

#if defined(CONFIG_NET_LLDP)
	net_lldp_set_lldpdu(iface);
#endif

	if (slip->init_done) {
		return;
	}

	ll_addr = slip_get_mac(slip);

	slip->init_done = true;
	slip->iface = iface;

	if (CONFIG_SLIP_MAC_ADDR[0] != 0) {
		if (net_bytes_from_str(slip->mac_addr, sizeof(slip->mac_addr),
				       CONFIG_SLIP_MAC_ADDR) < 0) {
			goto use_random_mac;
		}
	} else {
use_random_mac:
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		slip->mac_addr[0] = 0x00;
		slip->mac_addr[1] = 0x00;
		slip->mac_addr[2] = 0x5E;
		slip->mac_addr[3] = 0x00;
		slip->mac_addr[4] = 0x53;
		slip->mac_addr[5] = sys_rand8_get();
	}
	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);
}


#if !defined(CONFIG_SLIP_TAP)
static struct slip_context slip_context_data;

static const struct dummy_api slip_if_api = {
	.iface_api.init = slip_iface_init,

	.send = slip_send,
};

#define _SLIP_L2_LAYER DUMMY_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME, slip_init, NULL,
		&slip_context_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&slip_if_api, _SLIP_L2_LAYER, _SLIP_L2_CTX_TYPE, _SLIP_MTU);
#endif
