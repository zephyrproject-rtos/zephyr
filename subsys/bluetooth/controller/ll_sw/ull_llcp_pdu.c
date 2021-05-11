/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>

#include "hal/ccm.h"

#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "ll_feat.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_pdu
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/*
 * we need to filter on octet 0; the following mask has 7 octets
 * with all bits set to 1, the last octet is 0x00
 */
#define FEAT_FILT_OCTET0 0xFFFFFFFFFFFFFF00

/*
 * LE Ping Procedure Helpers
 */

void ull_cp_priv_pdu_encode_ping_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_req) + sizeof(struct pdu_data_llctrl_ping_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;
}

void ull_cp_priv_pdu_encode_ping_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_rsp) + sizeof(struct pdu_data_llctrl_ping_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
}

/*
 * Unknown response helper
 */

void ull_cp_priv_pdu_decode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu)
{
	ctx->unknown_response.type = pdu->llctrl.unknown_rsp.type;
}

void ull_cp_priv_ntf_encode_unknown_rsp(struct proc_ctx *ctx,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_unknown_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, unknown_rsp) +
		sizeof(struct pdu_data_llctrl_unknown_rsp);
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

	*featuresout = feat;
}

void ull_cp_priv_pdu_encode_feature_req(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_req) +
		sizeof(struct pdu_data_llctrl_feature_req);
	if (conn->lll.role == BT_HCI_ROLE_MASTER) {
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_REQ;
	} else {
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ;
	}

	p = &pdu->llctrl.feature_req;
	sys_put_le64(LL_FEAT, p->features);
}

void ull_cp_priv_pdu_encode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_rsp *p;
	uint64_t feature_rsp = LL_FEAT;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
		sizeof(struct pdu_data_llctrl_feature_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;

	p = &pdu->llctrl.feature_rsp;

	/*
	 * we only filter on octet 0, remaining 7 octets are the features
	 * we support, as defined in LL_FEAT
	 */
	feature_rsp &= (FEAT_FILT_OCTET0 | conn->llcp.fex.features_used);

	sys_put_le64(feature_rsp, p->features);
}

void ull_cp_priv_ntf_encode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
		sizeof(struct pdu_data_llctrl_feature_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
	p = &pdu->llctrl.feature_rsp;

	sys_put_le64(conn->llcp.fex.features_peer, p->features);
}

void ull_cp_priv_pdu_decode_feature_req(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	uint64_t featureset;

	feature_filter(pdu->llctrl.feature_req.features, &featureset);
	conn->llcp.fex.features_used = LL_FEAT & featureset;

	featureset &= (FEAT_FILT_OCTET0 | conn->llcp.fex.features_used);
	conn->llcp.fex.features_peer = featureset;

	conn->llcp.fex.valid = 1;
}

void ull_cp_priv_pdu_decode_feature_rsp(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	uint64_t featureset;

	feature_filter(pdu->llctrl.feature_rsp.features, &featureset);
	conn->llcp.fex.features_used = LL_FEAT & featureset;

	conn->llcp.fex.features_peer = featureset;
	conn->llcp.fex.valid = 1;
}
/*
 * Minimum used channels Procedure Helpers
 */

void ull_cp_priv_pdu_encode_min_used_chans_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_min_used_chans_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, min_used_chans_ind) + sizeof(struct pdu_data_llctrl_min_used_chans_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND;
	p = &pdu->llctrl.min_used_chans_ind;
	p->phys = ctx->data.muc.phys;
	p->min_used_chans = ctx->data.muc.min_used_chans;
}

void ull_cp_priv_pdu_decode_min_used_chans_ind(struct ll_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.muc.phys = pdu->llctrl.min_used_chans_ind.phys;
	conn->llcp.muc.min_used_chans = pdu->llctrl.min_used_chans_ind.min_used_chans;
}

/*
 * Termination Procedure Helper
 */
void ull_cp_priv_pdu_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_terminate_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, terminate_ind) + sizeof(struct pdu_data_llctrl_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
	p = &pdu->llctrl.terminate_ind;
	p->error_code = ctx->data.term.error_code;
}

void ull_cp_priv_ntf_encode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_terminate_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, terminate_ind) + sizeof(struct pdu_data_llctrl_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
	p = &pdu->llctrl.terminate_ind;
	p->error_code = ctx->data.term.error_code;
}

void ull_cp_priv_pdu_decode_terminate_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.term.error_code = pdu->llctrl.terminate_ind.error_code;
}

/*
 * Version Exchange Procedure Helper
 */
void ull_cp_priv_pdu_encode_version_ind(struct pdu_data *pdu)
{
	uint16_t cid;
	uint16_t svn;
	struct pdu_data_llctrl_version_ind *p;


	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) +
		sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;

	p = &pdu->llctrl.version_ind;
	p->version_number = LL_VERSION_NUMBER;
	cid = sys_cpu_to_le16(ll_settings_company_id());
	svn = sys_cpu_to_le16(ll_settings_subversion_number());
	p->company_id = cid;
	p->sub_version_number = svn;
}

