/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC frame related functions implementation
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_frame, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include "ieee802154_frame.h"
#include "ieee802154_security.h"

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>

#include <ipv6.h>
#include <nbr.h>

#define dbg_print_fs(fs)                                                                           \
	NET_DBG("fs(1): %u/%u/%u/%u/%u/%u", fs->fc.frame_type, fs->fc.security_enabled,            \
		fs->fc.frame_pending, fs->fc.ar, fs->fc.pan_id_comp, fs->fc.reserved);             \
	NET_DBG("fs(2): %u/%u/%u/%u/%u - %u", fs->fc.seq_num_suppr, fs->fc.ie_list,                \
		fs->fc.dst_addr_mode, fs->fc.frame_version, fs->fc.src_addr_mode, fs->sequence)

#define BUF_TIMEOUT K_MSEC(50)

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
const uint8_t level_2_authtag_len[4] = {0, IEEE802154_AUTH_TAG_LENGTH_32,
					IEEE802154_AUTH_TAG_LENGTH_64,
					IEEE802154_AUTH_TAG_LENGTH_128};
#endif

struct ieee802154_fcf_seq *ieee802154_validate_fc_seq(uint8_t *buf, uint8_t **p_buf,
						      uint8_t *length)
{
	struct ieee802154_fcf_seq *fs = (struct ieee802154_fcf_seq *)buf;

	dbg_print_fs(fs);

	/** Basic FC checks */
	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_RESERVED ||
	    fs->fc.frame_version >= IEEE802154_VERSION_RESERVED) {
		return NULL;
	}

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_MULTIPURPOSE) {
		if (fs->fc.frame_version != 0) {
			return NULL;
		}
	} else {
		/** Only for versions 2003/2006 */
		if (fs->fc.frame_version < IEEE802154_VERSION_802154 &&
		    (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_RESERVED ||
		     fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_RESERVED ||
		     fs->fc.frame_type >= IEEE802154_FRAME_TYPE_RESERVED)) {
			return NULL;
		}
	}

	if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_BEACON &&
	    (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE ||
	     fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE || fs->fc.pan_id_comp)) {
		/** See section 7.2.2.1.1 */
		return NULL;
	} else if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_DATA &&
		   fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE &&
		   fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		/** See section 7.2.2.2.1 */
		return NULL;
	} else if (fs->fc.frame_type == IEEE802154_FRAME_TYPE_MAC_COMMAND && fs->fc.frame_pending) {
		/** See section 7.3 */
		return NULL;
	}

#ifndef CONFIG_NET_L2_IEEE802154_SECURITY
	if (fs->fc.security_enabled) {
		return NULL;
	}
#endif

	if (p_buf) {
		*length -= IEEE802154_FCF_SEQ_LENGTH;
		*p_buf = buf + IEEE802154_FCF_SEQ_LENGTH;
	}

	return fs;
}

static inline bool validate_addr(uint8_t *buf, uint8_t **p_buf, uint8_t *length,
				 enum ieee802154_addressing_mode mode, bool pan_id_compression,
				 struct ieee802154_address_field **addr)
{
	uint8_t len = 0;

	*p_buf = buf;

	NET_DBG("Buf %p - mode %d - pan id comp %d", (void *)buf, mode, pan_id_compression);

	if (mode == IEEE802154_ADDR_MODE_NONE) {
		*addr = NULL;
		return true;
	}

	if (!pan_id_compression) {
		len = IEEE802154_PAN_ID_LENGTH;
	}

	if (mode == IEEE802154_ADDR_MODE_SHORT) {
		len += IEEE802154_SHORT_ADDR_LENGTH;
	} else {
		/* IEEE802154_ADDR_MODE_EXTENDED */
		len += IEEE802154_EXT_ADDR_LENGTH;
	}

	if (len > *length) {
		return false;
	}

	*p_buf += len;
	*length -= len;

	*addr = (struct ieee802154_address_field *)buf;

