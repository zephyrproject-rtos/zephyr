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

#include "ull_conn_types.h"
#include "ull_tx_queue.h"
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

static void feature_filter(u8_t *featuresin, u64_t *featuresout)
{
	u64_t feat;

	/*
	 * Note that in the split controller invalid bits are set
	 * to 1, in LLCP we set them to 0; the spec. does not
	 * define which value they should have
	 */
	feat = sys_get_le64(featuresin);
	feat &= LL_FEAT_BIT_MASK;

	*featuresout = feat;
}

void ull_cp_priv_pdu_encode_feature_req(struct ull_cp_conn *conn,
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

void ull_cp_priv_pdu_encode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu)
{
	struct pdu_data_llctrl_feature_rsp *p;
	u64_t feature_rsp = LL_FEAT;

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

void ull_cp_priv_ntf_encode_feature_rsp(struct ull_cp_conn *conn,
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

void ull_cp_priv_pdu_decode_feature_req(struct ull_cp_conn *conn,
					struct pdu_data *pdu)
{
	u64_t featureset;

	feature_filter(pdu->llctrl.feature_req.features, &featureset);
	conn->llcp.fex.features_used = LL_FEAT & featureset;

	featureset &= (FEAT_FILT_OCTET0 | conn->llcp.fex.features_used);
	conn->llcp.fex.features_peer = featureset;

	conn->llcp.fex.valid = 1;
}

void ull_cp_priv_pdu_decode_feature_rsp(struct ull_cp_conn *conn,
					struct pdu_data *pdu)
{
	u64_t featureset;

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

void ull_cp_priv_pdu_decode_min_used_chans_ind(struct ull_cp_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.muc.phys = pdu->llctrl.min_used_chans_ind.phys;
	conn->llcp.muc.min_used_chans = pdu->llctrl.min_used_chans_ind.min_used_chans;
}

/*
 * Version Exchange Procedure Helper
 */
void ull_cp_priv_pdu_encode_version_ind(struct pdu_data *pdu)
{
	u16_t cid;
	u16_t svn;
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

void ull_cp_priv_ntf_encode_version_ind(struct ull_cp_conn *conn,
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

void ull_cp_priv_pdu_decode_version_ind(struct ull_cp_conn *conn, struct pdu_data *pdu)
{
	conn->llcp.vex.valid = 1;
	conn->llcp.vex.cached.version_number = pdu->llctrl.version_ind.version_number;
	conn->llcp.vex.cached.company_id = sys_le16_to_cpu(pdu->llctrl.version_ind.company_id);
	conn->llcp.vex.cached.sub_version_number = sys_le16_to_cpu(pdu->llctrl.version_ind.sub_version_number);
}

/*
 * Encryption Start Procedure Helper
 */

void ull_cp_priv_pdu_encode_enc_req(struct pdu_data *pdu)
{
	//struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_enc_rsp(struct pdu_data *pdu)
{
	//struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_rsp) + sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_start_enc_req(struct pdu_data *pdu)
{
	//struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_req) + sizeof(struct pdu_data_llctrl_start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_start_enc_rsp(struct pdu_data *pdu)
{
	//struct pdu_data_llctrl_enc_req *p;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_rsp) + sizeof(struct pdu_data_llctrl_start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_reject_ind(struct pdu_data *pdu, u8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
	pdu->llctrl.reject_ind.error_code = error_code;
}

void ull_cp_priv_pdu_encode_reject_ext_ind(struct pdu_data *pdu, u8_t reject_opcode, u8_t error_code)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = error_code;
}

/*
 * PHY Update Procedure Helper
 */

void ull_cp_priv_pdu_encode_phy_req(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_phy_rsp(struct pdu_data *pdu)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	/* TODO(thoh): Fill in PDU with correct data */
}

void ull_cp_priv_pdu_encode_phy_update_ind(struct pdu_data *pdu, u16_t instant)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = sys_cpu_to_le16(instant);
}

void ull_cp_priv_pdu_decode_phy_update_ind(struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->data.pu.instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);
}