void ull_cp_priv_ntf_encode_version_ind(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_version_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) +
		sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;

	p = &pdu->llctrl.version_ind;
	p->version_number = conn->llcp.vex.cached.version_number;
	p->company_id = sys_cpu_to_le16(conn->llcp.vex.cached.company_id);
	p->sub_version_number = sys_cpu_to_le16(conn->llcp.vex.cached.sub_version_number);
}

void ull_cp_priv_pdu_decode_version_ind(struct ll_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.vex.valid = 1;
	conn->llcp.vex.cached.version_number = pdu->llctrl.version_ind.version_number;
	conn->llcp.vex.cached.company_id = sys_le16_to_cpu(pdu->llctrl.version_ind.company_id);
	conn->llcp.vex.cached.sub_version_number = sys_le16_to_cpu(pdu->llctrl.version_ind.sub_version_number);
}

/*
 * Encryption Start Procedure Helper
 */

static int csrand_get(void *buf, size_t len)
{
	if (k_is_in_isr()) {
		return lll_csrand_isr_get(buf, len);
	} else {
		return lll_csrand_get(buf, len);
	}
}

void ull_cp_priv_pdu_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;

	p = &pdu->llctrl.enc_req;
	memcpy(p->rand, ctx->data.enc.rand, sizeof(p->rand));
	p->ediv[0] = ctx->data.enc.ediv[0];
	p->ediv[1] = ctx->data.enc.ediv[1];
	/* TODO(thoh): Optimize getting random data */
	csrand_get(p->skdm, sizeof(p->skdm));
	csrand_get(p->ivm, sizeof(p->ivm));
}

void ull_cp_priv_ntf_encode_enc_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;

	p = &pdu->llctrl.enc_req;
	memcpy(p->rand, ctx->data.enc.rand, sizeof(p->rand));
	p->ediv[0] = ctx->data.enc.ediv[0];
	p->ediv[1] = ctx->data.enc.ediv[1];
}

void ull_cp_priv_pdu_encode_enc_rsp(struct pdu_data *pdu)
{
	struct pdu_data_llctrl_enc_rsp *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_rsp) + sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;

	p = &pdu->llctrl.enc_rsp;
	/* TODO(thoh): Optimize getting random data */
	csrand_get(p->skds, sizeof(p->skds));
	csrand_get(p->ivs, sizeof(p->ivs));
}

void ull_cp_priv_pdu_encode_start_enc_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_req) + sizeof(struct pdu_data_llctrl_start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
}

void ull_cp_priv_pdu_encode_start_enc_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_rsp) + sizeof(struct pdu_data_llctrl_start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
}

void ull_cp_priv_pdu_encode_pause_enc_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_req) + sizeof(struct pdu_data_llctrl_pause_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ;
}

void ull_cp_priv_pdu_encode_pause_enc_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_rsp) + sizeof(struct pdu_data_llctrl_pause_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
}

void ull_cp_priv_pdu_encode_reject_ind(struct pdu_data *pdu, uint8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
	pdu->llctrl.reject_ind.error_code = error_code;
}

void ull_cp_priv_pdu_encode_reject_ext_ind(struct pdu_data *pdu, uint8_t reject_opcode, uint8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = error_code;
}

#ifdef CONFIG_BT_CTLR_PHY
/*
 * PHY Update Procedure Helper
 */

void ull_cp_priv_pdu_encode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	pdu->llctrl.phy_req.rx_phys = ctx->data.pu.rx;
	pdu->llctrl.phy_req.tx_phys = ctx->data.pu.tx;
}

void ull_cp_priv_pdu_encode_phy_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	pdu->llctrl.phy_rsp.rx_phys = conn->phy_pref_rx;
	pdu->llctrl.phy_rsp.tx_phys = conn->phy_pref_tx;
}

void ull_cp_priv_pdu_encode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = sys_cpu_to_le16(ctx->data.pu.instant);
	pdu->llctrl.phy_upd_ind.m_to_s_phy = ctx->data.pu.m_to_s_phy;
	pdu->llctrl.phy_upd_ind.s_to_m_phy = ctx->data.pu.s_to_m_phy;
}

void ull_cp_priv_pdu_decode_phy_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.rx = pdu->llctrl.phy_req.tx_phys;
	ctx->data.pu.tx = pdu->llctrl.phy_req.rx_phys;
}

void ull_cp_priv_pdu_decode_phy_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.rx = pdu->llctrl.phy_rsp.tx_phys;
	ctx->data.pu.tx = pdu->llctrl.phy_rsp.rx_phys;
}