	return true;
}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
struct ieee802154_aux_security_hdr *
ieee802154_validate_aux_security_hdr(uint8_t *buf, uint8_t **p_buf, uint8_t *length)
{
	struct ieee802154_aux_security_hdr *ash = (struct ieee802154_aux_security_hdr *)buf;
	uint8_t len = IEEE802154_SECURITY_CF_LENGTH + IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;

	/* At least the asf is sized of: control field + frame counter */
	if (*length < len) {
		return NULL;
	}

	/* Only implicit key mode is supported for now */
	if (ash->control.key_id_mode != IEEE802154_KEY_ID_MODE_IMPLICIT) {
		return NULL;
	}

	/* Explicit key must have a key index != 0x00, see section 9.4.2.3 */
	switch (ash->control.key_id_mode) {
	case IEEE802154_KEY_ID_MODE_IMPLICIT:
		break;
	case IEEE802154_KEY_ID_MODE_INDEX:
		len += IEEE802154_KEY_ID_FIELD_INDEX_LENGTH;
		if (*length < len) {
			return NULL;
		}

		if (!ash->kif.mode_1.key_index) {
			return NULL;
		}

		break;
	case IEEE802154_KEY_ID_MODE_SRC_4_INDEX:
		len += IEEE802154_KEY_ID_FIELD_SRC_4_INDEX_LENGTH;
		if (*length < len) {
			return NULL;
		}

		if (!ash->kif.mode_2.key_index) {
			return NULL;
		}

		break;
	case IEEE802154_KEY_ID_MODE_SRC_8_INDEX:
		len += IEEE802154_KEY_ID_FIELD_SRC_8_INDEX_LENGTH;
		if (*length < len) {
			return NULL;
		}

		if (!ash->kif.mode_3.key_index) {
			return NULL;
		}

		break;
	}

	*p_buf = buf + len;
	*length -= len;

	return ash;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

static inline bool validate_beacon(struct ieee802154_mpdu *mpdu, uint8_t *buf, uint8_t length)
{
	struct ieee802154_beacon *beacon = (struct ieee802154_beacon *)buf;
	struct ieee802154_pas_spec *pas;
	uint8_t len = IEEE802154_BEACON_SF_SIZE + IEEE802154_BEACON_GTS_SPEC_SIZE;

	if (length < len) {
		return false;
	}

	if (beacon->gts.desc_count) {
		len += IEEE802154_BEACON_GTS_DIR_SIZE +
		       beacon->gts.desc_count * IEEE802154_BEACON_GTS_SIZE;
	}

	if (length < len) {
		return false;
	}

	pas = (struct ieee802154_pas_spec *)buf + len;

	len += IEEE802154_BEACON_PAS_SPEC_SIZE;
	if (length < len) {
		return false;
	}

	if (pas->nb_sap || pas->nb_eap) {
		len += (pas->nb_sap * IEEE802154_SHORT_ADDR_LENGTH) +
		       (pas->nb_eap * IEEE802154_EXT_ADDR_LENGTH);
	}

	if (length < len) {
		return false;
	}

	mpdu->beacon = beacon;

	return true;
}

static inline bool validate_mac_command_cfi_to_mhr(struct ieee802154_mhr *mhr,
						   bool ack_requested, bool has_pan_id,
						   uint8_t src_bf, bool src_pan_brdcst_chk,
						   uint8_t dst_bf, bool dst_brdcst_chk)
{
	if (mhr->fs->fc.ar != ack_requested || mhr->fs->fc.pan_id_comp == has_pan_id) {
		return false;
	}

	if (!(BIT(mhr->fs->fc.src_addr_mode) & src_bf) ||
	    !(BIT(mhr->fs->fc.dst_addr_mode) & dst_bf)) {
		return false;
	}

	if (src_pan_brdcst_chk) {
		if (mhr->src_addr->plain.pan_id != IEEE802154_BROADCAST_PAN_ID) {
			return false;
		}
	}

	if (dst_brdcst_chk) {
		/* broadcast address is symmetric so no need to swap byte order */
		if (mhr->dst_addr->plain.addr.short_addr != IEEE802154_BROADCAST_ADDRESS) {
			return false;
		}
	}

	return true;
}

static inline bool validate_mac_command(struct ieee802154_mpdu *mpdu, uint8_t *buf, uint8_t length)
{
	struct ieee802154_command *command = (struct ieee802154_command *)buf;
	uint8_t len = IEEE802154_CMD_CFI_LENGTH;
	bool src_pan_brdcst_chk = false;
	uint8_t src_bf = 0, dst_bf = 0;
	bool dst_brdcst_chk = false;
	bool ack_requested = false;
	bool has_pan_id = true;

	if (length < len) {
		return false;
	}

	switch (command->cfi) {
	case IEEE802154_CFI_UNKNOWN:
		return false;
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
		len += IEEE802154_CMD_ASSOC_REQ_LENGTH;
		ack_requested = true;
		src_bf = BIT(IEEE802154_ADDR_MODE_EXTENDED);
		src_pan_brdcst_chk = true;
		dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT) | BIT(IEEE802154_ADDR_MODE_EXTENDED);

		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
		len += IEEE802154_CMD_ASSOC_RES_LENGTH;
		__fallthrough;
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
		if (command->cfi == IEEE802154_CFI_DISASSOCIATION_NOTIFICATION) {
			len += IEEE802154_CMD_DISASSOC_NOTE_LENGTH;
			dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT);
		}
		__fallthrough;
	case IEEE802154_CFI_PAN_ID_CONFLICT_NOTIFICATION:
		ack_requested = true;
		has_pan_id = false;
		src_bf = BIT(IEEE802154_ADDR_MODE_EXTENDED);
		dst_bf |= BIT(IEEE802154_ADDR_MODE_EXTENDED);

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		ack_requested = true;
		src_bf = BIT(IEEE802154_ADDR_MODE_SHORT) | BIT(IEEE802154_ADDR_MODE_EXTENDED);

		if (mpdu->mhr.fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_NONE) {
			dst_bf = BIT(IEEE802154_ADDR_MODE_NONE);
		} else {
			has_pan_id = false;
			dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT) |
				 BIT(IEEE802154_ADDR_MODE_EXTENDED);
		}

		break;
	case IEEE802154_CFI_ORPHAN_NOTIFICATION:
		has_pan_id = false;
		src_bf = BIT(IEEE802154_ADDR_MODE_EXTENDED);
		dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT);

		break;
	case IEEE802154_CFI_BEACON_REQUEST:
		src_bf = BIT(IEEE802154_ADDR_MODE_NONE);
		dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT);
		dst_brdcst_chk = true;

		break;
	case IEEE802154_CFI_COORDINATOR_REALIGNEMENT:
		len += IEEE802154_CMD_COORD_REALIGN_LENGTH;
		src_bf = BIT(IEEE802154_ADDR_MODE_EXTENDED);

		if (mpdu->mhr.fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
			dst_bf = BIT(IEEE802154_ADDR_MODE_SHORT);
			dst_brdcst_chk = true;
		} else {
			dst_bf = BIT(IEEE802154_ADDR_MODE_EXTENDED);
		}

		break;
	case IEEE802154_CFI_GTS_REQUEST:
		len += IEEE802154_GTS_REQUEST_LENGTH;
		ack_requested = true;
		src_bf = BIT(IEEE802154_ADDR_MODE_SHORT);
		dst_bf = BIT(IEEE802154_ADDR_MODE_NONE);

		break;
	default:
		return false;
	}

	if (length < len) {
		return false;
	}

	if (!validate_mac_command_cfi_to_mhr(&mpdu->mhr, ack_requested, has_pan_id, src_bf,
					     src_pan_brdcst_chk, dst_bf,
					     dst_brdcst_chk)) {
		return false;
	}

	mpdu->command = command;

	return true;
}

