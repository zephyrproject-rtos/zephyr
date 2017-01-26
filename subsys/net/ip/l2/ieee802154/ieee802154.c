/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154)
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>

#include "ipv6.h"

#include <errno.h>

#ifdef CONFIG_NET_6LO
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
#include "ieee802154_fragment.h"
#endif
#include <6lo.h>
#endif /* CONFIG_NET_6LO */

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"
#include "ieee802154_mgmt.h"

#if 0

#include <misc/printk.h>

static inline void hexdump(uint8_t *pkt, uint16_t length, uint8_t reserve)
{
	int i;

	for (i = 0; i < length;) {
		int j;

		printk("\t");

		for (j = 0; j < 10 && i < length; j++, i++) {
#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
			if (i < reserve && reserve) {
				printk(SYS_LOG_COLOR_YELLOW);
			} else {
				printk(SYS_LOG_COLOR_OFF);
			}
#endif
			printk("%02x ", *pkt++);
		}

#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
		if (i < reserve) {
			printk(SYS_LOG_COLOR_OFF);
		}
#endif
		printk("\n");
	}
}

static void pkt_hexdump(struct net_buf *buf, bool each_frag_reserve)
{
	uint16_t reserve = each_frag_reserve ? net_nbuf_ll_reserve(buf) : 0;
	struct net_buf *frag;

	printk("IEEE 802.15.4 packet content:\n");

	frag = buf->frags;
	while (frag) {
		hexdump(each_frag_reserve ?
			frag->data - reserve : frag->data,
			frag->len + reserve, reserve);

		frag = frag->frags;
	}
}
#else
#define pkt_hexdump(...)
#endif

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
static inline void ieee802154_acknowledge(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu)
{
	struct net_buf *buf, *frag;

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return;
	}

	frag = net_nbuf_get_reserve_data(IEEE802154_ACK_PKT_LENGTH);

	net_buf_frag_insert(buf, frag);
	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));

	if (ieee802154_create_ack_frame(iface, buf, mpdu->mhr.fs->sequence)) {
		const struct ieee802154_radio_api *radio =
			iface->dev->driver_api;

		net_buf_add(frag, IEEE802154_ACK_PKT_LENGTH);

		radio->tx(iface->dev, buf, frag);
	}

	net_nbuf_unref(buf);

	return;
}
#else
#define ieee802154_acknowledge(...)
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */

static inline void set_buf_ll_addr(struct net_linkaddr *addr, bool comp,
				   enum ieee802154_addressing_mode mode,
				   struct ieee802154_address_field *ll)
{
	if (mode == IEEE802154_ADDR_MODE_NONE) {
		return;
	}

	if (mode == IEEE802154_ADDR_MODE_EXTENDED) {
		addr->len = IEEE802154_EXT_ADDR_LENGTH;

		if (comp) {
			addr->addr = ll->comp.addr.ext_addr;
		} else {
			addr->addr = ll->plain.addr.ext_addr;
		}
	} else {
		/* ToDo: Handle short address (lookup known nbr, ...) */
		addr->len = 0;
		addr->addr = NULL;
	}
}

#ifdef CONFIG_NET_6LO
static inline
enum net_verdict ieee802154_manage_recv_buffer(struct net_if *iface,
					       struct net_buf *buf)
{
	enum net_verdict verdict = NET_CONTINUE;
	uint32_t src;
	uint32_t dst;

	/* Upper IP stack expects the link layer address to be in
	 * big endian format so we must swap it here.
	 */
	if (net_nbuf_ll_src(buf)->addr &&
	    net_nbuf_ll_src(buf)->len == IEEE802154_EXT_ADDR_LENGTH) {
		sys_mem_swap(net_nbuf_ll_src(buf)->addr,
			     net_nbuf_ll_src(buf)->len);
	}

	if (net_nbuf_ll_dst(buf)->addr &&
	    net_nbuf_ll_dst(buf)->len == IEEE802154_EXT_ADDR_LENGTH) {
		sys_mem_swap(net_nbuf_ll_dst(buf)->addr,
			     net_nbuf_ll_dst(buf)->len);
	}

	/** Uncompress will drop the current fragment. Buf ll src/dst address
	 * will then be wrong and must be updated according to the new fragment.
	 */
	src = net_nbuf_ll_src(buf)->addr ?
		net_nbuf_ll_src(buf)->addr - net_nbuf_ll(buf) : 0;
	dst = net_nbuf_ll_dst(buf)->addr ?
		net_nbuf_ll_dst(buf)->addr - net_nbuf_ll(buf) : 0;

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	verdict = ieee802154_reassemble(buf);
	if (verdict == NET_DROP) {
		goto out;
	}
#else
	if (!net_6lo_uncompress(buf)) {
		NET_DBG("Packet decompression failed");
		verdict = NET_DROP;
		goto out;
	}
#endif
	net_nbuf_ll_src(buf)->addr = src ? net_nbuf_ll(buf) + src : NULL;
	net_nbuf_ll_dst(buf)->addr = dst ? net_nbuf_ll(buf) + dst : NULL;

