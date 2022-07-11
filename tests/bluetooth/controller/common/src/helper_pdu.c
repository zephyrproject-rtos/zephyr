/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/types.h"
#include "ztest.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"
#include "ll_feat.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn_iso.h"

#include "lll_conn.h"
#include "ull_tx_queue.h"
#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"

#include "ull_llcp.h"

#include "helper_pdu.h"
#include "helper_features.h"

#define PDU_MEM_EQUAL(_f, _s, _p, _t)                                                              \
	zassert_mem_equal(_s._f, _p->_f, sizeof(_p->_f), _t "\nCalled at %s:%d\n", file, line);

void helper_pdu_encode_ping_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_req) +
		   sizeof(struct pdu_data_llctrl_ping_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;
}

void helper_pdu_encode_ping_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_rsp) +
		   sizeof(struct pdu_data_llctrl_ping_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
}

void helper_pdu_encode_feature_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_req) +
		   sizeof(struct pdu_data_llctrl_feature_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_REQ;
	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];

		pdu->llctrl.feature_req.features[counter] = expected_value;
	}
}
void helper_pdu_encode_peripheral_feature_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_req) +
		   sizeof(struct pdu_data_llctrl_feature_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG;

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];

		pdu->llctrl.feature_req.features[counter] = expected_value;
	}
}

void helper_pdu_encode_feature_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_rsp *feature_rsp = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_rsp) +
		   sizeof(struct pdu_data_llctrl_feature_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_rsp->features[counter];

		pdu->llctrl.feature_req.features[counter] = expected_value;
	}
}

void helper_pdu_encode_min_used_chans_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_min_used_chans_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, min_used_chans_ind) +
		   sizeof(struct pdu_data_llctrl_min_used_chans_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND;
	pdu->llctrl.min_used_chans_ind.phys = p->phys;
	pdu->llctrl.min_used_chans_ind.min_used_chans = p->min_used_chans;
}

void helper_pdu_encode_version_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) +
		   sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
	pdu->llctrl.version_ind.version_number = p->version_number;
	pdu->llctrl.version_ind.company_id = p->company_id;
	pdu->llctrl.version_ind.sub_version_number = p->sub_version_number;
}

void helper_pdu_encode_enc_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
	memcpy(pdu->llctrl.enc_req.rand, p->rand, sizeof(pdu->llctrl.enc_req.rand));
	memcpy(pdu->llctrl.enc_req.ediv, p->ediv, sizeof(pdu->llctrl.enc_req.ediv));
	memcpy(pdu->llctrl.enc_req.skdm, p->skdm, sizeof(pdu->llctrl.enc_req.skdm));
	memcpy(pdu->llctrl.enc_req.ivm, p->ivm, sizeof(pdu->llctrl.enc_req.ivm));
}

void helper_pdu_encode_enc_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, enc_rsp) + sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
	memcpy(pdu->llctrl.enc_rsp.skds, p->skds, sizeof(pdu->llctrl.enc_rsp.skds));
	memcpy(pdu->llctrl.enc_rsp.ivs, p->ivs, sizeof(pdu->llctrl.enc_rsp.ivs));
}

void helper_pdu_encode_start_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_req) +
		   sizeof(struct pdu_data_llctrl_start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
}

void helper_pdu_encode_start_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_rsp) +
		   sizeof(struct pdu_data_llctrl_start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
}

void helper_pdu_encode_pause_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_req) +
		   sizeof(struct pdu_data_llctrl_pause_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ;
}

void helper_pdu_encode_pause_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_rsp) +
		   sizeof(struct pdu_data_llctrl_pause_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
}

void helper_pdu_encode_reject_ext_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) +
		   sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = p->reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = p->error_code;
}

void helper_pdu_encode_reject_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ind) +
		   sizeof(struct pdu_data_llctrl_reject_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
	pdu->llctrl.reject_ind.error_code = p->error_code;
}

void helper_pdu_encode_phy_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	pdu->llctrl.phy_req.rx_phys = p->rx_phys;
	pdu->llctrl.phy_req.tx_phys = p->tx_phys;
}

