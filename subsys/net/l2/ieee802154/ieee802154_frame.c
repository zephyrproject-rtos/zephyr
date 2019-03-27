/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_frame, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_if.h>

#include <ipv6.h>
#include <nbr.h>

#include "ieee802154_frame.h"
#include "ieee802154_security.h"

#define dbg_print_fs(fs)						\
	NET_DBG("fs(1): %u/%u/%u/%u/%u/%u",				\
		fs->fc.frame_type, fs->fc.security_enabled,		\
		fs->fc.frame_pending, fs->fc.ar, fs->fc.pan_id_comp,	\
		fs->fc.reserved);					\
	NET_DBG("fs(2): %u/%u/%u/%u/%u - %u",				\
		fs->fc.seq_num_suppr, fs->fc.ie_list,			\
		fs->fc.dst_addr_mode, fs->fc.frame_version,		\
		fs->fc.src_addr_mode, fs->sequence)

#define BUF_TIMEOUT K_MSEC(50)

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
const u8_t level_2_tag_size[4] = {
	0,
	IEEE8021254_AUTH_TAG_LENGTH_32,
	IEEE8021254_AUTH_TAG_LENGTH_64,
	IEEE8021254_AUTH_TAG_LENGTH_128
};
#endif

struct ieee802154_fcf_seq *ieee802154_validate_fc_seq(u8_t *buf, u8_t **p_buf)
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

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON &&
	    (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE ||
	     fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE ||
	     fs->fc.pan_id_comp)) {
		/** See section 5.2.2.1.1 */
		return NULL;
	} else if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_DATA &&
		   fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE &&
		   fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		/** See section 5.2.2.2.1 */
		return NULL;
	} else if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND &&
		   fs->fc.frame_pending) {
		/** See section 5.3 */
		return NULL;
	}

#ifndef CONFIG_NET_L2_IEEE802154_SECURITY
	if (fs->fc.security_enabled) {
		return NULL;
	}
#endif

	if (p_buf) {
		*p_buf = buf + 3;
	}

	return fs;
}

