/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/capture.h>

#include "ipv6.h"

#include <errno.h>

#include "ieee802154_fragment.h"
#include <6lo.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"
#include "ieee802154_mgmt_priv.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"
#include "ieee802154_radio_utils.h"

#define BUF_TIMEOUT K_MSEC(50)

/* No need to hold space for the FCS */
static uint8_t frame_buffer_data[IEEE802154_MTU - 2];

static struct net_buf frame_buf = {
	.data = frame_buffer_data,
	.size = IEEE802154_MTU - 2,
	.frags = NULL,
	.__buf = frame_buffer_data,
};

#define PKT_TITLE      "IEEE 802.15.4 packet content:"
#define TX_PKT_TITLE   "> " PKT_TITLE
#define RX_PKT_TITLE   "< " PKT_TITLE

#ifdef CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET

#include "net_private.h"

static inline void pkt_hexdump(const char *title, struct net_pkt *pkt,
			       bool in)
{
	if (IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_RX) &&
	    in) {
		net_pkt_hexdump(pkt, title);
	}

	if (IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_TX) &&
	    !in) {
		net_pkt_hexdump(pkt, title);
	}
}

#else
#define pkt_hexdump(...)
#endif /* CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET */

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
static inline void ieee802154_acknowledge(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu)
{
	struct net_pkt *pkt;

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IEEE802154_ACK_PKT_LENGTH,
					AF_UNSPEC, 0, BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	if (ieee802154_create_ack_frame(iface, pkt, mpdu->mhr.fs->sequence)) {
		ieee802154_tx(iface, IEEE802154_TX_MODE_DIRECT,
			      pkt, pkt->buffer);
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
		addr->len = 0U;
		addr->addr = NULL;
	}

	addr->type = NET_LINK_IEEE802154;
}

#ifdef CONFIG_NET_6LO
static inline
enum net_verdict ieee802154_manage_recv_packet(struct net_if *iface,
					       struct net_pkt *pkt,
					       size_t hdr_len)
{
	enum net_verdict verdict = NET_CONTINUE;

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

	pkt_hexdump(RX_PKT_TITLE, pkt, true);
out:
	return verdict;
}
#else /* CONFIG_NET_6LO */
#define ieee802154_manage_recv_packet(...) NET_CONTINUE
#endif /* CONFIG_NET_6LO */

static enum net_verdict ieee802154_recv(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct ieee802154_mpdu mpdu;
	size_t hdr_len;

	if (!ieee802154_validate_frame(net_pkt_data(pkt),
				       net_pkt_get_len(pkt), &mpdu)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
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

	set_pkt_ll_addr(net_pkt_lladdr_src(pkt), mpdu.mhr.fs->fc.pan_id_comp,
			mpdu.mhr.fs->fc.src_addr_mode, mpdu.mhr.src_addr);

	set_pkt_ll_addr(net_pkt_lladdr_dst(pkt), false,
			mpdu.mhr.fs->fc.dst_addr_mode, mpdu.mhr.dst_addr);

	if (!ieee802154_decipher_data_frame(iface, pkt, &mpdu)) {
		return NET_DROP;
	}

	pkt_hexdump(RX_PKT_TITLE " (with ll)", pkt, true);

	hdr_len = (uint8_t *)mpdu.payload - net_pkt_data(pkt);
	net_buf_pull(pkt->buffer, hdr_len);

	return ieee802154_manage_recv_packet(iface, pkt, hdr_len);

}

static int ieee802154_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_fragment_ctx f_ctx;
	struct net_buf *buf;
	uint8_t ll_hdr_size;
	bool fragment;
	int len;

	if (net_pkt_family(pkt) != AF_INET6) {
		return -EINVAL;
	}

	ll_hdr_size = ieee802154_compute_header_size(iface,
						     &NET_IPV6_HDR(pkt)->dst);

	/* len will hold the hdr size difference on success */
	len = net_6lo_compress(pkt, true);
	if (len < 0) {
		return len;
	}

	net_capture_pkt(iface, pkt);

	fragment = ieee802154_fragment_is_needed(pkt, ll_hdr_size);
	ieee802154_fragment_ctx_init(&f_ctx, pkt, len, true);

	len = 0;
	frame_buf.len = 0U;
	buf = pkt->buffer;

	while (buf) {
		int ret;

		net_buf_add(&frame_buf, ll_hdr_size);

		if (fragment) {
			ieee802154_fragment(&f_ctx, &frame_buf, true);
			buf = f_ctx.buf;
		} else {
			memcpy(frame_buf.data + frame_buf.len,
			       buf->data, buf->len);
			net_buf_add(&frame_buf, buf->len);
			buf = buf->frags;
		}

		if (!ieee802154_create_data_frame(ctx, net_pkt_lladdr_dst(pkt),
						  &frame_buf, ll_hdr_size)) {
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA) &&
		    ieee802154_get_hw_capabilities(iface) &
		    IEEE802154_HW_CSMA) {
			ret = ieee802154_tx(iface, IEEE802154_TX_MODE_CSMA_CA,
					    pkt, &frame_buf);
		} else {
			ret = ieee802154_radio_send(iface, pkt, &frame_buf);
		}

		if (ret) {
			return ret;
		}

		len += frame_buf.len;

		/* Reinitializing frame_buf */
		frame_buf.len = 0U;
	}

	net_pkt_unref(pkt);

	return len;
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
	    ieee802154_enable, ieee802154_flags);

void ieee802154_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	const uint8_t *mac = net_if_get_link_addr(iface)->addr;
	int16_t tx_power = CONFIG_NET_L2_IEEE802154_RADIO_DFLT_TX_POWER;
	uint8_t long_addr[8];

	NET_DBG("Initializing IEEE 802.15.4 stack on iface %p", iface);

	ctx->channel = IEEE802154_NO_CHANNEL;
	ctx->flags = NET_L2_MULTICAST;

	if (IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START)) {
		LOG_DBG("Interface auto start disabled.");
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}

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