void helper_pdu_encode_phy_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	pdu->llctrl.phy_rsp.rx_phys = p->rx_phys;
	pdu->llctrl.phy_rsp.tx_phys = p->tx_phys;
}

void helper_pdu_encode_phy_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_upd_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_upd_ind) +
		   sizeof(struct pdu_data_llctrl_phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = p->instant;
	pdu->llctrl.phy_upd_ind.c_to_p_phy = p->c_to_p_phy;
	pdu->llctrl.phy_upd_ind.p_to_c_phy = p->p_to_c_phy;
}

void helper_pdu_encode_unknown_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, unknown_rsp) +
		   sizeof(struct pdu_data_llctrl_unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	pdu->llctrl.unknown_rsp.type = p->type;
}

void helper_pdu_encode_conn_param_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_conn_param_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_req) +
		   sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;

	pdu->llctrl.conn_param_req.interval_min = sys_cpu_to_le16(p->interval_min);
	pdu->llctrl.conn_param_req.interval_max = sys_cpu_to_le16(p->interval_max);
	pdu->llctrl.conn_param_req.latency = sys_cpu_to_le16(p->latency);
	pdu->llctrl.conn_param_req.timeout = sys_cpu_to_le16(p->timeout);
	pdu->llctrl.conn_param_req.preferred_periodicity = p->preferred_periodicity;
	pdu->llctrl.conn_param_req.reference_conn_event_count =
		sys_cpu_to_le16(p->reference_conn_event_count);
	pdu->llctrl.conn_param_req.offset0 = sys_cpu_to_le16(p->offset0);
	pdu->llctrl.conn_param_req.offset1 = sys_cpu_to_le16(p->offset1);
	pdu->llctrl.conn_param_req.offset2 = sys_cpu_to_le16(p->offset2);
	pdu->llctrl.conn_param_req.offset3 = sys_cpu_to_le16(p->offset3);
	pdu->llctrl.conn_param_req.offset4 = sys_cpu_to_le16(p->offset4);
	pdu->llctrl.conn_param_req.offset5 = sys_cpu_to_le16(p->offset5);
}

void helper_pdu_encode_conn_param_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_conn_param_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_rsp) +
		   sizeof(struct pdu_data_llctrl_conn_param_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;

	pdu->llctrl.conn_param_rsp.interval_min = sys_cpu_to_le16(p->interval_min);
	pdu->llctrl.conn_param_rsp.interval_max = sys_cpu_to_le16(p->interval_max);
	pdu->llctrl.conn_param_rsp.latency = sys_cpu_to_le16(p->latency);
	pdu->llctrl.conn_param_rsp.timeout = sys_cpu_to_le16(p->timeout);
	pdu->llctrl.conn_param_rsp.preferred_periodicity = p->preferred_periodicity;
	pdu->llctrl.conn_param_rsp.reference_conn_event_count =
		sys_cpu_to_le16(p->reference_conn_event_count);
	pdu->llctrl.conn_param_rsp.offset0 = sys_cpu_to_le16(p->offset0);
	pdu->llctrl.conn_param_rsp.offset1 = sys_cpu_to_le16(p->offset1);
	pdu->llctrl.conn_param_rsp.offset2 = sys_cpu_to_le16(p->offset2);
	pdu->llctrl.conn_param_rsp.offset3 = sys_cpu_to_le16(p->offset3);
	pdu->llctrl.conn_param_rsp.offset4 = sys_cpu_to_le16(p->offset4);
	pdu->llctrl.conn_param_rsp.offset5 = sys_cpu_to_le16(p->offset5);
}

void helper_pdu_encode_conn_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_conn_update_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_update_ind) +
		   sizeof(struct pdu_data_llctrl_conn_update_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;

	pdu->llctrl.conn_update_ind.win_size = p->win_size;
	pdu->llctrl.conn_update_ind.win_offset = sys_cpu_to_le16(p->win_offset);
	pdu->llctrl.conn_update_ind.interval = sys_cpu_to_le16(p->interval);
	pdu->llctrl.conn_update_ind.latency = sys_cpu_to_le16(p->latency);
	pdu->llctrl.conn_update_ind.timeout = sys_cpu_to_le16(p->timeout);
	pdu->llctrl.conn_update_ind.instant = sys_cpu_to_le16(p->instant);
}