static inline struct ieee802154_address_field *
validate_addr(u8_t *buf, u8_t **p_buf,
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

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
struct ieee802154_aux_security_hdr *
ieee802154_validate_aux_security_hdr(u8_t *buf, u8_t **p_buf)
{
	struct ieee802154_aux_security_hdr *ash =
		(struct ieee802154_aux_security_hdr *)buf;

	*p_buf = buf;

	/* Only implicit key mode is supported for now */
	if (ash->control.key_id_mode != IEEE802154_KEY_ID_MODE_IMPLICIT) {
		return NULL;
	}

	/* At least the asf is sized of: control field + frame counter */
	*p_buf += sizeof(struct ieee802154_security_control_field) +
		sizeof(u32_t);

	/* Explicit key must have a key index != 0x00, see Section 7.4.3.2 */
	switch (ash->control.key_id_mode) {
	case IEEE802154_KEY_ID_MODE_IMPLICIT:
		break;
	case IEEE802154_KEY_ID_MODE_INDEX:
		*p_buf += IEEE8021254_KEY_ID_FIELD_INDEX_LENGTH;

		if (!ash->kif.mode_1.key_index) {
			return NULL;
		}

		break;
	case IEEE802154_KEY_ID_MODE_SRC_4_INDEX:
		*p_buf += IEEE8021254_KEY_ID_FIELD_SRC_4_INDEX_LENGTH;

		if (!ash->kif.mode_2.key_index) {
			return NULL;
		}

		break;
	case IEEE802154_KEY_ID_MODE_SRC_8_INDEX:
		*p_buf += IEEE8021254_KEY_ID_FIELD_SRC_8_INDEX_LENGTH;

		if (!ash->kif.mode_3.key_index) {
			return NULL;
		}

		break;
	}

	return ash;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

static inline bool
validate_beacon(struct ieee802154_mpdu *mpdu, u8_t *buf, u8_t length)
{
	struct ieee802154_beacon *b = (struct ieee802154_beacon *)buf;
	u8_t *p_buf = buf;
	struct ieee802154_pas_spec *pas;

	if (length < IEEE802154_BEACON_MIN_SIZE) {
		return false;
	}

	p_buf += IEEE802154_BEACON_SF_SIZE + IEEE802154_BEACON_GTS_SPEC_SIZE;

	if (b->gts.desc_count) {
		p_buf += IEEE802154_BEACON_GTS_DIR_SIZE +
			b->gts.desc_count * IEEE802154_BEACON_GTS_SIZE;
	}

	if (length < (p_buf - buf)) {
		return false;
	}

	pas = (struct ieee802154_pas_spec *)p_buf;
	p_buf += IEEE802154_BEACON_PAS_SPEC_SIZE;

	if (pas->nb_sap || pas->nb_eap) {
		p_buf += (pas->nb_sap * IEEE802154_SHORT_ADDR_LENGTH) +
			(pas->nb_eap * IEEE802154_EXT_ADDR_LENGTH);
	}

	if (length < (p_buf - buf)) {
		return false;
	}

	mpdu->beacon = b;

	return true;
}

static inline bool
validate_mac_command_cfi_to_mhr(struct ieee802154_mhr *mhr,
				u8_t ar, u8_t comp,
				u8_t src, bool src_pan_brdcst_chk,
				u8_t dst, bool dst_brdcst_chk)
{
	if (mhr->fs->fc.ar != ar || mhr->fs->fc.pan_id_comp != comp) {
		return false;
	}

	if ((mhr->fs->fc.src_addr_mode != src) ||
	    (mhr->fs->fc.dst_addr_mode != dst)) {
		return false;
	}

	/* This should be set only when comp == 0 */
	if (src_pan_brdcst_chk) {
		if (mhr->src_addr->plain.pan_id !=
		    IEEE802154_BROADCAST_PAN_ID) {
			return false;
		}
	}

	/* This should be set only when comp == 0 */
	if (dst_brdcst_chk) {
		if (mhr->dst_addr->plain.addr.short_addr !=
		    IEEE802154_BROADCAST_ADDRESS) {
			return false;
		}
	}

	return true;
}

static inline bool
validate_mac_command(struct ieee802154_mpdu *mpdu, u8_t *buf, u8_t length)
{
	struct ieee802154_command *c = (struct ieee802154_command *)buf;
	bool src_pan_brdcst_chk = false;
	bool dst_brdcst_chk = false;
	u8_t comp = 0U;
	u8_t ar = 0U;
	u8_t src, dst;

	switch (c->cfi) {
	case IEEE802154_CFI_UNKNOWN:
		return false;
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
		src = IEEE802154_EXT_ADDR_LENGTH;
		src_pan_brdcst_chk = true;
		dst = IEEE802154_ADDR_MODE_SHORT |
			IEEE802154_ADDR_MODE_EXTENDED;
		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
	case IEEE802154_CFI_PAN_ID_CONLICT_NOTIFICATION:
		ar = 1U;
		comp = 1U;
		src = IEEE802154_EXT_ADDR_LENGTH;
		dst = IEEE802154_EXT_ADDR_LENGTH;

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		ar = 1U;
		src = IEEE802154_ADDR_MODE_SHORT |
			IEEE802154_ADDR_MODE_EXTENDED;

		if (mpdu->mhr.fs->fc.dst_addr_mode ==
		    IEEE802154_ADDR_MODE_NONE) {
			dst = IEEE802154_ADDR_MODE_NONE;
		} else {
			comp = 1U;
			dst = IEEE802154_ADDR_MODE_SHORT |
				IEEE802154_ADDR_MODE_EXTENDED;
		}

		break;
	case IEEE802154_CFI_ORPHAN_NOTIFICATION:
		comp = 1U;
		src = IEEE802154_EXT_ADDR_LENGTH;
		dst = IEEE802154_ADDR_MODE_SHORT;

		break;
	case IEEE802154_CFI_BEACON_REQUEST:
		src = IEEE802154_ADDR_MODE_NONE;
		dst = IEEE802154_ADDR_MODE_SHORT;
		dst_brdcst_chk = true;

		break;
	case IEEE802154_CFI_COORDINATOR_REALIGNEMENT:
		src = IEEE802154_EXT_ADDR_LENGTH;

		if (mpdu->mhr.fs->fc.dst_addr_mode ==
		    IEEE802154_ADDR_MODE_SHORT) {
			dst = IEEE802154_ADDR_MODE_SHORT;
			dst_brdcst_chk = true;
		} else {
			dst = IEEE802154_ADDR_MODE_EXTENDED;
		}

		break;
	case IEEE802154_CFI_GTS_REQUEST:
		ar = 1U;
		src = IEEE802154_ADDR_MODE_SHORT;
		dst = IEEE802154_ADDR_MODE_NONE;

		break;
	default:
		return false;
	}

	if (!validate_mac_command_cfi_to_mhr(&mpdu->mhr, ar, comp,
					     src, src_pan_brdcst_chk,
					     dst, dst_brdcst_chk)) {
		return false;
	}

	mpdu->command = c;

	return true;
}

static inline bool
validate_payload_and_mfr(struct ieee802154_mpdu *mpdu,
			 u8_t *buf, u8_t *p_buf, u8_t length)
{
	u8_t type = mpdu->mhr.fs->fc.frame_type;
	u8_t payload_length;

	payload_length = length - (p_buf - buf);

	NET_DBG("Header size: %u, vs total length %u: payload size %u",
		(u32_t)(p_buf - buf), length, payload_length);

	if (type == IEEE802154_FRAME_TYPE_BEACON) {
		if (!validate_beacon(mpdu, p_buf, payload_length)) {
			return false;
		}
	} else if (type == IEEE802154_FRAME_TYPE_DATA) {
		 /** A data frame embeds a payload */
		if (payload_length == 0U) {
			return false;
		}

		mpdu->payload = (void *)p_buf;
	} else if (type == IEEE802154_FRAME_TYPE_ACK) {
		/** An ACK frame has no payload */
		if (payload_length) {
			return false;
		}

		mpdu->payload = NULL;
	} else {
		if (!validate_mac_command(mpdu, p_buf, payload_length)) {
			return false;
		}
	}

	mpdu->mfr = (struct ieee802154_mfr *)(p_buf + payload_length);

	return true;
}

bool ieee802154_validate_frame(u8_t *buf, u8_t length,
			       struct ieee802154_mpdu *mpdu)
{
	u8_t *p_buf = NULL;

	if (length > IEEE802154_MTU || length < IEEE802154_MIN_LENGTH) {
		NET_DBG("Wrong packet length: %d", length);
		return false;
	}

	mpdu->mhr.fs = ieee802154_validate_fc_seq(buf, &p_buf);
	if (!mpdu->mhr.fs) {
		return false;
	}

	/* ToDo: Support later version's frame types */
	if (mpdu->mhr.fs->fc.frame_type > IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		return false;
	}

	mpdu->mhr.dst_addr = validate_addr(p_buf, &p_buf,
					   mpdu->mhr.fs->fc.dst_addr_mode,
					   false);

	mpdu->mhr.src_addr = validate_addr(p_buf, &p_buf,
					   mpdu->mhr.fs->fc.src_addr_mode,
					   (mpdu->mhr.fs->fc.pan_id_comp));

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (mpdu->mhr.fs->fc.security_enabled) {
		mpdu->mhr.aux_sec =
			ieee802154_validate_aux_security_hdr(p_buf, &p_buf);
		if (!mpdu->mhr.aux_sec) {
			return false;
		}
	}
#endif

	return validate_payload_and_mfr(mpdu, buf, p_buf, length);
}

u8_t ieee802154_compute_header_size(struct net_if *iface,
				    struct in6_addr *dst)
{
	u8_t hdr_len = sizeof(struct ieee802154_fcf_seq);
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx *sec_ctx =
		&((struct ieee802154_context *)net_if_l2_data(iface))->sec_ctx;
#endif

	/** if dst is NULL, we'll consider it as a brodcast header */
	if (!dst ||
	    net_ipv6_is_addr_mcast(dst) ||
	    net_ipv6_is_addr_unspecified(dst)) {
		NET_DBG("Broadcast destination");
		/* 4 dst pan/addr + 8 src addr */
		hdr_len += IEEE802154_PAN_ID_LENGTH +
			IEEE802154_SHORT_ADDR_LENGTH +
			IEEE802154_EXT_ADDR_LENGTH;
		if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_SECURITY)) {
			NET_DBG("Broadcast packet do not have security");
			goto done;
		}
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

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		goto done;
	}

	/* Compute aux-sec hdr size and add it to hdr_len */
	hdr_len += IEEE802154_SECURITY_CF_LENGTH +
		IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;

	switch (sec_ctx->key_mode) {
	case IEEE802154_KEY_ID_MODE_IMPLICIT:
		/* The only mode supported for now,
		 * generate_aux_securiy_hdr() will fail on other modes
		 */
		break;
	case IEEE802154_KEY_ID_MODE_INDEX:
		hdr_len += IEEE8021254_KEY_ID_FIELD_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_4_INDEX:
		hdr_len += IEEE8021254_KEY_ID_FIELD_SRC_4_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_8_INDEX:
		hdr_len += IEEE8021254_KEY_ID_FIELD_SRC_8_INDEX_LENGTH;
	}

	/* This is a _HACK_: as net_buf do not let the possibility to
	 * reserve tailroom - here for authentication tag - it "reserves"
	 * it in headroom so the payload won't occupy all the left space
	 * and then when it will come to finalize the data frame it will
	 * reduce the reserved space by the tag size, move the payload
	 * backward accordingly, and only then: run the
	 * encryption/authentication which will fill the tag space in the end.
	 */
	if (sec_ctx->level < IEEE802154_SECURITY_LEVEL_ENC) {
		hdr_len += level_2_tag_size[sec_ctx->level];
	} else {
		hdr_len += level_2_tag_size[sec_ctx->level - 4];
	}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

done:
	NET_DBG("Computed size of %u", hdr_len);

	return hdr_len;
}

