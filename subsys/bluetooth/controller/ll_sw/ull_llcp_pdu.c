/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"

#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "ull_adv_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "ull_sync_types.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "ll_feat.h"

#include <soc.h>
#include "hal/debug.h"

/*
 * we need to filter on octet 0; the following mask has 7 octets
 * with all bits set to 1, the last octet is 0x00
 */
#define FEAT_FILT_OCTET0 0xFFFFFFFFFFFFFF00

#if defined(CONFIG_BT_CTLR_LE_PING)
/*
 * LE Ping Procedure Helpers
 */

void llcp_pdu_encode_ping_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(ping_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;
}

void llcp_pdu_encode_ping_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(ping_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
}
#endif /* CONFIG_BT_CTLR_LE_PING */
/*
 * Unknown response helper
 */

void llcp_pdu_encode_unknown_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;

	pdu->llctrl.unknown_rsp.type = ctx->unknown_response.type;
}

void llcp_pdu_decode_unknown_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->unknown_response.type = pdu->llctrl.unknown_rsp.type;
}

void llcp_ntf_encode_unknown_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_unknown_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	p = &pdu->llctrl.unknown_rsp;
	p->type = ctx->unknown_response.type;
}

/*
 * Feature Exchange Procedure Helper
 */

static void feature_filter(uint8_t *featuresin, uint64_t *featuresout)
{
	uint64_t feat;

	/*
	 * Note that in the split controller invalid bits are set
	 * to 1, in LLCP we set them to 0; the spec. does not
	 * define which value they should have
	 */
	feat = sys_get_le64(featuresin);
	feat &= LL_FEAT_BIT_MASK;
	feat &= LL_FEAT_BIT_MASK_VALID;

	*featuresout = feat;
}

void llcp_pdu_encode_feature_req(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(feature_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_REQ;

#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_PERIPHERAL)
	if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL) {
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG;
	}
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_PERIPHERAL */

	p = &pdu->llctrl.feature_req;
	sys_put_le64(ll_feat_get(), p->features);
}

void llcp_pdu_encode_feature_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_rsp *p;
	uint64_t feature_rsp = ll_feat_get();

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(feature_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;

	p = &pdu->llctrl.feature_rsp;

	/*
	 * we only filter on octet 0, remaining 7 octets are the features
	 * we support, as defined in LL_FEAT
	 */
	feature_rsp &= (FEAT_FILT_OCTET0 | conn->llcp.fex.features_used);

	sys_put_le64(feature_rsp, p->features);
}

void llcp_ntf_encode_feature_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(feature_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
	p = &pdu->llctrl.feature_rsp;

	sys_put_le64(conn->llcp.fex.features_peer, p->features);
}

static uint64_t features_used(uint64_t featureset)
{
	uint64_t x;

	/* swap bits for role specific features */
	x = ((featureset >> BT_LE_FEAT_BIT_CIS_CENTRAL) ^
	     (featureset >> BT_LE_FEAT_BIT_CIS_PERIPHERAL)) & 0x01;
	x = (x << BT_LE_FEAT_BIT_CIS_CENTRAL) |
	    (x << BT_LE_FEAT_BIT_CIS_PERIPHERAL);
	x ^= featureset;

	return ll_feat_get() & x;
}

void llcp_pdu_decode_feature_req(struct ll_conn *conn, struct pdu_data *pdu)
{
	uint64_t featureset;

	feature_filter(pdu->llctrl.feature_req.features, &featureset);
	conn->llcp.fex.features_used = features_used(featureset);

	featureset &= (FEAT_FILT_OCTET0 | conn->llcp.fex.features_used);
	conn->llcp.fex.features_peer = featureset;

	conn->llcp.fex.valid = 1;
}

void llcp_pdu_decode_feature_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	uint64_t featureset;

	feature_filter(pdu->llctrl.feature_rsp.features, &featureset);
	conn->llcp.fex.features_used = features_used(featureset);
	conn->llcp.fex.features_peer = featureset;
	conn->llcp.fex.valid = 1;
}

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
/*
 * Minimum used channels Procedure Helpers
 */