void helper_pdu_encode_terminate_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_terminate_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, terminate_ind) +
		   sizeof(struct pdu_data_llctrl_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
	pdu->llctrl.terminate_ind.error_code = p->error_code;
}

void helper_pdu_encode_channel_map_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_chan_map_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, chan_map_ind) +
		   sizeof(struct pdu_data_llctrl_chan_map_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
	pdu->llctrl.chan_map_ind.instant = p->instant;
	memcpy(pdu->llctrl.chan_map_ind.chm, p->chm, sizeof(pdu->llctrl.chan_map_ind.chm));
}

void helper_pdu_encode_length_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_length_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_req) +
		   sizeof(struct pdu_data_llctrl_length_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ;

	pdu->llctrl.length_req.max_rx_octets = p->max_rx_octets;
	pdu->llctrl.length_req.max_tx_octets = p->max_tx_octets;
	pdu->llctrl.length_req.max_rx_time = p->max_rx_time;
	pdu->llctrl.length_req.max_tx_time = p->max_tx_time;
}

void helper_pdu_encode_length_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_length_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_rsp) +
		   sizeof(struct pdu_data_llctrl_length_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

	pdu->llctrl.length_req.max_rx_octets = p->max_rx_octets;
	pdu->llctrl.length_req.max_tx_octets = p->max_tx_octets;
	pdu->llctrl.length_req.max_rx_time = p->max_rx_time;
	pdu->llctrl.length_req.max_tx_time = p->max_tx_time;
}
void helper_pdu_encode_cte_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cte_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, cte_req) + sizeof(struct pdu_data_llctrl_cte_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ;
	pdu->llctrl.cte_req.min_cte_len_req = p->min_cte_len_req;
	pdu->llctrl.cte_req.cte_type_req = p->cte_type_req;
}

void helper_pdu_encode_cte_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len =
		offsetof(struct pdu_data_llctrl, cte_rsp) + sizeof(struct pdu_data_llctrl_cte_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CTE_RSP;
}

void helper_pdu_encode_zero(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = 0;
}

void helper_node_encode_cte_rsp(struct node_rx_pdu *rx, void *param)
{
	rx->hdr.rx_ftr.iq_report = (struct cte_conn_iq_report *)param;
}

void helper_pdu_encode_cis_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_req) +
		   sizeof(struct pdu_data_llctrl_cis_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ;

	pdu->llctrl.cis_req.cig_id           =  p->cig_id;
	pdu->llctrl.cis_req.cis_id           =  p->cis_id;
	pdu->llctrl.cis_req.c_phy            =  p->c_phy;
	pdu->llctrl.cis_req.p_phy            =  p->p_phy;
	pdu->llctrl.cis_req.c_max_pdu        =  p->c_max_pdu;
	pdu->llctrl.cis_req.p_max_pdu        =  p->p_max_pdu;
	pdu->llctrl.cis_req.nse              =  p->nse;
	pdu->llctrl.cis_req.p_bn             =  p->p_bn;
	pdu->llctrl.cis_req.c_bn             =  p->c_bn;
	pdu->llctrl.cis_req.c_ft             =  p->c_ft;
	pdu->llctrl.cis_req.p_ft             =  p->p_ft;
	pdu->llctrl.cis_req.iso_interval     =  p->iso_interval;
	pdu->llctrl.cis_req.conn_event_count = p->conn_event_count;
	memcpy(pdu->llctrl.cis_req.c_max_sdu_packed, p->c_max_sdu_packed,
	       sizeof(p->c_max_sdu_packed));
	memcpy(pdu->llctrl.cis_req.p_max_sdu, p->p_max_sdu, sizeof(p->p_max_sdu));
	memcpy(pdu->llctrl.cis_req.c_sdu_interval, p->c_sdu_interval, sizeof(p->c_sdu_interval));
	memcpy(pdu->llctrl.cis_req.p_sdu_interval, p->p_sdu_interval, sizeof(p->p_sdu_interval));
	memcpy(pdu->llctrl.cis_req.sub_interval, p->sub_interval, sizeof(p->sub_interval));
	memcpy(pdu->llctrl.cis_req.cis_offset_min, p->cis_offset_min, sizeof(p->cis_offset_min));
	memcpy(pdu->llctrl.cis_req.cis_offset_max, p->cis_offset_max, sizeof(p->cis_offset_max));
}