static inline struct ieee802154_fcf_seq *generate_fcf_grounds(u8_t **p_buf,
							      bool ack)
{
	struct ieee802154_fcf_seq *fs;

	fs = (struct ieee802154_fcf_seq *) *p_buf;

	fs->fc.security_enabled = 0U;
	fs->fc.frame_pending = 0U;
	fs->fc.ar = ack;
	fs->fc.pan_id_comp = 0U;
	fs->fc.reserved = 0U;
	/** We support version 2006 only for now */
	fs->fc.seq_num_suppr = 0U;
	fs->fc.ie_list = 0U;
	fs->fc.frame_version = IEEE802154_VERSION_802154_2006;

	*p_buf += sizeof(struct ieee802154_fcf_seq);

	return fs;
}

static inline enum ieee802154_addressing_mode
get_dst_addr_mode(struct net_linkaddr *dst, bool *broadcast)
{
	if (!dst->addr) {
		NET_DBG("Broadcast destination");

		*broadcast = true;

		return IEEE802154_ADDR_MODE_SHORT;
	}

	*broadcast = false;

	if (dst->len == IEEE802154_SHORT_ADDR_LENGTH) {
		return IEEE802154_ADDR_MODE_SHORT;
	}

	if (dst->len == IEEE802154_EXT_ADDR_LENGTH) {
		return IEEE802154_ADDR_MODE_EXTENDED;
	}

	return IEEE802154_ADDR_MODE_NONE;
}