#if defined(CONFIG_BT_PERIPHERAL)
void llcp_pdu_encode_min_used_chans_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_min_used_chans_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(min_used_chans_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND;
	p = &pdu->llctrl.min_used_chans_ind;
	p->phys = ctx->data.muc.phys;
	p->min_used_chans = ctx->data.muc.min_used_chans;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_decode_min_used_chans_ind(struct ll_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.muc.phys = pdu->llctrl.min_used_chans_ind.phys;
	conn->llcp.muc.min_used_chans = pdu->llctrl.min_used_chans_ind.min_used_chans;
}
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

/*
 * Termination Procedure Helper
 */
void llcp_pdu_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_terminate_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
	p = &pdu->llctrl.terminate_ind;
	p->error_code = ctx->data.term.error_code;
}

void llcp_pdu_decode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.term.error_code = pdu->llctrl.terminate_ind.error_code;
}

/*
 * Version Exchange Procedure Helper
 */
void llcp_pdu_encode_version_ind(struct pdu_data *pdu)
{
	uint16_t cid;
	uint16_t svn;
	struct pdu_data_llctrl_version_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;

	p = &pdu->llctrl.version_ind;
	p->version_number = LL_VERSION_NUMBER;
	cid = sys_cpu_to_le16(ll_settings_company_id());
	svn = sys_cpu_to_le16(ll_settings_subversion_number());
	p->company_id = cid;
	p->sub_version_number = svn;
}

void llcp_ntf_encode_version_ind(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_version_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;

	p = &pdu->llctrl.version_ind;
	p->version_number = conn->llcp.vex.cached.version_number;
	p->company_id = sys_cpu_to_le16(conn->llcp.vex.cached.company_id);
	p->sub_version_number = sys_cpu_to_le16(conn->llcp.vex.cached.sub_version_number);
}

void llcp_pdu_decode_version_ind(struct ll_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.vex.valid = 1;
	conn->llcp.vex.cached.version_number = pdu->llctrl.version_ind.version_number;
	conn->llcp.vex.cached.company_id = sys_le16_to_cpu(pdu->llctrl.version_ind.company_id);
	conn->llcp.vex.cached.sub_version_number =
		sys_le16_to_cpu(pdu->llctrl.version_ind.sub_version_number);
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
/*
 * Encryption Start Procedure Helper
 */

static int csrand_get(void *buf, size_t len)
{
	if (mayfly_is_running()) {
		return lll_csrand_isr_get(buf, len);
	} else {
		return lll_csrand_get(buf, len);
	}
}

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_PERIPHERAL)
static void encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;

	p = &pdu->llctrl.enc_req;
	memcpy(p->rand, ctx->data.enc.rand, sizeof(p->rand));
	p->ediv[0] = ctx->data.enc.ediv[0];
	p->ediv[1] = ctx->data.enc.ediv[1];
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_req *p;

	encode_enc_req(ctx, pdu);

	/* Optimal getting random data, p->ivm is packed right after p->skdm */
	p = &pdu->llctrl.enc_req;
	BUILD_ASSERT(offsetof(struct pdu_data_llctrl_enc_req, ivm) ==
		     offsetof(struct pdu_data_llctrl_enc_req, skdm) + sizeof(p->skdm),
		     "Member IVM must be after member SKDM");
	csrand_get(p->skdm, sizeof(p->skdm) + sizeof(p->ivm));

}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
void llcp_ntf_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	encode_enc_req(ctx, pdu);
}

void llcp_pdu_encode_enc_rsp(struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;

	p = &pdu->llctrl.enc_rsp;
	/* Optimal getting random data, p->ivs is packed right after p->skds */
	BUILD_ASSERT(offsetof(struct pdu_data_llctrl_enc_rsp, ivs) ==
		     offsetof(struct pdu_data_llctrl_enc_rsp, skds) + sizeof(p->skds),
		     "Member IVS must be after member SKDS");
	csrand_get(p->skds, sizeof(p->skds) + sizeof(p->ivs));
}

void llcp_pdu_encode_start_enc_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
}
#endif /* CONFIG_BT_PERIPHERAL */

void llcp_pdu_encode_start_enc_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
}

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_pause_enc_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(pause_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ;
}
#endif /* CONFIG_BT_CENTRAL */