void helper_pdu_encode_cis_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_rsp) +
		   sizeof(struct pdu_data_llctrl_cis_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_RSP;
	memcpy(pdu->llctrl.cis_rsp.cis_offset_min, p->cis_offset_min, sizeof(p->cis_offset_min));
	memcpy(pdu->llctrl.cis_rsp.cis_offset_max, p->cis_offset_max, sizeof(p->cis_offset_max));
	pdu->llctrl.cis_rsp.conn_event_count = p->conn_event_count;
}

void helper_pdu_encode_cis_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_ind) +
		   sizeof(struct pdu_data_llctrl_cis_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_IND;
	memcpy(pdu->llctrl.cis_ind.aa, p->aa, sizeof(p->aa));
	memcpy(pdu->llctrl.cis_ind.cis_offset, p->cis_offset, sizeof(p->cis_offset));
	memcpy(pdu->llctrl.cis_ind.cig_sync_delay, p->cig_sync_delay, sizeof(p->cig_sync_delay));
	memcpy(pdu->llctrl.cis_ind.cis_sync_delay, p->cis_sync_delay, sizeof(p->cis_sync_delay));
	pdu->llctrl.cis_ind.conn_event_count = p->conn_event_count;
}

void helper_pdu_encode_cis_terminate_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_terminate_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_terminate_ind) +
		   sizeof(struct pdu_data_llctrl_cis_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_TERMINATE_IND;
	pdu->llctrl.cis_terminate_ind.cig_id = p->cig_id;
	pdu->llctrl.cis_terminate_ind.cis_id = p->cis_id;
	pdu->llctrl.cis_terminate_ind.error_code = p->error_code;
}

void helper_pdu_verify_version_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_VERSION_IND,
		      "Not a LL_VERSION_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.version_number, p->version_number,
		      "Wrong version number.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.company_id, p->company_id,
		      "Wrong company id.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.sub_version_number, p->sub_version_number,
		      "Wrong sub version number.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_ping_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PING_REQ,
		      "Not a LL_PING_REQ. Called at %s:%d\n", file, line);
}

void helper_pdu_verify_ping_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PING_RSP,
		      "Not a LL_PING_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_feature_req(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_FEATURE_REQ,
		      "Wrong opcode.\nCalled at %s:%d\n", file, line);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];

		zassert_equal(pdu->llctrl.feature_req.features[counter], expected_value,
			      "Wrong feature exchange data.\nAt %s:%d\n", file, line);
	}
}

void helper_pdu_verify_peripheral_feature_req(const char *file, uint32_t line, struct pdu_data *pdu,
					      void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG, NULL);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];

		zassert_equal(pdu->llctrl.feature_req.features[counter], expected_value,
			      "Wrong feature data\nCalled at %s:%d\n", file, line);
	}
}

void helper_pdu_verify_feature_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	struct pdu_data_llctrl_feature_rsp *feature_rsp = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_FEATURE_RSP,
		      "Response: %d Expected: %d\n", pdu->llctrl.opcode,
		      PDU_DATA_LLCTRL_TYPE_FEATURE_RSP);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_rsp->features[counter];

		zassert_equal(pdu->llctrl.feature_rsp.features[counter], expected_value,
			      "Wrong feature data\nCalled at %s:%d\n", file, line);
	}
}

