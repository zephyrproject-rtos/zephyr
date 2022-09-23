/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC layer implementation
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/capture.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>

#ifdef CONFIG_NET_6LO
#include "ieee802154_6lo.h"

#include <6lo.h>
#include <ipv6.h>

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
#include "ieee802154_6lo_fragment.h"
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */
#endif /* CONFIG_NET_6LO */

#include "ieee802154_frame.h"
#include "ieee802154_mgmt_priv.h"
#include "ieee802154_radio_utils.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"

#include <zephyr/net/ieee802154_radio.h>

#define BUF_TIMEOUT K_MSEC(50)

NET_BUF_POOL_DEFINE(tx_frame_buf_pool, 1, IEEE802154_MTU, 8, NULL);

#define PKT_TITLE    "IEEE 802.15.4 packet content:"
#define TX_PKT_TITLE "> " PKT_TITLE
#define RX_PKT_TITLE "< " PKT_TITLE

#ifdef CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET

#include "net_private.h"

static inline void pkt_hexdump(const char *title, struct net_pkt *pkt, bool in)
{
	if (IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_RX) && in) {
		net_pkt_hexdump(pkt, title);
	}

	if (IS_ENABLED(CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET_TX) && !in) {
		net_pkt_hexdump(pkt, title);
	}
}

#else
#define pkt_hexdump(...)
#endif /* CONFIG_NET_DEBUG_L2_IEEE802154_DISPLAY_PACKET */

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
static inline void ieee802154_acknowledge(struct net_if *iface, struct ieee802154_mpdu *mpdu)
{
	struct net_pkt *pkt;

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IEEE802154_ACK_PKT_LENGTH, AF_UNSPEC, 0,
					BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	if (ieee802154_create_ack_frame(iface, pkt, mpdu->mhr.fs->sequence)) {
		ieee802154_tx(iface, IEEE802154_TX_MODE_DIRECT, pkt, pkt->buffer);
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
		/* TODO: Handle short address (lookup known nbr, ...) */
		addr->len = 0U;
		addr->addr = NULL;
	}

	addr->type = NET_LINK_IEEE802154;
}

/**
 * Filters the destination address of the frame (used when IEEE802154_HW_FILTER
 * is not available).
 */
static bool ieeee802154_check_dst_addr(struct net_if *iface, struct ieee802154_mhr *mhr)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_address_field_plain *dst_plain = &mhr->dst_addr->plain;

	/*
	 * Apply filtering requirements from chapter 6.7.2 of the IEEE
	 * 802.15.4-2015 standard:
	 */

	if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		/* TODO: apply d.4 and d.5 when PAN coordinator is implemented */
		/* also, macImplicitBroadcast is not implemented */
		return false;
	}

	/*
	 * c. If a destination PAN ID is included in the frame, it shall match
	 * macPanId or shall be the broadcastPAN ID
	 */
	if (!(dst_plain->pan_id == IEEE802154_BROADCAST_PAN_ID ||
	      dst_plain->pan_id == ctx->pan_id)) {
		LOG_DBG("Frame PAN ID does not match!");
		return false;
	}

	if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		/*
		 * d.1. A short destination address is included in the frame,
		 * and it matches either macShortAddress or the broadcast
		 * address.
		 */
		if (!(dst_plain->addr.short_addr == IEEE802154_BROADCAST_ADDRESS ||
		      dst_plain->addr.short_addr == ctx->short_addr)) {
			LOG_DBG("Frame dst address (short) does not match!");
			return false;
		}

	} else if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_EXTENDED) {
		/*
		 * An extended destination address is included in the frame and
		 * matches either macExtendedAddress or, if macGroupRxMode is
		 * set to TRUE, an 64-bit extended unique identifier (EUI-64)
		 * group address.
		 */
		if (memcmp(dst_plain->addr.ext_addr, ctx->ext_addr,
				IEEE802154_EXT_ADDR_LENGTH) != 0) {
			LOG_DBG("Frame dst address (ext) does not match!");
			return false;
		}
	}

	return true;
}

static enum net_verdict ieee802154_recv(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ieee802154_radio_api *radio = net_if_get_device(iface)->api;
	struct ieee802154_mpdu mpdu;
	size_t hdr_len;

	if (!ieee802154_validate_frame(net_pkt_data(pkt), net_pkt_get_len(pkt), &mpdu)) {
		return NET_DROP;
	}

	/* validate LL destination address (when IEEE802154_HW_FILTER not available) */
	if (!(radio->get_capabilities(net_if_get_device(iface)) & IEEE802154_HW_FILTER) &&
	    !ieeee802154_check_dst_addr(iface, &mpdu.mhr)) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
		return NET_DROP;
	}

	if (mpdu.mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON) {
		return ieee802154_handle_beacon(iface, &mpdu, net_pkt_ieee802154_lqi(pkt));
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

	set_pkt_ll_addr(net_pkt_lladdr_dst(pkt), false, mpdu.mhr.fs->fc.dst_addr_mode,
			mpdu.mhr.dst_addr);

	if (!ieee802154_decipher_data_frame(iface, pkt, &mpdu)) {
		return NET_DROP;
	}

	pkt_hexdump(RX_PKT_TITLE " (with ll)", pkt, true);

	hdr_len = (uint8_t *)mpdu.payload - net_pkt_data(pkt);
	net_buf_pull(pkt->buffer, hdr_len);

#ifdef CONFIG_NET_6LO
	enum net_verdict verdict = ieee802154_6lo_decode_pkt(iface, pkt);

	pkt_hexdump(RX_PKT_TITLE, pkt, true);
	return verdict;
#else
	return NET_CONTINUE;
#endif /* CONFIG_NET_6LO */
}

static int ieee802154_send(struct net_if *iface, struct net_pkt *pkt)
{
	static struct net_buf *frame_buf;
	uint8_t ll_hdr_len = 0;
	bool send_raw = false;
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	struct ieee802154_6lo_fragment_ctx f_ctx;
	int requires_fragmentation = 0;
#endif

	if (frame_buf == NULL) {
		frame_buf = net_buf_alloc(&tx_frame_buf_pool, K_FOREVER);
	}

#if defined(CONFIG_NET_SOCKETS_PACKET)
	uint8_t pkt_family = net_pkt_family(pkt);

	if (pkt_family == AF_PACKET) {
		struct net_context *context = net_pkt_context(pkt);

		if (!context) {
			return -EINVAL;
		}
		switch (net_context_get_type(context)) {
		case SOCK_RAW:
			send_raw = true;
			break;
#if defined(CONFIG_NET_SOCKETS_PACKET_DGRAM)
		case SOCK_DGRAM: {
			struct sockaddr_ll *dst_addr = (struct sockaddr_ll *)&context->remote;
			struct sockaddr_ll_ptr *src_addr =
				(struct sockaddr_ll_ptr *)&context->local;

			net_pkt_lladdr_dst(pkt)->addr = dst_addr->sll_addr;
			net_pkt_lladdr_dst(pkt)->len = dst_addr->sll_halen;
			net_pkt_lladdr_src(pkt)->addr = src_addr->sll_addr;
			net_pkt_lladdr_src(pkt)->len = src_addr->sll_halen;
			break;
		}
#endif
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_NET_SOCKETS_PACKET */

	if (!send_raw) {
		ll_hdr_len = ieee802154_compute_header_and_authtag_size(
			iface, net_pkt_lladdr_dst(pkt), net_pkt_lladdr_src(pkt));

#ifdef CONFIG_NET_6LO
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
		requires_fragmentation = ieee802154_6lo_encode_pkt(iface, pkt, &f_ctx, ll_hdr_len);
		if (requires_fragmentation < 0) {
			return requires_fragmentation;
		}
#else
		ieee802154_6lo_encode_pkt(iface, pkt, NULL, ll_hdr_len);
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */
#endif /* CONFIG_NET_6LO */
	}

	net_capture_pkt(iface, pkt);

	int len = 0;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct net_buf *buf = pkt->buffer;
	while (buf) {
		int ret;

		/* Reinitializing frame_buf */
		net_buf_reset(frame_buf);
		net_buf_add(frame_buf, ll_hdr_len);

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
		if (requires_fragmentation) {
			buf = ieee802154_6lo_fragment(&f_ctx, frame_buf, true);
		} else {
			net_buf_add_mem(frame_buf, buf->data, buf->len);
			buf = buf->frags;
		}
#else

		if (buf->len > IEEE802154_MTU) {
			NET_ERR("Wrong packet length: %d", buf->len);
			return -EINVAL;
		}
		net_buf_add_mem(frame_buf, buf->data, buf->len);
		buf = buf->frags;
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */

		if (!(send_raw || ieee802154_create_data_frame(ctx, net_pkt_lladdr_dst(pkt),
							       net_pkt_lladdr_src(pkt),
							       frame_buf, ll_hdr_len))) {
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA) &&
		    ieee802154_get_hw_capabilities(iface) & IEEE802154_HW_CSMA) {
			/* CSMA in hardware */
			ret = ieee802154_tx(iface, IEEE802154_TX_MODE_CSMA_CA, pkt, frame_buf);
		} else {
			/* Media access (direct, CSMA, ALOHA, ...) in software */
			ret = ieee802154_radio_send(iface, pkt, frame_buf);
		}

		if (ret) {
			return ret;
		}

		len += frame_buf->len;
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

NET_L2_INIT(IEEE802154_L2, ieee802154_recv, ieee802154_send, ieee802154_enable, ieee802154_flags);

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