void llcp_pdu_encode_pause_enc_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(pause_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

void llcp_pdu_encode_reject_ind(struct pdu_data *pdu, uint8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(reject_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
	pdu->llctrl.reject_ind.error_code = error_code;
}

void llcp_pdu_encode_reject_ext_ind(struct pdu_data *pdu, uint8_t reject_opcode, uint8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = error_code;
}

void llcp_pdu_decode_reject_ext_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->reject_ext_ind.reject_opcode = pdu->llctrl.reject_ext_ind.reject_opcode;
	ctx->reject_ext_ind.error_code = pdu->llctrl.reject_ext_ind.error_code;
}

void llcp_ntf_encode_reject_ext_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_reject_ext_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;

	p = (void *)&pdu->llctrl.reject_ext_ind;
	p->error_code = ctx->reject_ext_ind.error_code;
	p->reject_opcode = ctx->reject_ext_ind.reject_opcode;
}

#ifdef CONFIG_BT_CTLR_PHY
/*
 * PHY Update Procedure Helper
 */

void llcp_pdu_encode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	pdu->llctrl.phy_req.rx_phys = ctx->data.pu.rx;
	pdu->llctrl.phy_req.tx_phys = ctx->data.pu.tx;
}

void llcp_pdu_decode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.rx = pdu->llctrl.phy_req.tx_phys;
	ctx->data.pu.tx = pdu->llctrl.phy_req.rx_phys;
}

#if defined(CONFIG_BT_PERIPHERAL)
void llcp_pdu_encode_phy_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	pdu->llctrl.phy_rsp.rx_phys = conn->phy_pref_rx;
	pdu->llctrl.phy_rsp.tx_phys = conn->phy_pref_tx;
}
void llcp_pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);
	ctx->data.pu.c_to_p_phy = pdu->llctrl.phy_upd_ind.c_to_p_phy;
	ctx->data.pu.p_to_c_phy = pdu->llctrl.phy_upd_ind.p_to_c_phy;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
void llcp_pdu_encode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = sys_cpu_to_le16(ctx->data.pu.instant);
	pdu->llctrl.phy_upd_ind.c_to_p_phy = ctx->data.pu.c_to_p_phy;
	pdu->llctrl.phy_upd_ind.p_to_c_phy = ctx->data.pu.p_to_c_phy;
}
void llcp_pdu_decode_phy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.rx = pdu->llctrl.phy_rsp.tx_phys;
	ctx->data.pu.tx = pdu->llctrl.phy_rsp.rx_phys;
}
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/*
 * Connection Update Procedure Helper
 */
static void encode_conn_param_req_rsp_common(struct proc_ctx *ctx, struct pdu_data *pdu,
					     struct pdu_data_llctrl_conn_param_req_rsp_common *p,
					     uint8_t opcode)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	/* The '+ 1U' is to count in opcode octet, the first member of struct pdu_data_llctrl */
	pdu->len = sizeof(struct pdu_data_llctrl_conn_param_req_rsp_common) + 1U;
	pdu->llctrl.opcode = opcode;

	p->interval_min = sys_cpu_to_le16(ctx->data.cu.interval_min);
	p->interval_max = sys_cpu_to_le16(ctx->data.cu.interval_max);
	p->latency = sys_cpu_to_le16(ctx->data.cu.latency);
	p->timeout = sys_cpu_to_le16(ctx->data.cu.timeout);
	p->preferred_periodicity = ctx->data.cu.preferred_periodicity;
	p->reference_conn_event_count = sys_cpu_to_le16(ctx->data.cu.reference_conn_event_count);
	p->offset0 = sys_cpu_to_le16(ctx->data.cu.offsets[0]);
	p->offset1 = sys_cpu_to_le16(ctx->data.cu.offsets[1]);
	p->offset2 = sys_cpu_to_le16(ctx->data.cu.offsets[2]);
	p->offset3 = sys_cpu_to_le16(ctx->data.cu.offsets[3]);
	p->offset4 = sys_cpu_to_le16(ctx->data.cu.offsets[4]);
	p->offset5 = sys_cpu_to_le16(ctx->data.cu.offsets[5]);
}


void llcp_pdu_encode_conn_param_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	encode_conn_param_req_rsp_common(ctx, pdu,
		(struct pdu_data_llctrl_conn_param_req_rsp_common *)&pdu->llctrl.conn_param_req,
		PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ);
}

void llcp_pdu_encode_conn_param_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	encode_conn_param_req_rsp_common(ctx, pdu,
		(struct pdu_data_llctrl_conn_param_req_rsp_common *)&pdu->llctrl.conn_param_rsp,
		PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP);
}

