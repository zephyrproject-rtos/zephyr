/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154)
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_LOG_ENABLED 1

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
validate_beacon(struct ieee802154_mpdu *mpdu, uint8_t *buf, uint8_t length)
{
	struct ieee802154_beacon *b = (struct ieee802154_beacon *)buf;
	uint8_t *p_buf = buf;
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
				uint8_t ar, uint8_t comp,
				uint8_t src, bool src_pan_brdcst_chk,
				uint8_t dst, bool dst_brdcst_chk)
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
validate_mac_command(struct ieee802154_mpdu *mpdu, uint8_t *buf, uint8_t length)
{
	struct ieee802154_command *c = (struct ieee802154_command *)buf;
	bool src_pan_brdcst_chk = false;
	bool dst_brdcst_chk = false;
	uint8_t comp = 0;
	uint8_t ar = 0;
	uint8_t src, dst;

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
		ar = 1;
		comp = 1;
		src = IEEE802154_EXT_ADDR_LENGTH;
		dst = IEEE802154_EXT_ADDR_LENGTH;

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		ar = 1;
		src = IEEE802154_ADDR_MODE_SHORT |
			IEEE802154_ADDR_MODE_EXTENDED;

		if (mpdu->mhr.fs->fc.dst_addr_mode ==
		    IEEE802154_ADDR_MODE_NONE) {
			dst = IEEE802154_ADDR_MODE_NONE;
		} else {
			comp = 1;
			dst = IEEE802154_ADDR_MODE_SHORT |
				IEEE802154_ADDR_MODE_EXTENDED;
		}

		break;
	case IEEE802154_CFI_ORPHAN_NOTIFICATION:
		comp = 1;
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
		ar = 1;
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
			 uint8_t *buf, uint8_t *p_buf, uint8_t length)
{
	uint8_t type = mpdu->mhr.fs->fc.frame_type;
	uint8_t payload_length;

	payload_length = length - (p_buf - buf) - IEEE802154_MFR_LENGTH;

	NET_DBG("Header size: %u, vs total length %u: payload size %u",
		(uint32_t)(p_buf - buf), length, payload_length);

	if (type == IEEE802154_FRAME_TYPE_BEACON) {
		if (!validate_beacon(mpdu, p_buf, payload_length)) {
			return false;
		}
	} else if (type == IEEE802154_FRAME_TYPE_DATA) {
		 /** A data frame embeds a payload */
		if (payload_length == 0) {
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

static inline struct ieee802154_fcf_seq *generate_fcf_grounds(uint8_t **p_buf,
							      uint8_t ack)
{
	struct ieee802154_fcf_seq *fs;

	fs = (struct ieee802154_fcf_seq *) *p_buf;

	fs->fc.security_enabled = 0;
	fs->fc.frame_pending = 0;
	fs->fc.ar = ack;
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
get_dst_addr_mode(struct net_if *iface, struct in6_addr *dst,
		  struct net_nbr **nbr, bool *broadcast)
{
	if (net_is_ipv6_addr_mcast(dst) ||
	    net_is_ipv6_addr_unspecified(dst)) {
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
	*nbr = net_ipv6_nbr_lookup(iface, dst);
	if (!*nbr) {
		return IEEE802154_ADDR_MODE_NONE;
	}

	return IEEE802154_ADDR_MODE_EXTENDED;
}

static inline
void data_addr_to_fs_settings(struct net_if *iface,
			      struct in6_addr *dst,
			      struct ieee802154_fcf_seq *fs,
			      struct ieee802154_frame_params *params)
{
	struct net_nbr *nbr;
	bool broadcast;

	fs->fc.dst_addr_mode = get_dst_addr_mode(iface, dst, &nbr, &broadcast);
	if (fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		fs->fc.pan_id_comp = 1;

		if (broadcast) {
			params->dst.short_addr = IEEE802154_BROADCAST_ADDRESS;
			params->dst.len = IEEE802154_SHORT_ADDR_LENGTH;
		} else {
			struct net_linkaddr_storage *addr;

			/* ToDo: Handle short address in nbr */
			addr = net_nbr_get_lladdr(nbr->idx);
			params->dst.ext_addr = addr->addr;
			params->dst.len = IEEE802154_EXT_ADDR_LENGTH;
		}
	}

	if (fs->fc.dst_addr_mode == IEEE802154_ADDR_MODE_SHORT && !broadcast) {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_SHORT;
	} else {
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
	}
}

static
uint8_t *generate_addressing_fields(struct net_if *iface,
				    struct ieee802154_fcf_seq *fs,
				    struct ieee802154_frame_params *params,
				    uint8_t *p_buf)
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
		sys_memcpy_swap(src_addr->ext_addr, iface->link_addr.addr,
				IEEE802154_EXT_ADDR_LENGTH);
		p_buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	return p_buf;
}

bool ieee802154_create_data_frame(struct net_if *iface,
				  struct in6_addr *dst,
				  uint8_t *p_buf,
				  uint8_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_frame_params params;
	struct ieee802154_fcf_seq *fs;
	uint8_t *frag_start = p_buf;

	fs = generate_fcf_grounds(&p_buf, ctx->ack_requested);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_DATA;
	fs->sequence = ctx->sequence++;

	params.dst.pan_id = ctx->pan_id;
	params.pan_id = ctx->pan_id;
	data_addr_to_fs_settings(iface, dst, fs, &params);

	p_buf = generate_addressing_fields(iface, fs, &params, p_buf);

	if ((p_buf - frag_start) != len) {
		/* ll reserve was too small? We probably overwrote
		 * payload bytes
		 */
		NET_ERR("Could not generate data frame");
		return false;
	}

	/* Todo: handle security aux header */

	dbg_print_fs(fs);

	return true;
}

#ifdef CONFIG_NET_L2_IEEE802154_RFD

static inline bool cfi_to_fs_settings(enum ieee802154_cfi cfi,
				      struct ieee802154_fcf_seq *fs,
				      struct ieee802154_frame_params *params)
{
	switch (cfi) {
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
		fs->fc.ar = 1;
		fs->fc.pan_id_comp = 1;

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
		fs->fc.ar = 1;
		fs->fc.pan_id_comp = 1;
		fs->fc.src_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;
		fs->fc.dst_addr_mode = IEEE802154_ADDR_MODE_EXTENDED;

		break;
	case IEEE802154_CFI_DATA_REQUEST:
		fs->fc.ar = 1;
		/* ToDo: src/dst addr mode: see 5.3.4 */

		break;
	case IEEE802154_CFI_ORPHAN_NOTIFICATION:
		fs->fc.pan_id_comp = 1;
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
		fs->fc.ar = 1;
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
	uint8_t reserve = 1; /* cfi is at least present */

	switch (cfi) {
	case IEEE802154_CFI_ASSOCIATION_REQUEST:
	case IEEE802154_CFI_DISASSOCIATION_NOTIFICATION:
	case IEEE802154_CFI_GTS_REQUEST:
		reserve += 1;
		break;
	case IEEE802154_CFI_ASSOCIATION_RESPONSE:
		reserve += 3;
		break;
	case IEEE802154_CFI_COORDINATOR_REALIGNEMENT:
		reserve += 8;
		break;
	default:
		break;
	}

	return reserve;
}

struct net_buf *
ieee802154_create_mac_cmd_frame(struct net_if *iface,
				enum ieee802154_cfi type,
				struct ieee802154_frame_params *params)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_fcf_seq *fs;
	struct net_buf *buf, *frag;
	uint8_t *p_buf;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return NULL;
	}

	frag = net_nbuf_get_reserve_data(0);
	if (!frag) {
		goto error;
	}

	net_buf_frag_add(buf, frag);
	p_buf = net_nbuf_ll(buf);

	fs = generate_fcf_grounds(&p_buf, false);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_MAC_COMMAND;
	fs->sequence = ctx->sequence;

	if (!cfi_to_fs_settings(type, fs, params)) {
		goto error;
	}

	p_buf = generate_addressing_fields(iface, fs, params, p_buf);

	/* Let's insert the cfi */
	((struct ieee802154_command *)p_buf)->cfi = type;

	/* In MAC command, we consider ll header being the mhr.
	 * Rest will be the MAC command itself. This will proove
	 * to be easy to handle afterwards to point directly to MAC
	 * command space, in order to fill-in its content.
	 */
	net_nbuf_set_ll_reserve(buf, p_buf - net_nbuf_ll(buf));
	net_buf_pull(frag, net_nbuf_ll_reserve(buf));

	/* Thus setting the right MAC command length
	 * Now up to the caller to fill-in this space relevantly.
	 * See ieee802154_mac_command() helper.
	 */
	net_nbuf_set_len(frag, mac_command_length(type));

	dbg_print_fs(fs);

	return buf;
error:
	net_nbuf_unref(buf);

	return NULL;
}
#endif /* CONFIG_NET_L2_IEEE802154_RFD */

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
bool ieee802154_create_ack_frame(struct net_if *iface,
				 struct net_buf *buf, uint8_t seq)
{
	uint8_t *p_buf = net_nbuf_ll(buf);
	struct ieee802154_fcf_seq *fs;

	if (!p_buf) {
		return false;
	}

	fs = generate_fcf_grounds(&p_buf, 0);

	fs->fc.dst_addr_mode = 0;
	fs->fc.src_addr_mode = 0;

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_ACK;
	fs->sequence = seq;

	return true;
}
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */
