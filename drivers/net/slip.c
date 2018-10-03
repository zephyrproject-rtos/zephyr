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

#define SYS_LOG_DOMAIN "slip"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SLIP_LEVEL
#include <logging/sys_log.h>
#include <stdio.h>

#include <kernel.h>

#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <misc/util.h>
#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/lldp.h>
#include <console/uart_pipe.h>
#include <net/ethernet.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

enum slip_state {
	STATE_GARBAGE,
	STATE_OK,
	STATE_ESC,
};

struct slip_context {
	bool init_done;
	bool first;		/* SLIP received it's byte or not after
				 * driver initialization or SLIP_END byte.
				 */
	u8_t buf[1];		/* SLIP data is read into this buf */
	struct net_pkt *rx;	/* and then placed into this net_pkt */
	struct net_buf *last;	/* Pointer to last fragment in the list */
	u8_t *ptr;		/* Where in net_pkt to add data */
	struct net_if *iface;
	u8_t state;

	u8_t mac_addr[6];
	struct net_linkaddr ll_addr;

#if defined(CONFIG_SLIP_STATISTICS)
#define SLIP_STATS(statement)
#else
	u16_t garbage;
#define SLIP_STATS(statement) statement
#endif
};

#if defined(CONFIG_NET_LLDP)
static const struct net_lldpdu lldpdu = {
	.chassis_id = {
		.type_length = htons((LLDP_TLV_CHASSIS_ID << 9) |
			NET_LLDP_CHASSIS_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_CHASSIS_ID_SUBTYPE,
		.value = NET_LLDP_CHASSIS_ID_VALUE
	},
	.port_id = {
		.type_length = htons((LLDP_TLV_PORT_ID << 9) |
			NET_LLDP_PORT_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_PORT_ID_SUBTYPE,
		.value = NET_LLDP_PORT_ID_VALUE
	},
	.ttl = {
		.type_length = htons((LLDP_TLV_TTL << 9) |
			NET_LLDP_TTL_TLV_LEN),
		.ttl = htons(NET_LLDP_TTL)
	},
#if defined(CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED)
	.end_lldpdu_tlv = NET_LLDP_END_LLDPDU_VALUE
#endif /* CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED */
};

#define lldpdu_ptr (&lldpdu)
#else
#define lldpdu_ptr NULL
#endif /* CONFIG_NET_LLDP */

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
#define COLOR_OFF     "\x1B[0m"
#define COLOR_YELLOW  "\x1B[0;33m"
#else
#define COLOR_OFF     ""
#define COLOR_YELLOW  ""
#endif

static void hexdump(const char *str, const u8_t *packet,
		    size_t length, size_t ll_reserve)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printf("%s %08X ", str, n);
		}

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_YELLOW);
		} else {
			printf(COLOR_OFF);
		}
#endif
		printf("%02X ", *packet++);

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (n < ll_reserve) {
			printf(COLOR_OFF);
		}
#endif
		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
	}
}
#else
#define hexdump(slip, str, packet, length, ll_reserve)
#endif