static void decode_conn_param_req_rsp_common(struct proc_ctx *ctx,
					     struct pdu_data_llctrl_conn_param_req_rsp_common *p)
{
	ctx->data.cu.interval_min = sys_le16_to_cpu(p->interval_min);
	ctx->data.cu.interval_max = sys_le16_to_cpu(p->interval_max);
	ctx->data.cu.latency = sys_le16_to_cpu(p->latency);
	ctx->data.cu.timeout = sys_le16_to_cpu(p->timeout);
	ctx->data.cu.preferred_periodicity = p->preferred_periodicity;
	ctx->data.cu.reference_conn_event_count = sys_le16_to_cpu(p->reference_conn_event_count);
	ctx->data.cu.offsets[0] = sys_le16_to_cpu(p->offset0);
	ctx->data.cu.offsets[1] = sys_le16_to_cpu(p->offset1);
	ctx->data.cu.offsets[2] = sys_le16_to_cpu(p->offset2);
	ctx->data.cu.offsets[3] = sys_le16_to_cpu(p->offset3);
	ctx->data.cu.offsets[4] = sys_le16_to_cpu(p->offset4);
	ctx->data.cu.offsets[5] = sys_le16_to_cpu(p->offset5);
}

void llcp_pdu_decode_conn_param_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	decode_conn_param_req_rsp_common(ctx,
		(struct pdu_data_llctrl_conn_param_req_rsp_common *)&pdu->llctrl.conn_param_req);
}

void llcp_pdu_decode_conn_param_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	decode_conn_param_req_rsp_common(ctx,
		(struct pdu_data_llctrl_conn_param_req_rsp_common *)&pdu->llctrl.conn_param_rsp);
}
#endif /* defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) */

void llcp_pdu_encode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_conn_update_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(conn_update_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;

	p = (void *)&pdu->llctrl.conn_update_ind;
	p->win_size = ctx->data.cu.win_size;
	p->win_offset = sys_cpu_to_le16(ctx->data.cu.win_offset_us / CONN_INT_UNIT_US);
	p->latency = sys_cpu_to_le16(ctx->data.cu.latency);
	p->interval = sys_cpu_to_le16(ctx->data.cu.interval_max);
	p->timeout = sys_cpu_to_le16(ctx->data.cu.timeout);
	p->instant = sys_cpu_to_le16(ctx->data.cu.instant);
}

void llcp_pdu_decode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_conn_update_ind *p;

	p = (void *)&pdu->llctrl.conn_update_ind;
	ctx->data.cu.win_size = p->win_size;
	ctx->data.cu.win_offset_us = sys_le16_to_cpu(p->win_offset) * CONN_INT_UNIT_US;
	ctx->data.cu.latency = sys_le16_to_cpu(p->latency);
	ctx->data.cu.interval_max = sys_le16_to_cpu(p->interval);
	ctx->data.cu.timeout = sys_le16_to_cpu(p->timeout);
	ctx->data.cu.instant = sys_le16_to_cpu(p->instant);
}

/*
 * Channel Map Update Procedure Helpers
 */
void llcp_pdu_encode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_chan_map_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(chan_map_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
	p = &pdu->llctrl.chan_map_ind;
	p->instant = sys_cpu_to_le16(ctx->data.chmu.instant);
	memcpy(p->chm, ctx->data.chmu.chm, sizeof(p->chm));
}

void llcp_pdu_decode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.chmu.instant = sys_le16_to_cpu(pdu->llctrl.chan_map_ind.instant);
	memcpy(ctx->data.chmu.chm, pdu->llctrl.chan_map_ind.chm, sizeof(ctx->data.chmu.chm));
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
/*
 * Data Length Update Procedure Helpers
 */
static void encode_length_req_rsp_common(struct pdu_data *pdu,
					 struct pdu_data_llctrl_length_req_rsp_common *p,
					 const uint8_t opcode,
					 const struct data_pdu_length *dle)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	/* The '+ 1U' is to count in opcode octet, the first member of struct pdu_data_llctrl */
	pdu->len = sizeof(struct pdu_data_llctrl_length_req_rsp_common) + 1U;
	pdu->llctrl.opcode = opcode;
	p->max_rx_octets = sys_cpu_to_le16(dle->max_rx_octets);
	p->max_tx_octets = sys_cpu_to_le16(dle->max_tx_octets);
	p->max_rx_time = sys_cpu_to_le16(dle->max_rx_time);
	p->max_tx_time = sys_cpu_to_le16(dle->max_tx_time);
}

void llcp_pdu_encode_length_req(struct ll_conn *conn, struct pdu_data *pdu)
{
	encode_length_req_rsp_common(pdu,
		(struct pdu_data_llctrl_length_req_rsp_common *)&pdu->llctrl.length_req,
		PDU_DATA_LLCTRL_TYPE_LENGTH_REQ,
		&conn->lll.dle.local);
}

void llcp_pdu_encode_length_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	encode_length_req_rsp_common(pdu,
		(struct pdu_data_llctrl_length_req_rsp_common *)&pdu->llctrl.length_rsp,
		PDU_DATA_LLCTRL_TYPE_LENGTH_RSP,
		&conn->lll.dle.local);
}

void llcp_ntf_encode_length_change(struct ll_conn *conn, struct pdu_data *pdu)
{
	encode_length_req_rsp_common(pdu,
		(struct pdu_data_llctrl_length_req_rsp_common *)&pdu->llctrl.length_rsp,
		PDU_DATA_LLCTRL_TYPE_LENGTH_RSP,
		&conn->lll.dle.eff);
}

static bool dle_remote_valid(const struct data_pdu_length *remote)
{
	if (!IN_RANGE(remote->max_rx_octets, PDU_DC_PAYLOAD_SIZE_MIN,
		      PDU_DC_PAYLOAD_SIZE_MAX)) {
		return false;
	}

	if (!IN_RANGE(remote->max_tx_octets, PDU_DC_PAYLOAD_SIZE_MIN,
		      PDU_DC_PAYLOAD_SIZE_MAX)) {
		return false;
	}

#if defined(CONFIG_BT_CTLR_PHY)
	if (!IN_RANGE(remote->max_rx_time, PDU_DC_PAYLOAD_TIME_MIN,
		      PDU_DC_PAYLOAD_TIME_MAX_CODED)) {
		return false;
	}

	if (!IN_RANGE(remote->max_tx_time, PDU_DC_PAYLOAD_TIME_MIN,
		      PDU_DC_PAYLOAD_TIME_MAX_CODED)) {
		return false;
	}
#else
	if (!IN_RANGE(remote->max_rx_time, PDU_DC_PAYLOAD_TIME_MIN,
		      PDU_DC_PAYLOAD_TIME_MAX)) {
		return false;
	}

	if (!IN_RANGE(remote->max_tx_time, PDU_DC_PAYLOAD_TIME_MIN,
		      PDU_DC_PAYLOAD_TIME_MAX)) {
		return false;
	}
#endif /* CONFIG_BT_CTLR_PHY */

	return true;
}
static void decode_length_req_rsp_common(struct ll_conn *conn,
					 struct pdu_data_llctrl_length_req_rsp_common *p)
{
	struct data_pdu_length remote;

	remote.max_rx_octets = sys_le16_to_cpu(p->max_rx_octets);
	remote.max_tx_octets = sys_le16_to_cpu(p->max_tx_octets);
	remote.max_rx_time = sys_le16_to_cpu(p->max_rx_time);
	remote.max_tx_time = sys_le16_to_cpu(p->max_tx_time);

	if (!dle_remote_valid(&remote)) {
		return;
	}

	conn->lll.dle.remote = remote;
}

void llcp_pdu_decode_length_req(struct ll_conn *conn, struct pdu_data *pdu)
{
	decode_length_req_rsp_common(conn,
		(struct pdu_data_llctrl_length_req_rsp_common *)&pdu->llctrl.length_req);
}

void llcp_pdu_decode_length_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	decode_length_req_rsp_common(conn,
		(struct pdu_data_llctrl_length_req_rsp_common *)&pdu->llctrl.length_rsp);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
/*
 * Constant Tone Request Procedure Helper
 */
void llcp_pdu_encode_cte_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_cte_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(cte_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ;

	p = &pdu->llctrl.cte_req;
	p->min_cte_len_req = ctx->data.cte_req.min_len;
	p->rfu = 0; /* Reserved for future use */
	p->cte_type_req = ctx->data.cte_req.type;
}

void llcp_pdu_decode_cte_rsp(struct proc_ctx *ctx, const struct pdu_data *pdu)
{
	if (pdu->cp == 0U || pdu->octet3.cte_info.time == 0U) {
		ctx->data.cte_remote_rsp.has_cte = false;
	} else {
		ctx->data.cte_remote_rsp.has_cte = true;
	}
}

