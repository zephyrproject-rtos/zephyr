/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC layer implementation
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/capture.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/random/random.h>

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
#include "ieee802154_priv.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"

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

static inline void ieee802154_acknowledge(struct net_if *iface, struct ieee802154_mpdu *mpdu)
{
	struct net_pkt *pkt;

	if (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_RX_TX_ACK) {
		return;
	}

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IEEE802154_ACK_PKT_LENGTH, AF_UNSPEC, 0,
					BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	if (ieee802154_create_ack_frame(iface, pkt, mpdu->mhr.fs->sequence)) {
		/* ACK frames must not use the CSMA/CA procedure, see section 6.2.5.1. */
		ieee802154_radio_tx(iface, IEEE802154_TX_MODE_DIRECT, pkt, pkt->buffer);
	}

	net_pkt_unref(pkt);

	return;
}

inline bool ieee802154_prepare_for_ack(struct net_if *iface, struct net_pkt *pkt,
				       struct net_buf *frag)
{
	bool ack_required = ieee802154_is_ar_flag_set(frag);

	if (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_TX_RX_ACK) {
		return ack_required;
	}

	if (ack_required) {
		struct ieee802154_fcf_seq *fs = (struct ieee802154_fcf_seq *)frag->data;
		struct ieee802154_context *ctx = net_if_l2_data(iface);

		ctx->ack_seq = fs->sequence;
		if (k_sem_count_get(&ctx->ack_lock) == 1U) {
			k_sem_take(&ctx->ack_lock, K_NO_WAIT);
		}

		return true;
	}

	return false;
}

enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	if (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_TX_RX_ACK) {
		__ASSERT_NO_MSG(ctx->ack_seq == 0U);
		/* TODO: Release packet in L2 as we're taking ownership. */
		return NET_OK;
	}

	if (pkt->buffer->len == IEEE802154_ACK_PKT_LENGTH) {
		uint8_t len = IEEE802154_ACK_PKT_LENGTH;
		struct ieee802154_fcf_seq *fs;

		fs = ieee802154_validate_fc_seq(net_pkt_data(pkt), NULL, &len);
		if (!fs || fs->fc.frame_type != IEEE802154_FRAME_TYPE_ACK ||
		    fs->sequence != ctx->ack_seq) {
			return NET_CONTINUE;
		}

		k_sem_give(&ctx->ack_lock);

		/* TODO: Release packet in L2 as we're taking ownership. */
		return NET_OK;
	}

	return NET_CONTINUE;
}

inline int ieee802154_wait_for_ack(struct net_if *iface, bool ack_required)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	int ret;

	if (!ack_required ||
	    (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_TX_RX_ACK)) {
		__ASSERT_NO_MSG(ctx->ack_seq == 0U);
		return 0;
	}

	ret = k_sem_take(&ctx->ack_lock, K_MSEC(10));
	if (ret == 0) {
		/* no-op */
	} else if (ret == -EAGAIN) {
		ret = -ETIME;
	} else {
		NET_ERR("Error while waiting for ACK.");
		ret = -EFAULT;
	}

	ctx->ack_seq = 0U;
	return ret;
}