	pkt_hexdump(buf, false);
out:
	return verdict;
}

static inline bool ieee802154_manage_send_buffer(struct net_if *iface,
						 struct net_buf *buf)
{
	bool ret;

	pkt_hexdump(buf, false);

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	ret = net_6lo_compress(buf, true, ieee802154_fragment);
#else
	ret = net_6lo_compress(buf, true, NULL);
#endif

	pkt_hexdump(buf, false);

	return ret;
}

#else /* CONFIG_NET_6LO */

#define ieee802154_manage_recv_buffer(...) NET_CONTINUE
#define ieee802154_manage_send_buffer(...) true

#endif /* CONFIG_NET_6LO */

static enum net_verdict ieee802154_recv(struct net_if *iface,
					struct net_buf *buf)
{
	struct ieee802154_mpdu mpdu;

	if (!ieee802154_validate_frame(net_nbuf_ll(buf),
				       net_buf_frags_len(buf), &mpdu)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON) {
		return ieee802154_handle_beacon(iface, &mpdu);
	}

	if (ieee802154_is_scanning(iface)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		return ieee802154_handle_mac_command(iface, &mpdu);
	}

	/* At this point the frame has to be a DATA one */

	ieee802154_acknowledge(iface, &mpdu);

	net_nbuf_set_ll_reserve(buf, mpdu.payload - (void *)net_nbuf_ll(buf));
	net_buf_pull(buf->frags, net_nbuf_ll_reserve(buf));

	set_buf_ll_addr(net_nbuf_ll_src(buf), mpdu.mhr.fs->fc.pan_id_comp,
			mpdu.mhr.fs->fc.src_addr_mode, mpdu.mhr.src_addr);

	set_buf_ll_addr(net_nbuf_ll_dst(buf), false,
			mpdu.mhr.fs->fc.dst_addr_mode, mpdu.mhr.dst_addr);

	pkt_hexdump(buf, true);

	return ieee802154_manage_recv_buffer(iface, buf);
}

static enum net_verdict ieee802154_send(struct net_if *iface,
					struct net_buf *buf)
{
	uint8_t reserved_space = net_nbuf_ll_reserve(buf);
	struct net_buf *frag;
	struct in6_addr dst;

	if (net_nbuf_family(buf) != AF_INET6) {
		return NET_DROP;
	}

	if (!net_nbuf_ll_dst(buf)->addr &&
	    !net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		buf = net_ipv6_prepare_for_send(buf);
		if (!buf) {
			return NET_CONTINUE;
		}
	}

	/* 6lo is going to compress the ipv6 header, and thus accessing
	 * packet's ipv6 address won't be possible anymore when creating
	 * the frame */
	memcpy(&dst, &NET_IPV6_BUF(buf)->dst, sizeof(struct in6_addr));

	if (!ieee802154_manage_send_buffer(iface, buf)) {
		return NET_DROP;
	}

	frag = buf->frags;
	while (frag) {
		if (frag->len > IEEE802154_MTU) {
			NET_ERR("Frag %p as too big length %u",
				frag, frag->len);
			return NET_DROP;
		}

		if (!ieee802154_create_data_frame(iface, &dst,
						  frag->data - reserved_space,
						  reserved_space)) {
			return NET_DROP;
		}

		frag = frag->frags;
	}

	pkt_hexdump(buf, true);

	net_if_queue_tx(iface, buf);

	return NET_OK;
}

static uint16_t ieeee802154_reserve(struct net_if *iface, void *data)
{
	return ieee802154_compute_header_size(iface, (struct in6_addr *)data);
}

NET_L2_INIT(IEEE802154_L2,
	    ieee802154_recv, ieee802154_send, ieeee802154_reserve, NULL);

void ieee802154_init(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		iface->dev->driver_api;

	NET_DBG("Initializing IEEE 802.15.4 stack on iface %p", iface);

	ieee802154_mgmt_init(iface);

#ifdef CONFIG_NET_L2_IEEE802154_ORFD
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	const uint8_t *mac = iface->link_addr.addr;
	uint8_t long_addr[8];
	uint16_t short_addr;

	sys_memcpy_swap(long_addr, mac, 8);

	short_addr = (mac[0] << 8) + mac[1];

	radio->set_short_addr(iface->dev, short_addr);
	radio->set_ieee_addr(iface->dev, long_addr);

	ctx->pan_id = CONFIG_NET_L2_IEEE802154_ORFD_PAN_ID;
	ctx->channel = CONFIG_NET_L2_IEEE802154_ORFD_CHANNEL;

	radio->set_pan_id(iface->dev, ctx->pan_id);
	radio->set_channel(iface->dev, ctx->channel);
#endif
	radio->start(iface->dev);
}