static inline bool validate_payload_and_mfr(struct ieee802154_mpdu *mpdu, uint8_t *buf,
					    uint8_t *p_buf, uint8_t length)
{
	uint8_t type = mpdu->mhr.fs->fc.frame_type;

	NET_DBG("Header size: %u, payload size %u", (uint32_t)(p_buf - buf), length);

	if (type == IEEE802154_FRAME_TYPE_BEACON) {
		if (!validate_beacon(mpdu, p_buf, length)) {
			return false;
		}
	} else if (type == IEEE802154_FRAME_TYPE_DATA) {
		/** A data frame embeds a payload */
		if (length == 0U) {
			return false;
		}
	} else if (type == IEEE802154_FRAME_TYPE_ACK) {
		/** An ACK frame has no payload */
		if (length) {
			return false;
		}
	} else {
		if (!validate_mac_command(mpdu, p_buf, length)) {
			return false;
		}
	}

	mpdu->payload_length = length;

	if (length) {
		mpdu->payload = (void *)p_buf;
	} else {
		mpdu->payload = NULL;
	}

	return true;
}

bool ieee802154_validate_frame(uint8_t *buf, uint8_t length, struct ieee802154_mpdu *mpdu)
{
	uint8_t *p_buf = NULL;

	if (length > IEEE802154_MTU || length < IEEE802154_MIN_LENGTH) {
		NET_DBG("Wrong packet length: %d", length);
		return false;
	}

	mpdu->mhr.fs = ieee802154_validate_fc_seq(buf, &p_buf, &length);
	if (!mpdu->mhr.fs) {
		return false;
	}

	/* TODO: Support later version's frame types */
	if (mpdu->mhr.fs->fc.frame_type > IEEE802154_FRAME_TYPE_MAC_COMMAND) {
		return false;
	}

	if (!validate_addr(p_buf, &p_buf, &length, mpdu->mhr.fs->fc.dst_addr_mode, false,
			   &mpdu->mhr.dst_addr) ||
	    !validate_addr(p_buf, &p_buf, &length, mpdu->mhr.fs->fc.src_addr_mode,
			   (mpdu->mhr.fs->fc.pan_id_comp), &mpdu->mhr.src_addr)) {
		return false;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (mpdu->mhr.fs->fc.security_enabled) {
		mpdu->mhr.aux_sec = ieee802154_validate_aux_security_hdr(p_buf, &p_buf, &length);
		if (!mpdu->mhr.aux_sec) {
			return false;
		}
	}
#endif

	return validate_payload_and_mfr(mpdu, buf, p_buf, length);
}

void ieee802154_compute_header_and_authtag_len(struct net_if *iface, struct net_linkaddr *dst,
					       struct net_linkaddr *src, uint8_t *ll_hdr_len,
					       uint8_t *authtag_len)
{
	uint8_t hdr_len = sizeof(struct ieee802154_fcf_seq), tag_len = 0;
	bool broadcast = !dst->addr;

	/* PAN ID */
	hdr_len += IEEE802154_PAN_ID_LENGTH;

	/* Destination Address - see get_dst_addr_mode() */
	hdr_len += broadcast ? IEEE802154_SHORT_ADDR_LENGTH : dst->len;

	/* Source Address - see data_addr_to_fs_settings() */
	hdr_len += src->addr ? src->len : dst->len;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx *sec_ctx;
	struct ieee802154_context *ctx;

	if (broadcast) {
		NET_DBG("Broadcast packets are not being encrypted.");
		goto done;
	}

	ctx = (struct ieee802154_context *)net_if_l2_data(iface);

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	sec_ctx = &ctx->sec_ctx;
	if (sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		goto release;
	}

	/* Compute aux-sec hdr size and add it to hdr_len */
	hdr_len += IEEE802154_SECURITY_CF_LENGTH + IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;

	switch (sec_ctx->key_mode) {
	case IEEE802154_KEY_ID_MODE_IMPLICIT:
		/* The only mode supported for now,
		 * generate_aux_securiy_hdr() will fail on other modes
		 */
		break;
	case IEEE802154_KEY_ID_MODE_INDEX:
		hdr_len += IEEE802154_KEY_ID_FIELD_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_4_INDEX:
		hdr_len += IEEE802154_KEY_ID_FIELD_SRC_4_INDEX_LENGTH;
		break;
	case IEEE802154_KEY_ID_MODE_SRC_8_INDEX:
		hdr_len += IEEE802154_KEY_ID_FIELD_SRC_8_INDEX_LENGTH;
	}

	if (sec_ctx->level < IEEE802154_SECURITY_LEVEL_ENC) {
		tag_len = level_2_authtag_len[sec_ctx->level];
	} else {
		tag_len = level_2_authtag_len[sec_ctx->level - 4U];
	}

release:
	k_sem_give(&ctx->ctx_lock);
done:
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	NET_DBG("Computed header size %u", hdr_len);
	NET_DBG("Computed authtag size: %u", tag_len);

	*ll_hdr_len = hdr_len;
	*authtag_len = tag_len;
}

static inline struct ieee802154_fcf_seq *generate_fcf_grounds(uint8_t **p_buf, bool ack_requested)
{
	struct ieee802154_fcf_seq *fs;

	fs = (struct ieee802154_fcf_seq *)*p_buf;

	fs->fc.security_enabled = 0U;
	fs->fc.frame_pending = 0U;
	fs->fc.ar = ack_requested;
	fs->fc.pan_id_comp = 0U;
	fs->fc.reserved = 0U;
	/* We support version 2006 only for now */
	fs->fc.seq_num_suppr = 0U;
	fs->fc.ie_list = 0U;
	fs->fc.frame_version = IEEE802154_VERSION_802154_2006;

	*p_buf += sizeof(struct ieee802154_fcf_seq);

	return fs;
}

static inline enum ieee802154_addressing_mode get_dst_addr_mode(struct net_linkaddr *dst,
								bool *broadcast)
{
	if (!dst->addr) {
		NET_DBG("Broadcast destination");
		*broadcast = true;
		return IEEE802154_ADDR_MODE_SHORT;
	}

	if (dst->len == IEEE802154_SHORT_ADDR_LENGTH) {
		uint16_t short_addr = ntohs(*(uint16_t *)(dst->addr));
		*broadcast = (short_addr == IEEE802154_BROADCAST_ADDRESS);
		return IEEE802154_ADDR_MODE_SHORT;
	} else {
		*broadcast = false;
	}

	if (dst->len == IEEE802154_EXT_ADDR_LENGTH) {
		return IEEE802154_ADDR_MODE_EXTENDED;
	}

	return IEEE802154_ADDR_MODE_NONE;
}

static inline bool data_addr_to_fs_settings(struct net_linkaddr *dst, struct ieee802154_fcf_seq *fs,
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
		} else if (dst->len == IEEE802154_SHORT_ADDR_LENGTH) {
			params->dst.short_addr = ntohs(*(uint16_t *)(dst->addr));
			params->dst.len = IEEE802154_SHORT_ADDR_LENGTH;
		} else {
			__ASSERT_NO_MSG(dst->len == IEEE802154_EXT_ADDR_LENGTH);
			memcpy(params->dst.ext_addr, dst->addr, sizeof(params->dst.ext_addr));
			params->dst.len = IEEE802154_EXT_ADDR_LENGTH;
		}
	}

	if (params->short_addr) {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_SHORT;
	} else {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
	}

	return broadcast;
}