int ieee802154_radio_send(struct net_if *iface, struct net_pkt *pkt, struct net_buf *frag)
{
	uint8_t remaining_attempts = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES + 1;
	bool hw_csma, ack_required;
	int ret;

	NET_DBG("frag %p", frag);

	if (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_RETRANSMISSION) {
		/* A driver that claims retransmission capability must also be able
		 * to wait for ACK frames otherwise it could not decide whether or
		 * not retransmission is required in a standard conforming way.
		 */
		__ASSERT_NO_MSG(ieee802154_radio_get_hw_capabilities(iface) &
				IEEE802154_HW_TX_RX_ACK);
		remaining_attempts = 1;
	}

	hw_csma = IS_ENABLED(CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA) &&
		  ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_CSMA;

	/* Media access (CSMA, ALOHA, ...) and retransmission, see section 6.7.4.4. */
	while (remaining_attempts) {
		if (!hw_csma) {
			ret = ieee802154_wait_for_clear_channel(iface);
			if (ret != 0) {
				NET_WARN("Clear channel assessment failed: dropping fragment %p on "
					 "interface %p.",
					 frag, iface);
				return ret;
			}
		}

		/* No-op in case the driver has IEEE802154_HW_TX_RX_ACK capability. */
		ack_required = ieee802154_prepare_for_ack(iface, pkt, frag);

		/* TX including:
		 *  - CSMA/CA in case the driver has IEEE802154_HW_CSMA capability,
		 *  - waiting for ACK in case the driver has IEEE802154_HW_TX_RX_ACK capability,
		 *  - retransmission on ACK timeout in case the driver has
		 *    IEEE802154_HW_RETRANSMISSION capability.
		 */
		ret = ieee802154_radio_tx(
			iface, hw_csma ? IEEE802154_TX_MODE_CSMA_CA : IEEE802154_TX_MODE_DIRECT,
			pkt, frag);
		if (ret) {
			/* Transmission failure. */
			return ret;
		}

		if (!ack_required) {
			/* See section 6.7.4.4: "A device that sends a frame with the AR field set
			 * to indicate no acknowledgment requested may assume that the transmission
			 * was successfully received and shall not perform the retransmission
			 * procedure."
			 */
			return 0;
		}


		/* No-op in case the driver has IEEE802154_HW_TX_RX_ACK capability. */
		ret = ieee802154_wait_for_ack(iface, ack_required);
		if (ret == 0) {
			/* ACK received - transmission is successful. */
			return 0;
		}

		remaining_attempts--;
	}

	return -EIO;
}

static inline void swap_and_set_pkt_ll_addr(struct net_linkaddr *addr, bool has_pan_id,
					    enum ieee802154_addressing_mode mode,
					    struct ieee802154_address_field *ll)
{
	addr->type = NET_LINK_IEEE802154;

	switch (mode) {
	case IEEE802154_ADDR_MODE_EXTENDED:
		addr->len = IEEE802154_EXT_ADDR_LENGTH;
		addr->addr = has_pan_id ? ll->plain.addr.ext_addr : ll->comp.addr.ext_addr;
		break;

	case IEEE802154_ADDR_MODE_SHORT:
		addr->len = IEEE802154_SHORT_ADDR_LENGTH;
		addr->addr = (uint8_t *)(has_pan_id ? &ll->plain.addr.short_addr
						    : &ll->comp.addr.short_addr);
		break;

	case IEEE802154_ADDR_MODE_NONE:
	default:
		addr->len = 0U;
		addr->addr = NULL;
	}

	/* The net stack expects big endian link layer addresses for POSIX compliance
	 * so we must swap it. This is ok as the L2 address field points into the L2
	 * header of the frame buffer which will no longer be accessible once the
	 * packet reaches upper layers.
	 */
	if (addr->len > 0) {
		sys_mem_swap(addr->addr, addr->len);
	}
}

/**
 * Filters the destination address of the frame.
 *
 * This is done before deciphering and authenticating encrypted frames.
 */
static bool ieee802154_check_dst_addr(struct net_if *iface, struct ieee802154_mhr *mhr)
{
	struct ieee802154_address_field_plain *dst_plain = &mhr->dst_addr->plain;
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	bool ret = false;

	/* Apply filtering requirements from section 6.7.2 c)-e). For a)-b),
	 * see ieee802154_parse_fcf_seq()
	 */

	if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		if (mhr->fs->fc.frame_version < IEEE802154_VERSION_802154 &&
		    mhr->fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON) {
			/* See IEEE 802.15.4-2015, section 7.3.1.1. */
			return true;
		}

		/* TODO: apply d.4 and d.5 when PAN coordinator is implemented */
		/* also, macImplicitBroadcast is not implemented */
		return false;
	}

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	/* c) If a destination PAN ID is included in the frame, it shall match
	 * macPanId or shall be the broadcast PAN ID.
	 */
	if (!(dst_plain->pan_id == IEEE802154_BROADCAST_PAN_ID ||
	      dst_plain->pan_id == sys_cpu_to_le16(ctx->pan_id))) {
		LOG_DBG("Frame PAN ID does not match!");
		goto out;
	}

	if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		/* d.1) A short destination address is included in the frame,
		 * and it matches either macShortAddress or the broadcast
		 * address.
		 */
		if (!(dst_plain->addr.short_addr == IEEE802154_BROADCAST_ADDRESS ||
		      dst_plain->addr.short_addr == sys_cpu_to_le16(ctx->short_addr))) {
			LOG_DBG("Frame dst address (short) does not match!");
			goto out;
		}

	} else if (mhr->fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_EXTENDED) {
		/* d.2) An extended destination address is included in the frame and
		 * matches [...] macExtendedAddress [...].
		 */
		if (memcmp(dst_plain->addr.ext_addr, ctx->ext_addr,
				IEEE802154_EXT_ADDR_LENGTH) != 0) {
			LOG_DBG("Frame dst address (ext) does not match!");
			goto out;
		}

		/* TODO: d.3) The Destination Address field and the Destination PAN ID
		 *       field are not included in the frame and macImplicitBroadcast is TRUE.
		 */

		/* TODO: d.4) The device is the PAN coordinator, only source addressing fields
		 *       are included in a Data frame or MAC command and the source PAN ID
		 *       matches macPanId.
		 */
	}
	ret = true;

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
}

static enum net_verdict ieee802154_recv(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ieee802154_radio_api *radio = net_if_get_device(iface)->api;
	enum net_verdict verdict = NET_CONTINUE;
	struct ieee802154_fcf_seq *fs;
	struct ieee802154_mpdu mpdu;
	bool is_broadcast;
	size_t ll_hdr_len;

	/* The IEEE 802.15.4 stack assumes that drivers provide a single-fragment package. */
	__ASSERT_NO_MSG(pkt->buffer && pkt->buffer->frags == NULL);

	if (!ieee802154_validate_frame(net_pkt_data(pkt), net_pkt_get_len(pkt), &mpdu)) {
		return NET_DROP;
	}

	/* validate LL destination address (when IEEE802154_HW_FILTER not available) */
	if (!(radio->get_capabilities(net_if_get_device(iface)) & IEEE802154_HW_FILTER) &&
	    !ieee802154_check_dst_addr(iface, &mpdu.mhr)) {
		return NET_DROP;
	}

	fs = mpdu.mhr.fs;

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
		return NET_DROP;
	}

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON) {
		verdict = ieee802154_handle_beacon(iface, &mpdu, net_pkt_ieee802154_lqi(pkt));
		if (verdict == NET_CONTINUE) {
			net_pkt_unref(pkt);
			return NET_OK;
		}
		/* Beacons must not be acknowledged, see section 6.7.4.1. */
		return verdict;
	}

	if (ieee802154_is_scanning(iface)) {
		return NET_DROP;
	}

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		verdict = ieee802154_handle_mac_command(iface, &mpdu);
		if (verdict == NET_DROP) {
			return verdict;
		}
	}

	/* At this point the frame is either a MAC command or a data frame
	 * which may have to be acknowledged, see section 6.7.4.1.
	 */

	is_broadcast = false;

	if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		struct ieee802154_address_field *dst_addr = mpdu.mhr.dst_addr;
		uint16_t short_dst_addr;

		short_dst_addr = fs->fc.pan_id_comp ? dst_addr->comp.addr.short_addr
						    : dst_addr->plain.addr.short_addr;
		is_broadcast = short_dst_addr == IEEE802154_BROADCAST_ADDRESS;
	}

	/* Frames that are broadcast must not be acknowledged, see section 6.7.2. */
	if (!is_broadcast) {
		ieee802154_acknowledge(iface, &mpdu);
	}

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		net_pkt_unref(pkt);
		return NET_OK;
	}

	if (!ieee802154_decipher_data_frame(iface, pkt, &mpdu)) {
		return NET_DROP;
	}

	/* Setting LL addresses for upper layers must be done after L2 packet
	 * handling as it will mangle the L2 frame header to comply with upper
	 * layers' (POSIX) requirement to represent network addresses in big endian.
	 */
	swap_and_set_pkt_ll_addr(net_pkt_lladdr_src(pkt), !fs->fc.pan_id_comp,
				 fs->fc.src_addr_mode, mpdu.mhr.src_addr);

	swap_and_set_pkt_ll_addr(net_pkt_lladdr_dst(pkt), true, fs->fc.dst_addr_mode,
				 mpdu.mhr.dst_addr);

	net_pkt_set_ll_proto_type(pkt, ETH_P_IEEE802154);

	pkt_hexdump(RX_PKT_TITLE " (with ll)", pkt, true);

	ll_hdr_len = (uint8_t *)mpdu.payload - net_pkt_data(pkt);
	net_buf_pull(pkt->buffer, ll_hdr_len);