void llcp_ntf_encode_cte_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(cte_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CTE_RSP;

	/* Received LL_CTE_RSP PDU didn't have CTE */
	pdu->cp = 0U;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
void llcp_pdu_decode_cte_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.cte_remote_req.min_cte_len = pdu->llctrl.cte_req.min_cte_len_req;
	ctx->data.cte_remote_req.cte_type = pdu->llctrl.cte_req.cte_type_req;
}

void llcp_pdu_encode_cte_rsp(const struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(cte_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CTE_RSP;

	/* Set content of a PDU header first byte, that is not changed by LLL */
	pdu->cp = 1U;
	pdu->rfu = 0U;

	pdu->octet3.cte_info.time = ctx->data.cte_remote_req.min_cte_len;
	pdu->octet3.cte_info.type = ctx->data.cte_remote_req.cte_type;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
void llcp_pdu_decode_cis_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.cis_create.cig_id	= pdu->llctrl.cis_req.cig_id;
	ctx->data.cis_create.cis_id	= pdu->llctrl.cis_req.cis_id;
	ctx->data.cis_create.cis_offset_min = sys_get_le24(pdu->llctrl.cis_req.cis_offset_min);
	ctx->data.cis_create.cis_offset_max = sys_get_le24(pdu->llctrl.cis_req.cis_offset_max);
	ctx->data.cis_create.conn_event_count =
		sys_le16_to_cpu(pdu->llctrl.cis_req.conn_event_count);
	ctx->data.cis_create.iso_interval = sys_le16_to_cpu(pdu->llctrl.cis_req.iso_interval);
	/* The remainder of the req is decoded by ull_peripheral_iso_acquire, so
	 *  no need to do it here too
	ctx->data.cis_create.c_phy	= pdu->llctrl.cis_req.c_phy;
	ctx->data.cis_create.p_phy	= pdu->llctrl.cis_req.p_phy;
	ctx->data.cis_create.c_max_sdu	= pdu->llctrl.cis_req.c_max_sdu;
	ctx->data.cis_create.p_max_sdu	= pdu->llctrl.cis_req.p_max_sdu;
	ctx->data.cis_create.framed	= pdu->llctrl.cis_req.framed;
	ctx->data.cis_create.c_sdu_interval = sys_get_le24(pdu->llctrl.cis_req.c_sdu_interval);
	ctx->data.cis_create.p_sdu_interval = sys_get_le24(pdu->llctrl.cis_req.p_sdu_interval);
	ctx->data.cis_create.nse	= pdu->llctrl.cis_req.nse;
	ctx->data.cis_create.c_max_pdu	= sys_le16_to_cpu(pdu->llctrl.cis_req.c_max_pdu);
	ctx->data.cis_create.p_max_pdu	= sys_le16_to_cpu(pdu->llctrl.cis_req.p_max_pdu);
	ctx->data.cis_create.sub_interval = sys_get_le24(pdu->llctrl.cis_req.sub_interval);
	ctx->data.cis_create.p_bn	= pdu->llctrl.cis_req.p_bn;
	ctx->data.cis_create.c_bn	= pdu->llctrl.cis_req.c_bn;
	ctx->data.cis_create.c_ft	= pdu->llctrl.cis_req.c_ft;
	ctx->data.cis_create.p_ft	= pdu->llctrl.cis_req.p_ft;
	*/
}

void llcp_pdu_decode_cis_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.cis_create.conn_event_count =
		sys_le16_to_cpu(pdu->llctrl.cis_ind.conn_event_count);
	/* The remainder of the cis ind is decoded by ull_peripheral_iso_setup, so
	 * no need to do it here too
	 */
}

