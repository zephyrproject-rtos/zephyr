/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/types.h"
#include "ztest.h"
#include "kconfig.h"

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
#include "ll_feat.h"

#include "lll.h"
#include "lll_conn.h"
#include "ull_tx_queue.h"
#include "ull_conn_types.h"

#include "ull_llcp.h"


#include "helper_pdu.h"
#include "helper_features.h"

#define PDU_MEM_EQUAL(_f, _s, _p, _t) zassert_mem_equal(_s._f, _p->_f, sizeof(_p->_f), _t "\nCalled at %s:%d\n", file, line);

void helper_pdu_encode_ping_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_req) + sizeof(struct pdu_data_llctrl_ping_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ;
}

void helper_pdu_encode_ping_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, ping_rsp) + sizeof(struct pdu_data_llctrl_ping_rsp);
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
void helper_pdu_encode_slave_feature_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, feature_req) +
		sizeof(struct pdu_data_llctrl_feature_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ;

	for (int counter = 0 ; counter < 8; counter++)	{
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
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) + sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
	pdu->llctrl.version_ind.version_number = p->version_number;
	pdu->llctrl.version_ind.company_id = p->company_id;
	pdu->llctrl.version_ind.sub_version_number = p->sub_version_number;
}

void helper_pdu_encode_enc_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
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
	pdu->len = offsetof(struct pdu_data_llctrl, enc_rsp) + sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
	memcpy(pdu->llctrl.enc_rsp.skds, p->skds, sizeof(pdu->llctrl.enc_rsp.skds));
	memcpy(pdu->llctrl.enc_rsp.ivs, p->ivs, sizeof(pdu->llctrl.enc_rsp.ivs));
}

void helper_pdu_encode_start_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_req) + sizeof(struct pdu_data_llctrl_start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
}

void helper_pdu_encode_start_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_rsp) + sizeof(struct pdu_data_llctrl_start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
}

void helper_pdu_encode_pause_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_req) + sizeof(struct pdu_data_llctrl_pause_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ;
}

void helper_pdu_encode_pause_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, pause_enc_rsp) + sizeof(struct pdu_data_llctrl_pause_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
}

void helper_pdu_encode_reject_ext_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = p->reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = p->error_code;
}

void helper_pdu_encode_reject_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_IND;
	pdu->llctrl.reject_ind.error_code = p->error_code;
}


void helper_pdu_encode_phy_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_phy_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_phy_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_upd_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = p->instant;
	/* TODO(thoh): Fill in correct data */
}


void helper_pdu_encode_unknown_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, unknown_rsp) + sizeof(struct pdu_data_llctrl_unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	pdu->llctrl.unknown_rsp.type = p->type;
}

void helper_pdu_encode_conn_param_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_req) + sizeof(struct pdu_data_llctrl_conn_param_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_conn_param_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_param_rsp) + sizeof(struct pdu_data_llctrl_conn_param_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_conn_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_conn_update_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, conn_update_ind) + sizeof(struct pdu_data_llctrl_conn_update_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
	pdu->llctrl.conn_update_ind.instant = sys_cpu_to_le16(p->instant);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_terminate_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_terminate_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, terminate_ind) + sizeof(struct pdu_data_llctrl_terminate_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
	pdu->llctrl.terminate_ind.error_code = p->error_code;
}

void helper_pdu_encode_channel_map_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_chan_map_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, chan_map_ind) + sizeof(struct pdu_data_llctrl_chan_map_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND;
	pdu->llctrl.chan_map_ind.instant = p->instant;
	memcpy(pdu->llctrl.chan_map_ind.chm, p->chm, sizeof(pdu->llctrl.chan_map_ind.chm));
}

void helper_pdu_encode_length_req(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_length_req *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, length_req) + sizeof(struct pdu_data_llctrl_length_req);
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
	pdu->len = offsetof(struct pdu_data_llctrl, length_rsp) + sizeof(struct pdu_data_llctrl_length_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;

	pdu->llctrl.length_req.max_rx_octets = p->max_rx_octets;
	pdu->llctrl.length_req.max_tx_octets = p->max_tx_octets;
	pdu->llctrl.length_req.max_rx_time = p->max_rx_time;
	pdu->llctrl.length_req.max_tx_time = p->max_tx_time;
}

void helper_pdu_verify_version_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_VERSION_IND, "Not a LL_VERSION_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.version_number, p->version_number, "Wrong version number.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.company_id, p->company_id, "Wrong company id.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.sub_version_number, p->sub_version_number, "Wrong sub version number.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_ping_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PING_REQ, "Not a LL_PING_REQ. Called at %s:%d\n", file, line);
}

void helper_pdu_verify_ping_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PING_RSP, "Not a LL_PING_RSP.\nCalled at %s:%d\n", file, line);
}


void helper_pdu_verify_feature_req(const char *file, uint32_t line,
				   struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_FEATURE_REQ,
		      "Wrong opcode.\nCalled at %s:%d\n", file, line);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];
		zassert_equal(pdu->llctrl.feature_req.features[counter],
			      expected_value,
			      "Wrong feature exchange data.\nAt %s:%d\n",
			      file, line);
	}
}

void helper_pdu_verify_slave_feature_req(const char *file, uint32_t line,
					 struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_req *feature_req = param;

		zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ, NULL);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_req->features[counter];
		zassert_equal(pdu->llctrl.feature_req.features[counter],
			      expected_value,
			      "Wrong feature data\nCalled at %s:%d\n",
			      file, line);
	}
}
void helper_pdu_verify_feature_rsp(const char *file, uint32_t line,
				   struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_feature_rsp *feature_rsp = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_FEATURE_RSP,
		      "Response: %d Expected: %d\n",
		      pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_FEATURE_RSP);

	for (int counter = 0; counter < 8; counter++) {
		uint8_t expected_value = feature_rsp->features[counter];
		zassert_equal(pdu->llctrl.feature_rsp.features[counter],
			      expected_value,
			      "Wrong feature data\nCalled at %s:%d\n",
			      file, line);
	}
}

void helper_pdu_verify_min_used_chans_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_min_used_chans_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND, "Not a MIN_USED_CHAN_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.min_used_chans_ind.phys, p->phys, "Wrong PHY.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.min_used_chans_ind.min_used_chans, p->min_used_chans, "Channel count\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ, "Not a LL_ENC_REQ. Called at %s:%d\n", file, line);

	PDU_MEM_EQUAL(rand, pdu->llctrl.enc_req, p, "Rand mismatch.");
	PDU_MEM_EQUAL(ediv, pdu->llctrl.enc_req, p, "EDIV mismatch.");
	PDU_MEM_EQUAL(skdm, pdu->llctrl.enc_req, p, "SKDm mismatch.");
	PDU_MEM_EQUAL(ivm, pdu->llctrl.enc_req, p, "IVm mismatch.");
}

void helper_pdu_ntf_verify_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_enc_req *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ, "Not a LL_ENC_REQ. Called at %s:%d\n", file, line);

	PDU_MEM_EQUAL(rand, pdu->llctrl.enc_req, p, "Rand mismatch.");
	PDU_MEM_EQUAL(ediv, pdu->llctrl.enc_req, p, "EDIV mismatch.");
}

void helper_pdu_verify_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_RSP, "Not a LL_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ, "Not a LL_START_ENC_REQ.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP, "Not a LL_START_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_pause_enc_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ, "Not a LL_PAUSE_ENC_REQ.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_pause_enc_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP, "Not a LL_PAUSE_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_enc_refresh(const char *file, uint32_t line, struct node_rx_pdu *rx, void *param)
{
	zassert_equal(rx->hdr.type, NODE_RX_TYPE_ENC_REFRESH, "Not an ENC_REFRESH node.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_IND, "Not a LL_REJECT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ind.error_code, p->error_code, "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ext_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND, "Not a LL_REJECT_EXT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.reject_opcode, p->reject_opcode, "Reject opcode mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.error_code, p->error_code, "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_phy_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_REQ, "Not a LL_PHY_REQ.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_phy_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_RSP, "Not a LL_PHY_RSP.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_phy_update_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND, "Not a LL_PHY_UPDATE_IND.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_node_verify_phy_update(const char *file, uint32_t line, struct node_rx_pdu *rx, void *param)
{
	struct node_rx_pu *pdu = (struct node_rx_pu *)rx->pdu;
	struct node_rx_pu *p = param;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_PHY_UPDATE, "Not a PHY_UPDATE node.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->status, p->status, "Status mismatch.\nCalled at %s:%d\n", file, line);
}


void helper_pdu_verify_unknown_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, unknown_rsp) + sizeof(struct pdu_data_llctrl_unknown_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP, "Not a LL_UNKNOWN_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.unknown_rsp.type, p->type, "Type mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_conn_param_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, conn_param_req) + sizeof(struct pdu_data_llctrl_conn_param_req), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ, "Not a LL_CONNECTION_PARAM_REQ.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_conn_param_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, conn_param_rsp) + sizeof(struct pdu_data_llctrl_conn_param_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP, "Not a LL_CONNECTION_PARAM_RSP.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_conn_update_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, conn_update_ind) + sizeof(struct pdu_data_llctrl_conn_update_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND, "Not a LL_CONNECTION_UPDATE_IND.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_node_verify_conn_update(const char *file, uint32_t line, struct node_rx_pdu *rx, void *param)
{
	struct node_rx_pu *pdu = (struct node_rx_pu *)rx->pdu;
	struct node_rx_pu *p = param;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_CONN_UPDATE, "Not a CONN_UPDATE node.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->status, p->status, "Status mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_terminate_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_terminate_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, terminate_ind) + sizeof(struct pdu_data_llctrl_terminate_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_TERMINATE_IND, "Not a LL_TERMINATE_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.terminate_ind.error_code, p->error_code, "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_channel_map_update_ind(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_chan_map_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND, "Not a LL_CHANNEL_MAP_UPDATE_IND.\nCalled at %s:%d ( %d %d)\n", file, line, pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, chan_map_ind) + sizeof(struct pdu_data_llctrl_chan_map_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.chan_map_ind.instant, p->instant, "Instant mismatch.\nCalled at %s:%d\n", file, line);
	zassert_mem_equal(pdu->llctrl.chan_map_ind.chm, p->chm, sizeof(p->chm), "Channel Map mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_length_req(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_length_req *p = param;
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, length_req) + sizeof(struct pdu_data_llctrl_length_req), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_LENGTH_REQ, "Not a LL_LENGTH_REQ.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_rx_octets, p->max_rx_octets, "max_rx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_tx_octets, p->max_tx_octets, "max_tx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_rx_time, p->max_rx_time, "max_rx_time mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_req.max_tx_time, p->max_tx_time, "max_tx_time mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_length_rsp(const char *file, uint32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_length_rsp *p = param;
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, length_rsp) + sizeof(struct pdu_data_llctrl_length_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_LENGTH_RSP, "Not a LL_LENGTH_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_rx_octets, p->max_rx_octets, "max_rx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_tx_octets, p->max_tx_octets, "max_tx_octets mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_rx_time, p->max_rx_time, "max_rx_time mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.length_rsp.max_tx_time, p->max_tx_time, "max_tx_time mismatch.\nCalled at %s:%d\n", file, line);
}