static uint8_t *generate_addressing_fields(struct ieee802154_context *ctx,
					   struct ieee802154_fcf_seq *fs,
					   struct ieee802154_frame_params *params, uint8_t *p_buf)
{
	struct ieee802154_address_field *address_field;
	struct ieee802154_address *src_addr;

	/* destination address */
	if (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		address_field = (struct ieee802154_address_field *)p_buf;

		address_field->plain.pan_id = sys_cpu_to_le16(params->dst.pan_id);
		p_buf += IEEE802154_PAN_ID_LENGTH;

		if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
			address_field->plain.addr.short_addr =
				sys_cpu_to_le16(params->dst.short_addr);
			p_buf += IEEE802154_SHORT_ADDR_LENGTH;
		} else {
			sys_memcpy_swap(address_field->plain.addr.ext_addr, params->dst.ext_addr,
					IEEE802154_EXT_ADDR_LENGTH);
			p_buf += IEEE802154_EXT_ADDR_LENGTH;
		}
	}

	/* source address */
	if (fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_NONE) {
		return p_buf;
	}

	address_field = (struct ieee802154_address_field *)p_buf;

	if (fs->fc.pan_id_comp) {
		src_addr = &address_field->comp.addr;
	} else {
		address_field->plain.pan_id = sys_cpu_to_le16(params->pan_id);
		src_addr = &address_field->plain.addr;
		p_buf += IEEE802154_PAN_ID_LENGTH;
	}

	if (fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		src_addr->short_addr = sys_cpu_to_le16(params->short_addr);
		p_buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else {
		memcpy(src_addr->ext_addr, ctx->ext_addr, IEEE802154_EXT_ADDR_LENGTH);
		p_buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	return p_buf;
}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
static uint8_t *generate_aux_security_hdr(struct ieee802154_security_ctx *sec_ctx, uint8_t *p_buf)
{
	struct ieee802154_aux_security_hdr *aux_sec;

	if (sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		return p_buf;
	}

	if (sec_ctx->key_mode != IEEE802154_KEY_ID_MODE_IMPLICIT) {
		/* TODO: Support other key ID modes. */
		return NULL;
	}

	aux_sec = (struct ieee802154_aux_security_hdr *)p_buf;

	aux_sec->control.security_level = sec_ctx->level;
	aux_sec->control.key_id_mode = sec_ctx->key_mode;
	aux_sec->control.reserved = 0U;

	aux_sec->frame_counter = sys_cpu_to_le32(sec_ctx->frame_counter);

	return p_buf + IEEE802154_SECURITY_CF_LENGTH + IEEE802154_SECURITY_FRAME_COUNTER_LENGTH;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

bool ieee802154_create_data_frame(struct ieee802154_context *ctx, struct net_linkaddr *dst,
				  struct net_linkaddr *src, struct net_buf *buf, uint8_t ll_hdr_len)
{
	struct ieee802154_frame_params params = {0};
	struct ieee802154_fcf_seq *fs;
	uint8_t *p_buf = buf->data;
	uint8_t *buf_start = p_buf;
	bool ret = false;
	bool broadcast;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	fs = generate_fcf_grounds(&p_buf, ctx->ack_requested);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_DATA;
	fs->sequence = ctx->sequence++;

	params.dst.pan_id = ctx->pan_id;
	params.pan_id = ctx->pan_id;
	if (src->addr && src->len == IEEE802154_SHORT_ADDR_LENGTH) {
		params.short_addr = ntohs(*(uint16_t *)(src->addr));
		if (ctx->short_addr != params.short_addr) {
			goto out;
		}
	} else {
		uint8_t ext_addr_le[IEEE802154_EXT_ADDR_LENGTH];

		if (src->len != IEEE802154_EXT_ADDR_LENGTH) {
			goto out;
		}

		sys_memcpy_swap(ext_addr_le, src->addr, IEEE802154_EXT_ADDR_LENGTH);
		if (memcmp(ctx->ext_addr, ext_addr_le, src->len)) {
			goto out;
		}
	}

	broadcast = data_addr_to_fs_settings(dst, fs, &params);

	p_buf = generate_addressing_fields(ctx, fs, &params, p_buf);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	uint8_t level, authtag_len, payload_len;

	if (broadcast) {
		/* TODO: This may not always be correct. */
		NET_DBG("No security hdr needed: broadcasting");
		goto no_security_hdr;
	}

	if (ctx->sec_ctx.level == IEEE802154_SECURITY_LEVEL_NONE) {
		NET_WARN("IEEE 802.15.4 security is enabled but has not been configured.");
		goto no_security_hdr;
	}

	fs->fc.security_enabled = 1U;

	p_buf = generate_aux_security_hdr(&ctx->sec_ctx, p_buf);
	if (!p_buf) {
		NET_ERR("Unsupported key mode.");
		goto out;
	}

	level = ctx->sec_ctx.level;
	if (level >= IEEE802154_SECURITY_LEVEL_ENC) {
		level -= 4U;
	}

	/* Let's encrypt/auth only in the end, if needed */
	authtag_len = level_2_authtag_len[level];
	payload_len = buf->len - ll_hdr_len - authtag_len;
	if (!ieee802154_encrypt_auth(&ctx->sec_ctx, buf_start, ll_hdr_len,
				     payload_len, authtag_len, ctx->ext_addr)) {
		goto out;
	};

no_security_hdr:
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
	if ((p_buf - buf_start) != ll_hdr_len) {
		/* ll_hdr_len was too small? We probably overwrote payload bytes */
		NET_ERR("Could not generate data frame %zu vs %u", (p_buf - buf_start), ll_hdr_len);
		goto out;
	}

	dbg_print_fs(fs);

	ret = true;

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
}

#ifdef CONFIG_NET_L2_IEEE802154_RFD

static inline bool cfi_to_fs_settings(enum ieee802154_cfi cfi, struct ieee802154_fcf_seq *fs,
				      struct ieee802154_frame_params *params)
{
	switch (cfi) {
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
		fs->fc.pan_id_comp = 1U;

		__fallthrough;
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
		fs->fc.ar = 1U;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;

		if (params->dst.len == IEEE802154_SHORT_ADDR_LENGTH) {
			fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_SHORT;
		} else {
			fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		}

		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
	case IEEE802154_CFI_PAN_ID_CONFLICT_NOTIFICATION:
		fs->fc.ar = 1U;
		fs->fc.pan_id_comp = 1U;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		fs->fc.ar = 1U;
		/* TODO: src/dst addr mode: see section 7.5.5 */

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
		/* TODO: ack_requested and dst addr mode: see section 7.5.10 */

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

static inline uint8_t mac_command_length(enum ieee802154_cfi cfi)
{
	uint8_t length = 1U; /* cfi is at least present */

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

struct net_pkt *ieee802154_create_mac_cmd_frame(struct net_if *iface, enum ieee802154_cfi type,
						struct ieee802154_frame_params *params)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_fcf_seq *fs;
	struct net_pkt *pkt = NULL;
	uint8_t *p_buf, *p_start;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	/* It would be costly to compute the size when actual frames are never
	 * bigger than IEEE802154_MTU bytes less the FCS size, so let's allocate that
	 * size as buffer.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, IEEE802154_MTU, AF_UNSPEC, 0, BUF_TIMEOUT);
	if (!pkt) {
		goto out;
	}

	p_buf = net_pkt_data(pkt);
	p_start = p_buf;

	fs = generate_fcf_grounds(
		&p_buf, type == IEEE802154_CFI_BEACON_REQUEST ? false : ctx->ack_requested);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_MAC_COMMAND;
	fs->sequence = ctx->sequence++;

	if (!cfi_to_fs_settings(type, fs, params)) {
		goto error;
	}

	p_buf = generate_addressing_fields(ctx, fs, params, p_buf);

	net_buf_add(pkt->buffer, p_buf - p_start);

	/* Let's insert the cfi */
	((struct ieee802154_command *)p_buf)->cfi = type;

	dbg_print_fs(fs);

	goto out;

error:
	net_pkt_unref(pkt);
	pkt = NULL;

out:
	k_sem_give(&ctx->ctx_lock);
	return pkt;
}

void ieee802154_mac_cmd_finalize(struct net_pkt *pkt, enum ieee802154_cfi type)
{
	net_buf_add(pkt->buffer, mac_command_length(type));
}

#endif /* CONFIG_NET_L2_IEEE802154_RFD */

bool ieee802154_create_ack_frame(struct net_if *iface, struct net_pkt *pkt, uint8_t seq)
{
	uint8_t *p_buf = net_pkt_data(pkt);
	struct ieee802154_fcf_seq *fs;

	if (!p_buf) {
		return false;
	}

	fs = generate_fcf_grounds(&p_buf, false);

	fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_NONE;
	fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_NONE;

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_ACK;
	fs->sequence = seq;

	net_buf_add(pkt->buffer, IEEE802154_ACK_PKT_LENGTH);

	return true;
}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
bool ieee802154_decipher_data_frame(struct net_if *iface, struct net_pkt *pkt,
				    struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint8_t level, authtag_len, ll_hdr_len, payload_len;
	struct ieee802154_mhr *mhr = &mpdu->mhr;
	struct ieee802154_address *src;
	bool ret = false;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (!mhr->fs->fc.security_enabled) {
		ret = true;
		goto out;
	}

	level = ctx->sec_ctx.level;

	/* Section 9.2.4: Incoming frame security procedure, Security Enabled field is set to one
	 *
	 * [...]
	 *
	 * a) Legacy security. If the Frame Version field of the frame to be unsecured is set to
	 *    zero, the procedure shall return with a Status of UNSUPPORTED_LEGACY.
	 */
	if (mhr->aux_sec->control.security_level != level) {
		goto out;
	}

	if (level >= IEEE802154_SECURITY_LEVEL_ENC) {
		level -= 4U;
	}

	authtag_len = level_2_authtag_len[level];
	ll_hdr_len = (uint8_t *)mpdu->payload - net_pkt_data(pkt);
	payload_len = net_pkt_get_len(pkt) - ll_hdr_len - authtag_len;

	/* TODO: Handle src short address.
	 * This will require to look up in nbr cache with short addr
	 * in order to get the extended address related to it.
	 */
	if (mhr->fs->fc.src_addr_mode != IEEE802154_ADDR_MODE_EXTENDED) {
		NET_ERR("Only encrypting packages with extended source addresses is supported.");
		goto out;
	}

	src = mhr->fs->fc.pan_id_comp ? &mhr->src_addr->comp.addr : &mhr->src_addr->plain.addr;

	if (!ieee802154_decrypt_auth(&ctx->sec_ctx, net_pkt_data(pkt), ll_hdr_len, payload_len,
				     authtag_len, src->ext_addr,
				     sys_le32_to_cpu(mhr->aux_sec->frame_counter))) {
		NET_ERR("Could not decipher the frame");
		goto out;
	}

	/* We remove tag size from buf's length, it is now useless. */
	pkt->buffer->len -= authtag_len;

	ret = true;

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