void helper_pdu_verify_min_used_chans_ind(const char *file, uint32_t line, struct pdu_data *pdu,
					  void *param)
{
	struct pdu_data_llctrl_min_used_chans_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND,
		      "Not a MIN_USED_CHAN_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.min_used_chans_ind.phys, p->phys, "Wrong PHY.\nCalled at %s:%d\n",
		      file, line);
	zassert_equal(pdu->llctrl.min_used_chans_ind.min_used_chans, p->min_used_chans,
		      "Channel count\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		      "Not a LL_ENC_REQ. Called at %s:%d\n", file, line);

	PDU_MEM_EQUAL(rand, pdu->llctrl.enc_req, p, "Rand mismatch.");
	PDU_MEM_EQUAL(ediv, pdu->llctrl.enc_req, p, "EDIV mismatch.");
	PDU_MEM_EQUAL(skdm, pdu->llctrl.enc_req, p, "SKDm mismatch.");
	PDU_MEM_EQUAL(ivm, pdu->llctrl.enc_req, p, "IVm mismatch.");
}

void helper_pdu_ntf_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		      "Not a LL_ENC_REQ. Called at %s:%d\n", file, line);

	PDU_MEM_EQUAL(rand, pdu->llctrl.enc_req, p, "Rand mismatch.");
	PDU_MEM_EQUAL(ediv, pdu->llctrl.enc_req, p, "EDIV mismatch.");
}