static inline void slip_writeb(unsigned char c)
{
	u8_t buf[1] = { c };

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

static int slip_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_buf *frag;
#if defined(CONFIG_SLIP_TAP)
	u16_t ll_reserve = net_pkt_ll_reserve(pkt);
	bool send_header_once = false;
#endif
	u8_t *ptr;
	u16_t i;
	u8_t c;

	if (!pkt->frags) {
		/* No data? */
		return -ENODATA;
	}

	slip_writeb(SLIP_END);

	for (frag = pkt->frags; frag; frag = frag->frags) {
#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
		int frag_count = 0;
#endif

#if defined(CONFIG_SLIP_TAP)
		ptr = frag->data - ll_reserve;

		/* This writes ethernet header */
		if (!send_header_once && ll_reserve) {
			for (i = 0; i < ll_reserve; i++) {
				slip_writeb_esc(*ptr++);
			}
		}

		if (net_if_get_mtu(iface) > net_buf_headroom(frag)) {
			/* Do not add link layer header if the mtu is bigger
			 * than fragment size. The first packet needs the
			 * link layer header always.
			 */
			send_header_once = true;
			ll_reserve = 0;
			ptr = frag->data;
		}
#else
		/* There is no ll header in tun device */
		ptr = frag->data;
#endif

		for (i = 0; i < frag->len; ++i) {
			c = *ptr++;
			slip_writeb_esc(c);
		}

#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
		SYS_LOG_DBG("sent data %d bytes",
			    frag->len + net_pkt_ll_reserve(pkt));
		if (frag->len + net_pkt_ll_reserve(pkt)) {
			char msg[8 + 1];

			snprintf(msg, sizeof(msg), "<slip %2d", frag_count++);

			hexdump(msg, net_pkt_ll(pkt),
				frag->len + net_pkt_ll_reserve(pkt),
				net_pkt_ll_reserve(pkt));
		}
#endif
	}

	net_pkt_unref(pkt);
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
				       u16_t vlan_tag)
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
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_pkt *pkt;

	pkt = slip_poll_handler(slip);
	if (!pkt || !pkt->frags) {
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

			slip->rx = net_pkt_get_reserve_rx(0, K_NO_WAIT);
			if (!slip->rx) {
				SYS_LOG_ERR("[%p] cannot allocate pkt",
					    slip);
				return 0;
			}

			slip->last = net_pkt_get_frag(slip->rx, K_NO_WAIT);
			if (!slip->last) {
				SYS_LOG_ERR("[%p] cannot allocate 1st data frag",
					    slip);
				net_pkt_unref(slip->rx);
				slip->rx = NULL;
				return 0;
			}

			net_pkt_frag_add(slip->rx, slip->last);
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
		/* We need to allocate a new fragment */
		struct net_buf *frag;

		frag = net_pkt_get_reserve_rx_data(0, K_NO_WAIT);
		if (!frag) {
			SYS_LOG_ERR("[%p] cannot allocate next data frag",
				    slip);
			net_pkt_unref(slip->rx);
			slip->rx = NULL;
			slip->last = NULL;

			return 0;
		}

		net_buf_frag_insert(slip->last, frag);
		slip->last = frag;
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

static u8_t *recv_cb(u8_t *buf, size_t *off)
{
	struct slip_context *slip =
		CONTAINER_OF(buf, struct slip_context, buf);
	size_t i;

	if (!slip->init_done) {
		*off = 0;
		return buf;
	}

	for (i = 0; i < *off; i++) {
		if (slip_input_byte(slip, buf[i])) {
#if SYS_LOG_LEVEL >= SYS_LOG_LEVEL_DEBUG
			struct net_buf *frag = slip->rx->frags;
			int bytes = net_buf_frags_len(frag);
			int count = 0;

			while (bytes && frag) {
				char msg[8 + 1];

				snprintf(msg, sizeof(msg), ">slip %2d", count);

				hexdump(msg, frag->data, frag->len, 0);

				frag = frag->frags;
				count++;
			}

			SYS_LOG_DBG("[%p] received data %d bytes", slip, bytes);
#endif
			process_msg(slip);
			break;
		}
	}

	*off = 0;

	return buf;
}

static int slip_init(struct device *dev)
{
	struct slip_context *slip = dev->driver_data;

	SYS_LOG_DBG("[%p] dev %p", slip, dev);

	slip->state = STATE_OK;
	slip->rx = NULL;
	slip->first = false;

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_IPV4)
	SYS_LOG_DBG("ARP enabled");
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

static void slip_iface_init(struct net_if *iface)
{
	struct slip_context *slip = net_if_get_device(iface)->driver_data;
	struct net_linkaddr *ll_addr;

	ethernet_init(iface);

	net_eth_set_lldpdu(iface, lldpdu_ptr);

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
		slip->mac_addr[5] = sys_rand32_get();
	}
	net_if_set_link_addr(iface, ll_addr->addr, ll_addr->len,
			     NET_LINK_ETHERNET);
}

static struct slip_context slip_context_data;

static enum ethernet_hw_caps eth_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
		;
}

#if defined(CONFIG_SLIP_TAP) && defined(CONFIG_NET_L2_ETHERNET)
static const struct ethernet_api slip_if_api = {
	.iface_api.init = slip_iface_init,
	.iface_api.send = slip_send,

	.get_capabilities = eth_capabilities,
};

#define _SLIP_L2_LAYER ETHERNET_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#define _SLIP_MTU 1500

ETH_NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME, slip_init, &slip_context_data,
		    NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &slip_if_api,
		    _SLIP_MTU);
#else

static const struct net_if_api slip_if_api = {
	.init = slip_iface_init,
	.send = slip_send,
};

#define _SLIP_L2_LAYER DUMMY_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#define _SLIP_MTU 576

NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME, slip_init, &slip_context_data,
		NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &slip_if_api,
		_SLIP_L2_LAYER, _SLIP_L2_CTX_TYPE, _SLIP_MTU);
#endif