static inline
bool data_addr_to_fs_settings(struct net_linkaddr *dst,
			      struct ieee802154_fcf_seq *fs,
			      struct ieee802154_frame_params *params)
{
	bool broadcast;

	fs->fc.dst_addr_mode = get_dst_addr_mode(dst, &broadcast);
	if (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		fs->fc.pan_id_comp = 1U;

		if (broadcast) {
			params->dst.short_addr = IEEE802154_BROADCAST_ADDRESS;
			params->dst.len = IEEE802154_SHORT_ADDR_LENGTH;
			fs->fc.ar = 0U;
		} else {
			params->dst.ext_addr = dst->addr;
			params->dst.len = dst->len;
		}
	}

	if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT && !broadcast) {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_SHORT;
	} else {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
	}

	return broadcast;
}

static
u8_t *generate_addressing_fields(struct ieee802154_context *ctx,
				 struct ieee802154_fcf_seq *fs,
				 struct ieee802154_frame_params *params,
				 u8_t *p_buf)
{
	struct ieee802154_address_field *af;
	struct ieee802154_address *src_addr;

	if (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		af = (struct ieee802154_address_field *)p_buf;
		af->plain.pan_id = params->dst.pan_id;

		if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
			af->plain.addr.short_addr =
				sys_cpu_to_le16(params->dst.short_addr);
			p_buf += IEEE802154_PAN_ID_LENGTH +
				IEEE802154_SHORT_ADDR_LENGTH;
		} else {
			sys_memcpy_swap(af->plain.addr.ext_addr,
					params->dst.ext_addr,
					IEEE802154_EXT_ADDR_LENGTH);
			p_buf += IEEE802154_PAN_ID_LENGTH +
				IEEE802154_EXT_ADDR_LENGTH;
		}
	}

	if (fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		return p_buf;
	}

	af = (struct ieee802154_address_field *)p_buf;

	if (!fs->fc.pan_id_comp) {
		af->plain.pan_id = params->pan_id;
		src_addr = &af->plain.addr;
		p_buf += IEEE802154_PAN_ID_LENGTH;
	} else {
		src_addr = &af->comp.addr;
	}

	if (fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		src_addr->short_addr = sys_cpu_to_le16(params->short_addr);
		p_buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else {
		memcpy(src_addr->ext_addr, ctx->ext_addr,
		       IEEE802154_EXT_ADDR_LENGTH);
		p_buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	return p_buf;
}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
static
u8_t *generate_aux_security_hdr(struct ieee802154_security_ctx *sec_ctx,
				u8_t *p_buf)
{
	struct ieee802154_aux_security_hdr *aux_sec;

	if (sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		return p_buf;
	}

	if (sec_ctx->key_mode != IEEE802154_KEY_ID_MODE_IMPLICIT) {
		/* ToDo: it supports implicit mode only, for now */
		return NULL;
	}

	aux_sec = (struct ieee802154_aux_security_hdr *)p_buf;

	aux_sec->control.security_level = sec_ctx->level;
	aux_sec->control.key_id_mode = sec_ctx->key_mode;
	aux_sec->control.reserved = 0U;

	aux_sec->frame_counter = sys_cpu_to_le32(sec_ctx->frame_counter);

	return p_buf + IEEE802154_SECURITY_CF_LENGTH +
		IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

bool ieee802154_create_data_frame(struct ieee802154_context *ctx,
				  struct net_linkaddr *dst,
				  struct net_buf *buf,
				  u8_t hdr_size)
{
	struct ieee802154_frame_params params;
	struct ieee802154_fcf_seq *fs;
	u8_t *p_buf = buf->data;
	u8_t *buf_start = p_buf;
	bool broadcast;

	fs = generate_fcf_grounds(&p_buf, ctx->ack_requested);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_DATA;
	fs->sequence = ctx->sequence++;

	params.dst.pan_id = ctx->pan_id;
	params.pan_id = ctx->pan_id;

	broadcast = data_addr_to_fs_settings(dst, fs, &params);

	p_buf = generate_addressing_fields(ctx, fs, &params, p_buf);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (broadcast) {
		NET_DBG("No security hdr needed: broadcasting");

		goto no_security_hdr;
	}

	fs->fc.security_enabled = 1U;

	p_buf = generate_aux_security_hdr(&ctx->sec_ctx, p_buf);

	/* If tagged, let's retrieve tag space from hdr reserved space.
	 * See comment in ieee802154_compute_header_size()
	 */
	if (ctx->sec_ctx.level != IEEE802154_SECURITY_LEVEL_NONE &&
	    ctx->sec_ctx.level != IEEE802154_SECURITY_LEVEL_ENC) {
		u8_t level;

		level = ctx->sec_ctx.level;
		if (level >= IEEE802154_SECURITY_LEVEL_ENC) {
			level -= 4U;
		}

		/* p_buf should point to the right place */
		memmove(p_buf, buf->data, buf->len);
		hdr_size -= level_2_tag_size[level];
	}

no_security_hdr:
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	if ((p_buf - buf_start) != hdr_size) {
		/* hdr_size was too small? We probably overwrote
		 * payload bytes
		 */
		NET_ERR("Could not generate data frame %zu vs %u",
			(p_buf - buf_start), hdr_size);
		return false;
	}

	dbg_print_fs(fs);

	/* Let's encrypt/auth only in the end, is needed */
	return ieee802154_encrypt_auth(broadcast ? NULL : &ctx->sec_ctx,
				       buf_start, hdr_size, buf->len,
				       ctx->ext_addr);
}

#ifdef CONFIG_NET_L2_IEEE802154_RFD

static inline bool cfi_to_fs_settings(enum ieee802154_cfi cfi,
				      struct ieee802154_fcf_seq *fs,
				      struct ieee802154_frame_params *params)
{
	switch (cfi) {
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
		fs->fc.ar = 1U;
		fs->fc.pan_id_comp = 1U;

		/* Fall through for common src/dst addr mode handling */
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;

		if (params->dst.len == IEEE802154_SHORT_ADDR_LENGTH) {
			fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_SHORT;
		} else {
			fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		}

		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
	case IEEE802154_CFI_PAN_ID_CONLICT_NOTIFICATION:
		fs->fc.ar = 1U;
		fs->fc.pan_id_comp = 1U;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		fs->fc.ar = 1U;
		/* ToDo: src/dst addr mode: see 5.3.4 */

		break;
	case IEEE802154_CFI_ORPHAN_NOTIFICATION:
		fs->fc.pan_id_comp = 1U;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_SHORT;

		break;
	case IEEE802154_CFI_BEACON_REQUEST:
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_NONE;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_SHORT;
		break;
	case IEEE802154_CFI_COORDINATOR_REALIGNEMENT:
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		/* Todo: ar and dst addr mode: see 5.3.8 */

		break;
	case IEEE802154_CFI_GTS_REQUEST:
		fs->fc.ar = 1U;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_SHORT;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_NONE;

		break;
	default:
		return false;
	}

	return true;
}

static inline u8_t mac_command_length(enum ieee802154_cfi cfi)
{
	u8_t length = 1U; /* cfi is at least present */

	switch (cfi) {
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
	case IEEE802154_CFI_GTS_REQUEST:
		length += 1U;
		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
		length += 3U;
		break;
	case IEEE802154_CFI_COORDINATOR_REALIGNEMENT:
		length += 8U;
		break;
	default:
		break;
	}

	return length;
}

struct net_pkt *
ieee802154_create_mac_cmd_frame(struct net_if *iface,
				enum ieee802154_cfi type,
				struct ieee802154_frame_params *params)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_fcf_seq *fs;
	struct net_pkt *pkt;
	u8_t *p_buf, *p_start;

	/* It would be costly to compute the size when actual frame are never
	 * bigger than 125 bytes, so let's allocate that size as buffer.
	 */
	pkt = net_pkt_alloc_with_buffer(iface,
					IEEE802154_MTU - IEEE802154_MFR_LENGTH,
					AF_UNSPEC, 0, BUF_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	p_buf = net_pkt_data(pkt);
	p_start = p_buf;

	fs = generate_fcf_grounds(&p_buf,
				  type == IEEE802154_CFI_BEACON_REQUEST ?
				  false : ctx->ack_requested);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_MAC_COMMAND;
	fs->sequence = ctx->sequence;

	if (!cfi_to_fs_settings(type, fs, params)) {
		goto error;
	}

	p_buf = generate_addressing_fields(ctx, fs, params, p_buf);

	net_buf_add(pkt->buffer, p_buf - p_start);

	/* Let's insert the cfi */
	((struct ieee802154_command *)p_buf)->cfi = type;

	dbg_print_fs(fs);

	return pkt;
error:
	net_pkt_unref(pkt);

	return NULL;
}

void ieee802154_mac_cmd_finalize(struct net_pkt *pkt,
				 enum ieee802154_cfi type)
{
	net_buf_add(pkt->buffer, mac_command_length(type));
}

#endif /* CONFIG_NET_L2_IEEE802154_RFD */

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
bool ieee802154_create_ack_frame(struct net_if *iface,
				 struct net_pkt *pkt, u8_t seq)
{
	u8_t *p_buf = net_pkt_data(pkt);
	struct ieee802154_fcf_seq *fs;

	if (!p_buf) {
		return false;
	}

	fs = generate_fcf_grounds(&p_buf, false);

	fs->fc.dst_addr_mode = 0U;
	fs->fc.src_addr_mode = 0U;

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_ACK;
	fs->sequence = seq;

	net_buf_add(pkt->buffer, IEEE802154_ACK_PKT_LENGTH);

	return true;
}
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
bool ieee802154_decipher_data_frame(struct net_if *iface, struct net_pkt *pkt,
				    struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	u8_t level;

	if (!mpdu->mhr.fs->fc.security_enabled) {
		return true;
	}

	/* Section 7.2.3 (i) talks about "security level policy" conformance
	 * but such policy does not seem to be detailed. So let's assume both
	 * ends should have same security level.
	 */
	if (mpdu->mhr.aux_sec->control.security_level != ctx->sec_ctx.level) {
		return false;
	}

	/* ToDo: handle src short address
	 * This will require to look up in nbr cache with short addr
	 * in order to get the extended address related to it
	 */
	if (!ieee802154_decrypt_auth(&ctx->sec_ctx, net_pkt_data(pkt),
				     (u8_t *)mpdu->payload - net_pkt_data(pkt),
				     net_pkt_get_len(pkt),
				     net_pkt_lladdr_src(pkt)->addr,
				     sys_le32_to_cpu(
					mpdu->mhr.aux_sec->frame_counter))) {
		NET_ERR("Could not decipher the frame");
		return false;
	}

	level = ctx->sec_ctx.level;
	if (level >= IEEE802154_SECURITY_LEVEL_ENC) {
		level -= 4U;
	}

	/* We remove tag size from buf's length, it is now useless */
	pkt->buffer->len -= level_2_tag_size[level];

	return true;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