void helper_pdu_verify_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_RSP,
		      "Not a LL_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ,
		      "Not a LL_START_ENC_REQ.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP,
		      "Not a LL_START_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_pause_enc_req(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ,
		      "Not a LL_PAUSE_ENC_REQ.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_pause_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP,
		      "Not a LL_PAUSE_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_enc_refresh(const char *file, uint32_t line, struct node_rx_pdu *rx,
				    void *param)
{
	zassert_equal(rx->hdr.type, NODE_RX_TYPE_ENC_REFRESH,
		      "Not an ENC_REFRESH node.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, reject_ind) +
			      sizeof(struct pdu_data_llctrl_reject_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_IND,
		      "Not a LL_REJECT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ind.error_code, p->error_code,
		      "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ext_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, reject_ext_ind) +
			      sizeof(struct pdu_data_llctrl_reject_ext_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND,
		      "Not a LL_REJECT_EXT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.reject_opcode, p->reject_opcode,
		      "Reject opcode mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.error_code, p->error_code,
		      "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_phy_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, phy_req) +
			      sizeof(struct pdu_data_llctrl_phy_req),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_REQ,
		      "Not a LL_PHY_REQ.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_req.rx_phys, p->rx_phys,
		      "rx phys mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_req.tx_phys, p->tx_phys,
		      "tx phys mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_phy_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, phy_rsp) +
			      sizeof(struct pdu_data_llctrl_phy_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_RSP,
		      "Not a LL_PHY_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_rsp.rx_phys, p->rx_phys,
		      "rx phys mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_rsp.tx_phys, p->tx_phys,
		      "tx phys mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_phy_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param)
{
	struct pdu_data_llctrl_phy_upd_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, phy_upd_ind) +
			      sizeof(struct pdu_data_llctrl_phy_upd_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND,
		      "Not a LL_PHY_UPDATE_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_upd_ind.instant, p->instant,
		      "instant mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_upd_ind.c_to_p_phy, p->c_to_p_phy,
		      "c_to_p_phy mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.phy_upd_ind.p_to_c_phy, p->p_to_c_phy,
		      "p_to_c_phy mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_phy_update(const char *file, uint32_t line, struct node_rx_pdu *rx,
				   void *param)
{
	struct node_rx_pu *pdu = (struct node_rx_pu *)rx->pdu;
	struct node_rx_pu *p = param;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_PHY_UPDATE,
		      "Not a PHY_UPDATE node.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->status, p->status, "Status mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_unknown_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, unknown_rsp) +
			      sizeof(struct pdu_data_llctrl_unknown_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP,
		      "Not a LL_UNKNOWN_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.unknown_rsp.type, p->type, "Type mismatch.\nCalled at %s:%d\n",
		      file, line);
}

void helper_pdu_verify_conn_param_req(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param)
{
	struct pdu_data_llctrl_conn_param_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, conn_param_req) +
			      sizeof(struct pdu_data_llctrl_conn_param_req),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		      "Not a LL_CONNECTION_PARAM_REQ.\nCalled at %s:%d\n", file, line);

	zassert_equal(pdu->llctrl.conn_param_req.interval_min, p->interval_min,
		      "Interval_min mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.interval_max, p->interval_max,
		      "Interval_max mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.latency, p->latency,
		      "Latency mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.timeout, p->timeout,
		      "Timeout mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.preferred_periodicity, p->preferred_periodicity,
		      "Preferred_periodicity mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.reference_conn_event_count,
		      p->reference_conn_event_count,
		      "Reference_conn_event_count mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset0, p->offset0,
		      "Offset0 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset1, p->offset1,
		      "Offset1 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset2, p->offset2,
		      "Offset2 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset3, p->offset3,
		      "Offset3 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset4, p->offset4,
		      "Offset4 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_req.offset5, p->offset5,
		      "Offset5 mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_conn_param_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				      void *param)
{
	struct pdu_data_llctrl_conn_param_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, conn_param_rsp) +
			      sizeof(struct pdu_data_llctrl_conn_param_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
		      "Not a LL_CONNECTION_PARAM_RSP.\nCalled at %s:%d\n", file, line);

	zassert_equal(pdu->llctrl.conn_param_rsp.interval_min, p->interval_min,
		      "Interval_min mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.interval_max, p->interval_max,
		      "Interval_max mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.latency, p->latency,
		      "Latency mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.timeout, p->timeout,
		      "Timeout mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.preferred_periodicity, p->preferred_periodicity,
		      "Preferred_periodicity mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.reference_conn_event_count,
		      p->reference_conn_event_count,
		      "Reference_conn_event_count mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset0, p->offset0,
		      "Offset0 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset1, p->offset1,
		      "Offset1 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset2, p->offset2,
		      "Offset2 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset3, p->offset3,
		      "Offset3 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset4, p->offset4,
		      "Offset4 mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_param_rsp.offset5, p->offset5,
		      "Offset5 mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_conn_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				       void *param)
{
	struct pdu_data_llctrl_conn_update_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, conn_update_ind) +
			      sizeof(struct pdu_data_llctrl_conn_update_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND,
		      "Not a LL_CONNECTION_UPDATE_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.win_size, p->win_size,
		      "Win_size mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.win_offset, p->win_offset,
		      "Win_offset mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.latency, p->latency,
		      "Latency.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.interval, p->interval,
		      "Interval mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.timeout, p->timeout,
		      "Timeout mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.conn_update_ind.instant, p->instant,
		      "Instant mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_conn_update(const char *file, uint32_t line, struct node_rx_pdu *rx,
				    void *param)
{
	struct node_rx_pu *pdu = (struct node_rx_pu *)rx->pdu;
	struct node_rx_pu *p = param;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_CONN_UPDATE,
		      "Not a CONN_UPDATE node.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->status, p->status, "Status mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_terminate_ind(const char *file, uint32_t line, struct pdu_data *pdu,
				     void *param)
{
	struct pdu_data_llctrl_terminate_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, terminate_ind) +
			      sizeof(struct pdu_data_llctrl_terminate_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_TERMINATE_IND,
		      "Not a LL_TERMINATE_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.terminate_ind.error_code, p->error_code,
		      "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_channel_map_update_ind(const char *file, uint32_t line, struct pdu_data *pdu,
					      void *param)
{
	struct pdu_data_llctrl_chan_map_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND,
		      "Not a LL_CHANNEL_MAP_UPDATE_IND.\nCalled at %s:%d ( %d %d)\n", file, line,
		      pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, chan_map_ind) +
			      sizeof(struct pdu_data_llctrl_chan_map_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.chan_map_ind.instant, p->instant,
		      "Instant mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.chan_map_ind.chm, p->chm, sizeof(p->chm),
			  "Channel Map mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_length_req(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param)
{
	struct pdu_data_llctrl_length_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, length_req) +
			      sizeof(struct pdu_data_llctrl_length_req),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_LENGTH_REQ,
		      "Not a LL_LENGTH_REQ.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_rx_octets, p->max_rx_octets,
		      "max_rx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_tx_octets, p->max_tx_octets,
		      "max_tx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_rx_time, p->max_rx_time,
		      "max_rx_time mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_tx_time, p->max_tx_time,
		      "max_tx_time mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_length_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				  void *param)
{
	struct pdu_data_llctrl_length_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, length_rsp) +
			      sizeof(struct pdu_data_llctrl_length_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_LENGTH_RSP,
		      "Not a LL_LENGTH_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_rx_octets, p->max_rx_octets,
		      "max_rx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_tx_octets, p->max_tx_octets,
		      "max_tx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_rx_time, p->max_rx_time,
		      "max_rx_time mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_tx_time, p->max_tx_time,
		      "max_tx_time mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_cte_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cte_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CTE_REQ,
		      "Not a LL_CTE_REQ.\nCalled at %s:%d ( %d %d)\n", file, line,
		      pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CTE_REQ);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, cte_req) +
			      sizeof(struct pdu_data_llctrl_cte_req),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cte_req.min_cte_len_req, p->min_cte_len_req,
		      "Minimal CTE length request mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cte_req.cte_type_req, p->cte_type_req,
		      "CTE type request mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_cte_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, cte_rsp) +
			      sizeof(struct pdu_data_llctrl_cte_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CTE_RSP,
		      "Not a LL_CTE_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_cte_rsp(const char *file, uint32_t line, struct node_rx_pdu *rx,
				void *param)
{
	struct cte_conn_iq_report *p_iq_report = param;
	struct cte_conn_iq_report *rx_iq_report = rx->hdr.rx_ftr.iq_report;

	zassert_equal(rx_iq_report->cte_info.time, p_iq_report->cte_info.time,
		      "CTE Time mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(rx_iq_report->local_slot_durations, p_iq_report->local_slot_durations,
		      "Slot duration mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(rx_iq_report->packet_status, p_iq_report->packet_status,
		      "Packet status mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(rx_iq_report->rssi_ant_id, p_iq_report->rssi_ant_id,
		      "RSSI antenna id mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(rx_iq_report->sample_count, p_iq_report->sample_count,
		      "Sample count mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(memcmp(rx_iq_report->sample, p_iq_report->sample, p_iq_report->sample_count),
		      0, "IQ samples mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_ntf_verify_cte_rsp(const char *file, uint32_t line, struct pdu_data *pdu,
				   void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CTE_RSP,
		      "Not a LL_CTE_RSP. Called at %s:%d\n", file, line);
}

void helper_node_verify_cis_request(const char *file, uint32_t line, struct node_rx_pdu *rx,
				    void *param)
{
	struct node_rx_conn_iso_req *p = (struct node_rx_conn_iso_req *)param;
	struct node_rx_conn_iso_req *pdu = (struct node_rx_conn_iso_req *)rx->pdu;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_CIS_REQUEST,
		      "Not a CIS_REQUEST node.\nCalled at %s:%d\n", file, line);
	zassert_equal(p->cig_id, pdu->cig_id,
		      "cig_id mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(p->cis_handle, pdu->cis_handle,
		      "cis_handle mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(p->cis_id, pdu->cis_id,
		      "cis_id mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_cis_established(const char *file, uint32_t line, struct node_rx_pdu *rx,
					void *param)
{
	struct node_rx_conn_iso_estab *p = (struct node_rx_conn_iso_estab *)param;
	struct node_rx_conn_iso_estab *pdu = (struct node_rx_conn_iso_estab *)rx->pdu;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_CIS_ESTABLISHED,
		      "Not a CIS_ESTABLISHED node.\nCalled at %s:%d\n", file, line);
	zassert_equal(p->status, pdu->status,
		      "status mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(p->cis_handle, pdu->cis_handle,
		      "cis_handle mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_cis_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, cis_req) +
		   sizeof(struct pdu_data_llctrl_cis_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ;

	zassert_equal(pdu->llctrl.cis_req.cig_id, p->cig_id,
		      "cig_id mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.cis_id, p->cis_id,
		      "cis_id mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.c_phy, p->c_phy,
		      "c_phy mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.p_phy, p->p_phy,
		      "p_phy mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.c_max_pdu, p->c_max_pdu,
		      "c_max_pdu mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.p_max_pdu, p->p_max_pdu,
		      "p_max_pdu mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.nse, p->nse,
		      "nse mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.p_bn, p->p_bn,
		      "p_bn mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.c_bn, p->c_bn,
		      "c_bn mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.c_ft, p->c_ft,
		      "c_ft mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.p_ft, p->p_ft,
		      "p_ft mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.iso_interval, p->iso_interval,
		      "iso_interval mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_req.conn_event_count, p->conn_event_count,
		      "conn_event_count mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.c_max_sdu_packed, p->c_max_sdu_packed,
			  sizeof(p->c_max_sdu_packed),
			  "c_max_sdu_packed mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.p_max_sdu, p->p_max_sdu,
			  sizeof(p->p_max_sdu),
			  "p_max_sdu mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.c_sdu_interval, p->c_sdu_interval,
			  sizeof(p->c_sdu_interval),
			  "c_sdu_interval mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.p_sdu_interval, p->p_sdu_interval,
			  sizeof(p->p_sdu_interval),
			  "p_sdu_interval mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.sub_interval, p->sub_interval,
			  sizeof(p->sub_interval),
			  "sub_interval mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.cis_offset_min, p->cis_offset_min,
			  sizeof(p->cis_offset_min),
			  "cis_offset_min mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_req.cis_offset_max, p->cis_offset_max,
			  sizeof(p->cis_offset_max),
			  "cis_offset_max mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_cis_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, cis_rsp) +
			      sizeof(struct pdu_data_llctrl_cis_rsp),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CIS_RSP,
		      "Not a LL_CIS_RSP.\nCalled at %s:%d\n", file, line);

	zassert_mem_equal(pdu->llctrl.cis_rsp.cis_offset_min, p->cis_offset_min,
			  sizeof(p->cis_offset_min),
			  "cis_offset_min mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_rsp.cis_offset_max, p->cis_offset_max,
			  sizeof(p->cis_offset_max),
			  "cis_offset_max mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_rsp.conn_event_count, p->conn_event_count,
		      "conn_event_count mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_cis_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_cis_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, cis_ind) +
			      sizeof(struct pdu_data_llctrl_cis_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CIS_IND,
		      "Not a LL_CIS_IND.\nCalled at %s:%d\n", file, line);

	zassert_mem_equal(pdu->llctrl.cis_ind.aa, p->aa, sizeof(p->aa),
			  "aa mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_ind.cis_offset, p->cis_offset, sizeof(p->cis_offset),
			  "cis_offset mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_ind.cig_sync_delay, p->cig_sync_delay,
			  sizeof(p->cig_sync_delay),
			  "cig_sync_delay mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.cis_ind.cis_sync_delay, p->cis_sync_delay,
			  sizeof(p->cis_sync_delay),
			  "cis_sync_delay mismatch.\nCalled at %s:%d\n", file, line);

	pdu->llctrl.cis_ind.conn_event_count = p->conn_event_count;
}

void helper_pdu_verify_cis_terminate_ind(const char *file, uint32_t line, struct pdu_data *pdu,
					 void *param)
{
	struct pdu_data_llctrl_cis_terminate_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file,
		      line);
	zassert_equal(pdu->len,
		      offsetof(struct pdu_data_llctrl, cis_terminate_ind) +
			      sizeof(struct pdu_data_llctrl_cis_terminate_ind),
		      "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CIS_TERMINATE_IND,
		      "Not a LL_CIS_TERMINATE_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_terminate_ind.cig_id, p->cig_id,
		      "CIG ID mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_terminate_ind.cis_id, p->cis_id,
		      "CIS ID mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.cis_terminate_ind.error_code, p->error_code,
		      "Error code mismatch.\nCalled at %s:%d\n", file, line);
}