void llcp_pdu_encode_cis_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_cis_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(cis_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_RSP;

	p = &pdu->llctrl.cis_rsp;
	sys_put_le24(ctx->data.cis_create.cis_offset_max, p->cis_offset_max);
	sys_put_le24(ctx->data.cis_create.cis_offset_min, p->cis_offset_min);
	p->conn_event_count = sys_cpu_to_le16(ctx->data.cis_create.conn_event_count);
}
#endif /* defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
void llcp_pdu_encode_cis_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_cis_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_req) +
			    sizeof(struct pdu_data_llctrl_cis_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ;

	p = &pdu->llctrl.cis_req;

	p->cig_id = ctx->data.cis_create.cig_id;
	p->cis_id = ctx->data.cis_create.cis_id;
	p->c_phy = ctx->data.cis_create.c_phy;
	p->p_phy = ctx->data.cis_create.p_phy;
	p->nse = ctx->data.cis_create.nse;
	p->c_bn = ctx->data.cis_create.c_bn;
	p->p_bn = ctx->data.cis_create.p_bn;
	p->c_ft = ctx->data.cis_create.c_ft;
	p->p_ft = ctx->data.cis_create.p_ft;

	sys_put_le24(ctx->data.cis_create.c_sdu_interval, p->c_sdu_interval);
	sys_put_le24(ctx->data.cis_create.p_sdu_interval, p->p_sdu_interval);
	sys_put_le24(ctx->data.cis_create.cis_offset_max, p->cis_offset_max);
	sys_put_le24(ctx->data.cis_create.cis_offset_min, p->cis_offset_min);
	sys_put_le24(ctx->data.cis_create.sub_interval, p->sub_interval);

	p->c_max_pdu = sys_cpu_to_le16(ctx->data.cis_create.c_max_pdu);
	p->p_max_pdu = sys_cpu_to_le16(ctx->data.cis_create.p_max_pdu);
	p->iso_interval = sys_cpu_to_le16(ctx->data.cis_create.iso_interval);
	p->conn_event_count = sys_cpu_to_le16(ctx->data.cis_create.conn_event_count);

	p->c_max_sdu_packed[0] = ctx->data.cis_create.c_max_sdu & 0xFF;
	p->c_max_sdu_packed[1] = ((ctx->data.cis_create.c_max_sdu >> 8) & 0x0F) |
				  (ctx->data.cis_create.framed << 7);
	p->p_max_sdu[0] = ctx->data.cis_create.p_max_sdu & 0xFF;
	p->p_max_sdu[1] = (ctx->data.cis_create.p_max_sdu >> 8) & 0x0F;
}

void llcp_pdu_decode_cis_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* Limit response to valid range */
	uint32_t cis_offset_min = sys_get_le24(pdu->llctrl.cis_rsp.cis_offset_min);
	uint32_t cis_offset_max = sys_get_le24(pdu->llctrl.cis_rsp.cis_offset_max);

	/* TODO: Fail procedure if offsets are invalid? */
	if (cis_offset_min <= cis_offset_max &&
	    cis_offset_min >= ctx->data.cis_create.cis_offset_min &&
	    cis_offset_min <= ctx->data.cis_create.cis_offset_max &&
	    cis_offset_max <= ctx->data.cis_create.cis_offset_max &&
	    cis_offset_max >= ctx->data.cis_create.cis_offset_min) {
		/* Offsets are valid */
		ctx->data.cis_create.cis_offset_min = cis_offset_min;
		ctx->data.cis_create.cis_offset_max = cis_offset_max;
	}

	ctx->data.cis_create.conn_event_count =
		sys_le16_to_cpu(pdu->llctrl.cis_rsp.conn_event_count);
}

void llcp_pdu_encode_cis_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_cis_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_ind) +
			    sizeof(struct pdu_data_llctrl_cis_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_IND;

	p = &pdu->llctrl.cis_ind;

	p->aa[0] = ctx->data.cis_create.aa[0];
	p->aa[1] = ctx->data.cis_create.aa[1];
	p->aa[2] = ctx->data.cis_create.aa[2];
	p->aa[3] = ctx->data.cis_create.aa[3];

	sys_put_le24(ctx->data.cis_create.cis_offset_min, p->cis_offset);
	sys_put_le24(ctx->data.cis_create.cig_sync_delay, p->cig_sync_delay);
	sys_put_le24(ctx->data.cis_create.cis_sync_delay, p->cis_sync_delay);

	p->conn_event_count = sys_cpu_to_le16(ctx->data.cis_create.conn_event_count);
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
void llcp_pdu_encode_cis_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_cis_terminate_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(cis_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_TERMINATE_IND;

	p = &pdu->llctrl.cis_terminate_ind;
	p->cig_id = ctx->data.cis_term.cig_id;
	p->cis_id = ctx->data.cis_term.cis_id;
	p->error_code = ctx->data.cis_term.error_code;
}

void llcp_pdu_decode_cis_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.cis_term.cig_id = pdu->llctrl.cis_terminate_ind.cig_id;
	ctx->data.cis_term.cis_id = pdu->llctrl.cis_terminate_ind.cis_id;
	ctx->data.cis_term.error_code = pdu->llctrl.cis_terminate_ind.error_code;
}
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
/*
 * SCA Update Procedure Helpers
 */