#ifdef CONFIG_NET_6LO
	verdict = ieee802154_6lo_decode_pkt(iface, pkt);
#endif /* CONFIG_NET_6LO */

	if (verdict == NET_CONTINUE) {
		pkt_hexdump(RX_PKT_TITLE, pkt, true);
	}

	return verdict;

	/* At this point the call amounts to (part of) an
	 * MCPS-DATA.indication primitive, see section 8.3.3.
	 */
}

/**
 * Implements (part of) the MCPS-DATA.request/confirm primitives, see sections 8.3.2/3.
 */
static int ieee802154_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint8_t ll_hdr_len = 0, authtag_len = 0;
	static struct net_buf *frame_buf;
	static struct net_buf *pkt_buf;
	bool send_raw = false;
	int len;
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
	struct ieee802154_6lo_fragment_ctx frag_ctx;
	int requires_fragmentation = 0;
#endif

	if (frame_buf == NULL) {
		frame_buf = net_buf_alloc(&tx_frame_buf_pool, K_FOREVER);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && net_pkt_family(pkt) == AF_PACKET) {
		enum net_sock_type socket_type;
		struct net_context *context;

		context = net_pkt_context(pkt);
		if (!context) {
			return -EINVAL;
		}

		socket_type = net_context_get_type(context);
		if (socket_type == SOCK_RAW) {
			send_raw = true;
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET_DGRAM) &&
			   socket_type == SOCK_DGRAM) {
			struct sockaddr_ll *dst_addr = (struct sockaddr_ll *)&context->remote;
			struct sockaddr_ll_ptr *src_addr =
				(struct sockaddr_ll_ptr *)&context->local;

			net_pkt_lladdr_dst(pkt)->addr = dst_addr->sll_addr;
			net_pkt_lladdr_dst(pkt)->len = dst_addr->sll_halen;
			net_pkt_lladdr_src(pkt)->addr = src_addr->sll_addr;
			net_pkt_lladdr_src(pkt)->len = src_addr->sll_halen;
		} else {
			return -EINVAL;
		}
	}

	if (!send_raw) {
		ieee802154_compute_header_and_authtag_len(iface, net_pkt_lladdr_dst(pkt),
							  net_pkt_lladdr_src(pkt), &ll_hdr_len,
							  &authtag_len);

#ifdef CONFIG_NET_6LO
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
		requires_fragmentation =
			ieee802154_6lo_encode_pkt(iface, pkt, &frag_ctx, ll_hdr_len, authtag_len);
		if (requires_fragmentation < 0) {
			return requires_fragmentation;
		}
#else
		ieee802154_6lo_encode_pkt(iface, pkt, NULL, ll_hdr_len, authtag_len);
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */
#endif /* CONFIG_NET_6LO */
	}

	net_capture_pkt(iface, pkt);

	len = 0;
	pkt_buf = pkt->buffer;
	while (pkt_buf) {
		int ret;

		/* Reinitializing frame_buf */
		net_buf_reset(frame_buf);
		net_buf_add(frame_buf, ll_hdr_len);

#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
		if (requires_fragmentation) {
			pkt_buf = ieee802154_6lo_fragment(&frag_ctx, frame_buf, true);
		} else {
			net_buf_add_mem(frame_buf, pkt_buf->data, pkt_buf->len);
			pkt_buf = pkt_buf->frags;
		}
#else
		if (ll_hdr_len + pkt_buf->len + authtag_len > IEEE802154_MTU) {
			NET_ERR("Frame too long: %d", pkt_buf->len);
			return -EINVAL;
		}
		net_buf_add_mem(frame_buf, pkt_buf->data, pkt_buf->len);
		pkt_buf = pkt_buf->frags;
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */

		__ASSERT_NO_MSG(authtag_len <= net_buf_tailroom(frame_buf));
		net_buf_add(frame_buf, authtag_len);

		if (!(send_raw || ieee802154_create_data_frame(ctx, net_pkt_lladdr_dst(pkt),
							       net_pkt_lladdr_src(pkt),
							       frame_buf, ll_hdr_len))) {
			return -EINVAL;
		}

		ret = ieee802154_radio_send(iface, pkt, frame_buf);
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

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (ctx->channel == IEEE802154_NO_CHANNEL) {
		k_sem_give(&ctx->ctx_lock);
		return -ENETDOWN;
	}

	k_sem_give(&ctx->ctx_lock);

	if (state) {
		return ieee802154_radio_start(iface);
	}

	return ieee802154_radio_stop(iface);
}