void ull_cp_priv_pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);
	ctx->data.pu.m_to_s_phy = pdu->llctrl.phy_upd_ind.m_to_s_phy;
	ctx->data.pu.s_to_m_phy = pdu->llctrl.phy_upd_ind.s_to_m_phy;
}
#endif /* CONFIG_BT_CTLR_PHY */

/*
 * Connection Update Procedure Helper
 */
void ull_cp_priv_pdu_encode_conn_param_req(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_req) + sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
}

void ull_cp_priv_pdu_encode_conn_param_rsp(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_rsp) + sizeof(struct pdu_data_llctrl_conn_param_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
}

void ull_cp_priv_pdu_encode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_conn_update_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_update_ind) + sizeof(struct pdu_data_llctrl_conn_update_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;

	p = (void *)&pdu->llctrl.conn_update_ind;
	p->instant = sys_cpu_to_le16(ctx->data.cu.instant);
}

void ull_cp_priv_pdu_decode_conn_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.cu.instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);
}

/*
 * Channel Map Update Procedure Helpers
 */
void ull_cp_priv_pdu_encode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_chan_map_ind *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, chan_map_ind) + sizeof(struct pdu_data_llctrl_chan_map_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
	p = &pdu->llctrl.chan_map_ind;
	p->instant = sys_cpu_to_le16(ctx->data.chmu.instant);
	memcpy(p->chm, ctx->data.chmu.chm, sizeof(p->chm));
}

void ull_cp_priv_pdu_decode_chan_map_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.chmu.instant = sys_le16_to_cpu(pdu->llctrl.chan_map_ind.instant);
	memcpy(ctx->data.chmu.chm, pdu->llctrl.chan_map_ind.chm, sizeof(ctx->data.chmu.chm));
}

/*
 * Data Length Update Procedure Helpers
*/
void ull_cp_priv_pdu_encode_length_req(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_length_req *p = &pdu->llctrl.length_req;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_req) +
		sizeof(struct pdu_data_llctrl_length_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ;
	p->max_rx_octets = sys_cpu_to_le16(conn->lll.dle.local.max_rx_octets);
	p->max_tx_octets = sys_cpu_to_le16(conn->lll.dle.local.max_tx_octets);
	p->max_rx_time = sys_cpu_to_le16(conn->lll.dle.local.max_rx_time);
	p->max_tx_time = sys_cpu_to_le16(conn->lll.dle.local.max_tx_time);
}

void ull_cp_priv_pdu_encode_length_rsp(struct ll_conn *conn, struct pdu_data *pdu)
{
	struct pdu_data_llctrl_length_rsp *p = &pdu->llctrl.length_rsp;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_rsp) +
		sizeof(struct pdu_data_llctrl_length_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
	p->max_rx_octets = sys_cpu_to_le16(conn->lll.dle.local.max_rx_octets);
	p->max_tx_octets = sys_cpu_to_le16(conn->lll.dle.local.max_tx_octets);
	p->max_rx_time = sys_cpu_to_le16(conn->lll.dle.local.max_rx_time);
	p->max_tx_time = sys_cpu_to_le16(conn->lll.dle.local.max_tx_time);
}

void ull_cp_priv_ntf_encode_length_change(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_length_rsp *p = &pdu->llctrl.length_rsp;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_rsp) +
		sizeof(struct pdu_data_llctrl_length_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
	p->max_rx_octets = sys_cpu_to_le16(conn->lll.dle.eff.max_rx_octets);
	p->max_tx_octets = sys_cpu_to_le16(conn->lll.dle.eff.max_tx_octets);
	p->max_rx_time   = sys_cpu_to_le16(conn->lll.dle.eff.max_rx_time);
	p->max_tx_time   = sys_cpu_to_le16(conn->lll.dle.eff.max_tx_time);
}

void ull_cp_priv_pdu_decode_length_req(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_length_req *p = &pdu->llctrl.length_req;
	conn->lll.dle.remote.max_rx_octets = sys_le16_to_cpu(p->max_rx_octets);
	conn->lll.dle.remote.max_tx_octets = sys_le16_to_cpu(p->max_tx_octets);
	conn->lll.dle.remote.max_rx_time = sys_le16_to_cpu(p->max_rx_time);
	conn->lll.dle.remote.max_tx_time = sys_le16_to_cpu(p->max_tx_time);
}

void ull_cp_priv_pdu_decode_length_rsp(struct ll_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_length_rsp *p = &pdu->llctrl.length_rsp;
	conn->lll.dle.remote.max_rx_octets = sys_le16_to_cpu(p->max_rx_octets);
	conn->lll.dle.remote.max_tx_octets = sys_le16_to_cpu(p->max_tx_octets);
	conn->lll.dle.remote.max_rx_time = sys_le16_to_cpu(p->max_rx_time);
	conn->lll.dle.remote.max_tx_time = sys_le16_to_cpu(p->max_tx_time);
}
