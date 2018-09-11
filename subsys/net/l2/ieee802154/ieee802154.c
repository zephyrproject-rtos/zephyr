/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154) || \
	defined(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET)
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
#include "ieee802154_mgmt_priv.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"

#define BUF_TIMEOUT K_MSEC(50)

#define PKT_TITLE      "IEEE 802.15.4 packet content:"
#define TX_PKT_TITLE   "> " PKT_TITLE
#define RX_PKT_TITLE   "< " PKT_TITLE

#ifdef CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET

#include "net_private.h"

static inline void pkt_hexdump(const char *title, struct net_pkt *pkt,
			       bool in, bool full)
{
	if ((IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_RX) ||
	     IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_FULL)) &&
	    in) {
		net_hexdump_frags(title, pkt, full);
	}

	if ((IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_TX) ||
	     IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_FULL)) &&
	    !in) {
		net_hexdump_frags(title, pkt, full);
	}
}

#ifndef CONFIG_NET_DEBUG_L2_IEEE802154
#undef NET_LOG_ENABLED
#endif /* CONFIG_NET_DEBUG_L2_IEEE802154 */

#else
#define pkt_hexdump(...)
#endif /* CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET */

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
static inline void ieee802154_acknowledge(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu)
{
	struct net_pkt *pkt;
	struct net_buf *frag;

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	pkt = net_pkt_get_reserve_tx(IEEE802154_ACK_PKT_LENGTH, BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	frag = net_pkt_get_frag(pkt, BUF_TIMEOUT);
	if (!frag) {
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_frag_insert(pkt, frag);

	if (ieee802154_create_ack_frame(iface, pkt, mpdu->mhr.fs->sequence)) {
		net_buf_add(frag, IEEE802154_ACK_PKT_LENGTH);

		ieee802154_tx(iface, pkt, frag);
	}

	net_pkt_unref(pkt);

	return;
}
#else
#define ieee802154_acknowledge(...)
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */

static inline void set_pkt_ll_addr(struct net_linkaddr *addr, bool comp,
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

	addr->type = NET_LINK_IEEE802154;
}

#ifdef CONFIG_NET_6LO
static inline
enum net_verdict ieee802154_manage_recv_packet(struct net_if *iface,
					       struct net_pkt *pkt)
{
	enum net_verdict verdict = NET_CONTINUE;
	u32_t src;
	u32_t dst;

	/* Upper IP stack expects the link layer address to be in
	 * big endian format so we must swap it here.
	 */
	if (net_pkt_lladdr_src(pkt)->addr &&
	    net_pkt_lladdr_src(pkt)->len == IEEE802154_EXT_ADDR_LENGTH) {
		sys_mem_swap(net_pkt_lladdr_src(pkt)->addr,
			     net_pkt_lladdr_src(pkt)->len);
	}

	if (net_pkt_lladdr_dst(pkt)->addr &&
	    net_pkt_lladdr_dst(pkt)->len == IEEE802154_EXT_ADDR_LENGTH) {
		sys_mem_swap(net_pkt_lladdr_dst(pkt)->addr,
			     net_pkt_lladdr_dst(pkt)->len);
	}

	/** Uncompress will drop the current fragment. Pkt ll src/dst address
	 * will then be wrong and must be updated according to the new fragment.
	 */
	src = net_pkt_lladdr_src(pkt)->addr ?
		net_pkt_lladdr_src(pkt)->addr - net_pkt_ll(pkt) : 0;
	dst = net_pkt_lladdr_dst(pkt)->addr ?
		net_pkt_lladdr_dst(pkt)->addr - net_pkt_ll(pkt) : 0;

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	verdict = ieee802154_reassemble(pkt);
	if (verdict != NET_CONTINUE) {
		goto out;
	}
#else
	if (!net_6lo_uncompress(pkt)) {
		NET_DBG("Packet decompression failed");
		verdict = NET_DROP;
		goto out;
	}
#endif
	net_pkt_lladdr_src(pkt)->addr = src ? net_pkt_ll(pkt) + src : NULL;
	net_pkt_lladdr_dst(pkt)->addr = dst ? net_pkt_ll(pkt) + dst : NULL;

	pkt_hexdump(RX_PKT_TITLE, pkt, true, false);
out:
	return verdict;
}

static inline bool ieee802154_manage_send_packet(struct net_if *iface,
						 struct net_pkt *pkt)
{
	bool ret;

	pkt_hexdump(TX_PKT_TITLE " (before 6lo)", pkt, false, false);

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	ret = net_6lo_compress(pkt, true, ieee802154_fragment);
#else
	ret = net_6lo_compress(pkt, true, NULL);
#endif

	pkt_hexdump(TX_PKT_TITLE " (after 6lo)", pkt, false, false);

	return ret;
}

#else /* CONFIG_NET_6LO */

#define ieee802154_manage_recv_packet(...) NET_CONTINUE
#define ieee802154_manage_send_packet(...) true

#endif /* CONFIG_NET_6LO */

static enum net_verdict ieee802154_recv(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct ieee802154_mpdu mpdu;

	if (!ieee802154_validate_frame(net_pkt_ll(pkt),
				       net_pkt_get_len(pkt), &mpdu)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON) {
		return ieee802154_handle_beacon(iface, &mpdu,
						net_pkt_ieee802154_lqi(pkt));
	}

	if (ieee802154_is_scanning(iface)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		return ieee802154_handle_mac_command(iface, &mpdu);
	}

	/* At this point the frame has to be a DATA one */

	ieee802154_acknowledge(iface, &mpdu);

	net_pkt_set_ll_reserve(pkt, mpdu.payload - (void *)net_pkt_ll(pkt));
	net_buf_pull(pkt->frags, net_pkt_ll_reserve(pkt));

	set_pkt_ll_addr(net_pkt_lladdr_src(pkt), mpdu.mhr.fs->fc.pan_id_comp,
			mpdu.mhr.fs->fc.src_addr_mode, mpdu.mhr.src_addr);

	set_pkt_ll_addr(net_pkt_lladdr_dst(pkt), false,
			mpdu.mhr.fs->fc.dst_addr_mode, mpdu.mhr.dst_addr);

	if (!ieee802154_decipher_data_frame(iface, pkt, &mpdu)) {
		return NET_DROP;
	}

	pkt_hexdump(RX_PKT_TITLE " (with ll)", pkt, true, true);

	return ieee802154_manage_recv_packet(iface, pkt);
}

static enum net_verdict ieee802154_send(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	u8_t reserved_space = net_pkt_ll_reserve(pkt);
	struct net_buf *frag;

	if (net_pkt_family(pkt) != AF_INET6) {
		return NET_DROP;
	}

	if (!ieee802154_manage_send_packet(iface, pkt)) {
		return NET_DROP;
	}

	frag = pkt->frags;
	while (frag) {
		if (frag->len > IEEE802154_MTU) {
			NET_ERR("Frag %p as too big length %u",
				frag, frag->len);
			return NET_DROP;
		}

		if (!ieee802154_create_data_frame(ctx, net_pkt_lladdr_dst(pkt),
						  frag, reserved_space)) {
			return NET_DROP;
		}

		frag = frag->frags;
	}

	pkt_hexdump(TX_PKT_TITLE " (with ll)", pkt, false, true);

	net_if_queue_tx(iface, pkt);

	return NET_OK;
}

static u16_t ieee802154_reserve(struct net_if *iface, void *data)
{
	return ieee802154_compute_header_size(iface, (struct in6_addr *)data);
}

static int ieee802154_enable(struct net_if *iface, bool state)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	NET_DBG("iface %p %s", iface, state ? "up" : "down");

	if (ctx->channel == IEEE802154_NO_CHANNEL) {
		return -ENETDOWN;
	}

	if (state) {
		return ieee802154_start(iface);
	}

	return ieee802154_stop(iface);
}

enum net_l2_flags ieee802154_flags(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return ctx->flags;
}

NET_L2_INIT(IEEE802154_L2,
	    ieee802154_recv, ieee802154_send,
	    ieee802154_reserve, ieee802154_enable, ieee802154_flags);

void ieee802154_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	const u8_t *mac = net_if_get_link_addr(iface)->addr;
	s16_t tx_power = CONFIG_NET_L2_IEEE802154_RADIO_DFLT_TX_POWER;
	u8_t long_addr[8];

	NET_DBG("Initializing IEEE 802.15.4 stack on iface %p", iface);

	ctx->channel = IEEE802154_NO_CHANNEL;
	ctx->flags = NET_L2_MULTICAST;

	ieee802154_mgmt_init(iface);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (ieee802154_security_init(&ctx->sec_ctx)) {
		NET_ERR("Initializing link-layer security failed");
	}
#endif

	sys_memcpy_swap(long_addr, mac, 8);
	memcpy(ctx->ext_addr, long_addr, 8);
	ieee802154_filter_ieee_addr(iface, ctx->ext_addr);

	if (!ieee802154_set_tx_power(iface, tx_power)) {
		ctx->tx_power = tx_power;
	}
}