void llcp_pdu_encode_clock_accuracy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_clock_accuracy_req *p = &pdu->llctrl.clock_accuracy_req;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(clock_accuracy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ;
	/* Currently we do not support variable SCA, so we always 'report' current SCA */
	p->sca = lll_clock_sca_local_get();
}

void llcp_pdu_encode_clock_accuracy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_clock_accuracy_rsp *p = &pdu->llctrl.clock_accuracy_rsp;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(clock_accuracy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP;
	/* Currently we do not support variable SCA, so we always 'report' current SCA */
	p->sca = lll_clock_sca_local_get();
}

void llcp_pdu_decode_clock_accuracy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_clock_accuracy_req *p = &pdu->llctrl.clock_accuracy_req;

	ctx->data.sca_update.sca = p->sca;
}

void llcp_pdu_decode_clock_accuracy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_clock_accuracy_rsp *p = &pdu->llctrl.clock_accuracy_rsp;

	ctx->data.sca_update.sca = p->sca;
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

/*
 * Periodic Adv Sync Transfer Procedure Helpers
 */
#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
void llcp_pdu_fill_sync_info_offset(struct pdu_adv_sync_info *si, uint32_t offset_us)
{
	uint8_t offset_adjust = 0U;
	uint8_t offset_units = 0U;

	if (offset_us >= OFFS_ADJUST_US) {
		offset_us -= OFFS_ADJUST_US;
		offset_adjust = 1U;
	}

	offset_us = offset_us / OFFS_UNIT_30_US;
	if (!!(offset_us >> OFFS_UNIT_BITS)) {
		offset_us = offset_us / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		offset_units = OFFS_UNIT_VALUE_300_US;
	}

	/* Fill in the offset */
	PDU_ADV_SYNC_INFO_OFFS_SET(si, offset_us, offset_units, offset_adjust);
}

void llcp_pdu_encode_periodic_sync_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_periodic_sync_ind *p = &pdu->llctrl.periodic_sync_ind;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = PDU_DATA_LLCTRL_LEN(periodic_sync_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PERIODIC_SYNC_IND;

	p->id = sys_cpu_to_le16(ctx->data.periodic_sync.id);

	memcpy(&p->sync_info, &ctx->data.periodic_sync.sync_info, sizeof(struct pdu_adv_sync_info));

	p->sync_info.evt_cntr = sys_cpu_to_le16(ctx->data.periodic_sync.sync_info.evt_cntr);

	p->conn_event_count = sys_cpu_to_le16(ctx->data.periodic_sync.conn_event_count);
	p->last_pa_event_counter = sys_cpu_to_le16(ctx->data.periodic_sync.last_pa_event_counter);
	p->sid = ctx->data.periodic_sync.sid;
	p->addr_type = ctx->data.periodic_sync.addr_type;
	p->sca = ctx->data.periodic_sync.sca;
	p->phy = ctx->data.periodic_sync.phy;

	memcpy(&p->adv_addr, &ctx->data.periodic_sync.adv_addr, sizeof(bt_addr_t));

	p->sync_conn_event_count = sys_cpu_to_le16(ctx->data.periodic_sync.sync_conn_event_count);
}
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
void llcp_pdu_decode_periodic_sync_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_periodic_sync_ind *p = &pdu->llctrl.periodic_sync_ind;

	ctx->data.periodic_sync.id = sys_le16_to_cpu(p->id);

	memcpy(&ctx->data.periodic_sync.sync_info, &p->sync_info, sizeof(struct pdu_adv_sync_info));

	ctx->data.periodic_sync.conn_event_count = sys_le16_to_cpu(p->conn_event_count);
	ctx->data.periodic_sync.last_pa_event_counter = sys_le16_to_cpu(p->last_pa_event_counter);
	ctx->data.periodic_sync.sid = p->sid;
	ctx->data.periodic_sync.addr_type = p->addr_type;
	ctx->data.periodic_sync.sca = p->sca;
	ctx->data.periodic_sync.phy = p->phy;

	memcpy(&ctx->data.periodic_sync.adv_addr, &p->adv_addr, sizeof(bt_addr_t));

	ctx->data.periodic_sync.sync_conn_event_count = sys_le16_to_cpu(p->sync_conn_event_count);
}
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */
