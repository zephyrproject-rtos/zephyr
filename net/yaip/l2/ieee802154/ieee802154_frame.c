/*
 * Copyright (c) 2016 Intel Corporation.
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

#ifdef CONFIG_NET_L2_IEEE802154_DEBUG
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_DEBUG 1

#define dbg_print_fs(fs)						\
	NET_DBG("fs: %u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u - %u",		\
		fs->fc.frame_type, fs->fc.security_enabled,		\
		fs->fc.frame_pending, fs->fc.ar, fs->fc.pan_id_comp,	\
		fs->fc.reserved, fs->fc.seq_num_suppr, fs->fc.ie_list,	\
		fs->fc.dst_addr_mode, fs->fc.frame_version,		\
		fs->fc.src_addr_mode,					\
		fs->sequence)
#else
#define dbg_print_fs(...)
#endif /* CONFIG_NET_L2_IEEE802154_DEBUG */

#include <net/net_core.h>
#include <net/net_if.h>

#include <ipv6.h>
#include <nbr.h>

#include "ieee802154_frame.h"

static inline struct ieee802154_fcf_seq *
validate_fc_seq(uint8_t *buf, uint8_t **p_buf)
{
	struct ieee802154_fcf_seq *fs = (struct ieee802154_fcf_seq *)buf;

	dbg_print_fs(fs);

	/** Basic FC checks */
	if (fs->fc.frame_type >= IEEE802154_FRAME_TYPE_RESERVED ||
	    fs->fc.frame_version >= IEEE802154_VERSION_RESERVED) {
		return NULL;
	}

	/** Only for versions 2003/2006 */
	if (fs->fc.frame_version < IEEE802154_VERSION_802154 &&
	    (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_RESERVED ||
	     fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_RESERVED ||
	     fs->fc.frame_type >= IEEE802154_FRAME_TYPE_LLDN)) {
		return NULL;
	}

	/** See section 5.2.2.2.1 */
	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_DATA &&
	    fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE &&
	    fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		return NULL;
	}

	*p_buf = buf + 3;

	return fs;
}

static inline struct ieee802154_address_field *
validate_addr(uint8_t *buf, uint8_t **p_buf,
	      enum ieee802154_addressing_mode mode,
	      bool pan_id_compression)
{
	*p_buf = buf;

	NET_DBG("Buf %p - mode %d - pan id comp %d",
		buf, mode, pan_id_compression);

	if (mode == IEEE802154_ADDR_MODE_NONE) {
		return NULL;
	}

	if (!pan_id_compression) {
		*p_buf += IEEE802154_PAN_ID_LENGTH;
	}

	if (mode == IEEE802154_ADDR_MODE_SHORT) {
		*p_buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else {
		/* IEEE802154_ADDR_MODE_EXTENDED */
		*p_buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	return (struct ieee802154_address_field *)buf;
}

static inline bool
validate_payload_and_mfr(struct ieee802154_mpdu *mpdu,
			 uint8_t *buf, uint8_t *p_buf, uint8_t length)
{
	uint8_t payload_length;

	payload_length = length - (p_buf - buf) - IEEE802154_MFR_LENGTH;

	NET_DBG("Header size: %u, vs total length %u: payload size %u",
		p_buf - buf, length, payload_length);

	if (mpdu->mhr.fs->fc.frame_type == IEEE802154_FRAME_TYPE_ACK) {
		if (payload_length) {
			return false;
		}

		mpdu->payload = NULL;
	} else {
		 /** A data frame embeds a payload */
		if (payload_length == 0) {
			return false;
		}

		mpdu->payload = (void *)p_buf;
	}

	mpdu->mfr = (struct ieee802154_mfr *)(p_buf + payload_length);

	return true;
}

bool ieee802154_validate_frame(uint8_t *buf, uint8_t length,
			       struct ieee802154_mpdu *mpdu)
{
	uint8_t *p_buf;

	if (length > IEEE802154_MTU || length < IEEE802154_MIN_LENGTH) {
		NET_DBG("Wrong packet length: %d", length);
		return false;
	}

	mpdu->mhr.fs = validate_fc_seq(buf, &p_buf);
	if (!mpdu->mhr.fs) {
		return false;
	}

	/** ToDo: Support other frames */
	if (mpdu->mhr.fs->fc.frame_type != IEEE802154_FRAME_TYPE_DATA &&
	    mpdu->mhr.fs->fc.frame_type != IEEE802154_FRAME_TYPE_ACK) {
		return false;
	}

	mpdu->mhr.dst_addr = validate_addr(p_buf, &p_buf,
					   mpdu->mhr.fs->fc.dst_addr_mode,
					   false);
	mpdu->mhr.src_addr = validate_addr(p_buf, &p_buf,
					   mpdu->mhr.fs->fc.src_addr_mode,
					   (mpdu->mhr.fs->fc.pan_id_comp));

	return validate_payload_and_mfr(mpdu, buf, p_buf, length);
}

uint16_t ieee802154_compute_header_size(struct net_if *iface,
					struct in6_addr *dst)
{
	uint16_t hdr_len = sizeof(struct ieee802154_fcf_seq);

	/** if dst is NULL, we'll consider it as a brodcast header */
	if (!dst ||
	    net_is_ipv6_addr_mcast(dst) ||
	    net_is_ipv6_addr_unspecified(dst)) {
		/* 4 dst pan/addr + 8 src addr */
		hdr_len += IEEE802154_PAN_ID_LENGTH +
			IEEE802154_SHORT_ADDR_LENGTH +
			IEEE802154_EXT_ADDR_LENGTH;
	} else {
		struct net_nbr *nbr;

		nbr = net_ipv6_nbr_lookup(iface, dst);
		if (nbr) {
			/* ToDo: handle short addresses */
			/* dst pan/addr + src addr */
			hdr_len += IEEE802154_PAN_ID_LENGTH +
				(IEEE802154_EXT_ADDR_LENGTH * 2);
		} else {
			/* src pan/addr only */
			hdr_len += IEEE802154_PAN_ID_LENGTH +
				IEEE802154_EXT_ADDR_LENGTH;
		}
	}

	/* Todo: handle security aux header */

	NET_DBG("Computed size of %u", hdr_len);

	return hdr_len;
}

static inline struct ieee802154_fcf_seq *generate_fcf_grounds(uint8_t **p_buf)
{
	struct ieee802154_fcf_seq *fs;

	fs = (struct ieee802154_fcf_seq *) *p_buf;

	fs->fc.security_enabled = 0;
	fs->fc.frame_pending = 0;
	fs->fc.ar = 0;
	fs->fc.pan_id_comp = 0;
	fs->fc.reserved = 0;
	/** We support version 2006 only for now */
	fs->fc.seq_num_suppr = 0;
	fs->fc.ie_list = 0;
	fs->fc.frame_version = IEEE802154_VERSION_802154_2006;

	*p_buf += sizeof(struct ieee802154_fcf_seq);

	return fs;
}

static inline enum ieee802154_addressing_mode
get_dst_addr_mode(struct net_if *iface, struct net_buf *buf,
		  struct net_nbr **nbr, bool *broadcast)
{
	if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst) ||
	    net_is_ipv6_addr_unspecified(&NET_IPV6_BUF(buf)->dst)) {
		*broadcast = true;
		*nbr = NULL;

		return IEEE802154_ADDR_MODE_SHORT;
	}

	*broadcast = false;

	/* ToDo: Prefer short addr, when _associated_ to a PAN
	 * if associated:
	 * - Check if dst is known nb and has short addr
	 * - if so, return short.
	 */
	*nbr = net_ipv6_nbr_lookup(iface, &NET_IPV6_BUF(buf)->dst);
	if (!*nbr) {
		return IEEE802154_ADDR_MODE_NONE;
	}

	return IEEE802154_ADDR_MODE_EXTENDED;
}