static enum net_l2_flags ieee802154_flags(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	/* No need for locking as these flags are set once
	 * during L2 initialization and then never changed.
	 */
	return ctx->flags;
}

NET_L2_INIT(IEEE802154_L2, ieee802154_recv, ieee802154_send, ieee802154_enable, ieee802154_flags);

void ieee802154_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	const uint8_t *eui64_be = net_if_get_link_addr(iface)->addr;
	int16_t tx_power = CONFIG_NET_L2_IEEE802154_RADIO_DFLT_TX_POWER;

	NET_DBG("Initializing IEEE 802.15.4 stack on iface %p", iface);

	k_sem_init(&ctx->ctx_lock, 1, 1);
	k_sem_init(&ctx->ack_lock, 0, 1);

	/* no need to lock the context here as it has
	 * not been published yet.
	 */

	/* See section 6.7.1 - Transmission: "Each device shall initialize its data sequence number
	 * (DSN) to a random value and store its current DSN value in the MAC PIB attribute macDsn
	 * [...]."
	 */
	ctx->sequence = sys_rand32_get() & 0xFF;

	ctx->channel = IEEE802154_NO_CHANNEL;
	ctx->flags = NET_L2_MULTICAST;
	if (ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_PROMISC) {
		ctx->flags |= NET_L2_PROMISC_MODE;
	}

	ctx->pan_id = IEEE802154_PAN_ID_NOT_ASSOCIATED;
	ctx->short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	ctx->coord_short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	sys_memcpy_swap(ctx->ext_addr, eui64_be, IEEE802154_EXT_ADDR_LENGTH);

	/* We switch to a link address store that we
	 * own so that we can write user defined short
	 * or extended addresses w/o mutating internal
	 * driver storage.
	 */
	ctx->linkaddr.type = NET_LINK_IEEE802154;
	ctx->linkaddr.len = IEEE802154_EXT_ADDR_LENGTH;
	memcpy(ctx->linkaddr.addr, eui64_be, IEEE802154_EXT_ADDR_LENGTH);
	net_if_set_link_addr(iface, ctx->linkaddr.addr, ctx->linkaddr.len, ctx->linkaddr.type);

	if (IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START) ||
	    IS_ENABLED(CONFIG_NET_CONFIG_SETTINGS)) {
		LOG_DBG("Interface auto start disabled.");
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}

	ieee802154_mgmt_init(iface);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (ieee802154_security_init(&ctx->sec_ctx)) {
		NET_ERR("Initializing link-layer security failed");
	}
#endif

	sys_memcpy_swap(ctx->ext_addr, eui64_be, IEEE802154_EXT_ADDR_LENGTH);
	ieee802154_radio_filter_ieee_addr(iface, ctx->ext_addr);

	if (!ieee802154_radio_set_tx_power(iface, tx_power)) {
		ctx->tx_power = tx_power;
	}
}