static inline void
generate_addressing_fields(struct net_if *iface, struct net_buf *buf,
			   struct ieee802154_fcf_seq *fs, uint8_t **p_buf)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_address_field *af;
	struct net_nbr *nbr;
	bool broadcast;

	fs->fc.dst_addr_mode = get_dst_addr_mode(iface, buf, &nbr, &broadcast);
	if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		/* No known destination */
		goto src;
	}

	af = (struct ieee802154_address_field *)*p_buf;
	af->plain.pan_id = ctx->pan_id;

	if (broadcast) { /* We are broadcasting */
		fs->fc.pan_id_comp = 1;
		af->plain.addr.short_addr = IEEE802154_BROADCAST_ADDRESS;
		*p_buf += IEEE802154_PAN_ID_LENGTH +
			IEEE802154_SHORT_ADDR_LENGTH;
	} else {
		struct net_linkaddr_storage *l_addr;

		/**
		 * ToDo:
		 * - handle short address
		 * - handle different dst PAN ID
		 */
		fs->fc.pan_id_comp = 1;

		l_addr = net_nbr_get_lladdr(nbr->idx);
		memcpy(af->comp.addr.ext_addr, l_addr->addr, l_addr->len);

		*p_buf += IEEE802154_PAN_ID_LENGTH +
			IEEE802154_EXT_ADDR_LENGTH;
	}

src:
	if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT && !broadcast) {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_SHORT;
	} else {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
	}

	af = (struct ieee802154_address_field *)*p_buf;

	if (fs->fc.pan_id_comp) {
		memcpy(af->comp.addr.ext_addr,
		       iface->link_addr.addr, IEEE802154_EXT_ADDR_LENGTH);
	} else {
		af->plain.pan_id = ctx->pan_id;
		memcpy(af->plain.addr.ext_addr,
		       iface->link_addr.addr, IEEE802154_EXT_ADDR_LENGTH);
	}
}

bool ieee802154_create_data_frame(struct net_if *iface, struct net_buf *buf)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint8_t *p_buf = net_nbuf_ll(buf);
	struct ieee802154_fcf_seq *fs;

	if (!p_buf) {
		NET_DBG("No room for IEEE 802.15.4 header");
		return false;
	}

	fs = generate_fcf_grounds(&p_buf);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_DATA;
	fs->sequence = ctx->sequence;

	generate_addressing_fields(iface, buf, fs, &p_buf);

	/* Todo: handle security aux header */

	dbg_print_fs(fs);

	return true;
}

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
bool ieee802154_create_ack_frame(struct net_if *iface,
				 struct net_buf *buf, uint8_t seq)
{
	uint8_t *p_buf = net_nbuf_ll(buf);
	struct ieee802154_fcf_seq *fs;

	if (!p_buf) {
		return false;
	}

	fs = generate_fcf_grounds(&p_buf);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_ACK;
	fs->sequence = seq;

	return true;
}
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */
