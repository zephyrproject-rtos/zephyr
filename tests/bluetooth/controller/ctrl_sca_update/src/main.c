/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"
#include "ull_llcp_features.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;

static void sca_setup(void *data)
{
	test_setup(&conn);
}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                                |
 *    | Start                      |                                |
 *    | SCA Update Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_CLOCK_ACCURACY_REQ          |
 *    |                            |------------------------------->|
 *    |                            |                                |
 *    |                            |    LL_CLOCK_ACCURACY_RSP       |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 *    | Start                      |                                |
 *    | SCA UPdate Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_CLOCK_ACCURACY_REQ          |
 *    |                            |------------------------------->|
 *    |                            |                                |
 *    |                            |    LL_UNKNOWN_RSP              |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 */
ZTEST(sca_central, test_sca_central_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_sca scau = { .status = BT_HCI_ERR_SUCCESS, .sca = 2 };
	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = { };
	struct pdu_data_llctrl_clock_accuracy_rsp remote_sca_rsp = { .sca = 2 };
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Confirm SCA Update is indicated as supported */
	zassert_equal(feature_sca(&conn), true, "SCA Update Feature masked out");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CLOCK_ACCURACY_RSP, &conn, &remote_sca_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination not 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, 0,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should be one notification due to Peer SCA Request */
	ut_rx_node(NODE_PEER_SCA_UPDATE, &ntf, &scau);
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	/* Initiate another SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Confirm SCA Update is now indicated as NOT supported */
	zassert_equal(feature_sca(&conn), false, "SCA Update Feature masked in");

	/* Termination not 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, 0,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	scau.status = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	scau.sca = 0;

	/* There should be one notification due to Peer SCA Request */
	ut_rx_node(NODE_PEER_SCA_UPDATE, &ntf, &scau);
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/* +-----+                     +-------+                   +-----+
 * | UT  |                     | LL_A  |                   | LT  |
 * +-----+                     +-------+                   +-----+
 *    |                            |                          |
 *    | Start                      |                          |
 *    | SCA Update Proc.           |                          |
 *    |--------------------------->|                          |
 *    |                            |                          |
 *    |                            | LL_CLOCK_ACCURACY_REQ    |
 *    |                            |------------------------->|
 *    |                            |                          |
 *    |                            | LL_<INVALID>_RSP         |
 *    |                            |<-------------------------|
 *    |                            |                          |
 *       ~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~
 *    |                            |                          |
 */
ZTEST(sca_central, test_sca_central_loc_invalid_rsp)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	/* Initiate another SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/* +-----+                     +-------+                   +-----+
 * | UT  |                     | LL_A  |                   | LT  |
 * +-----+                     +-------+                   +-----+
 *    |                            |                          |
 *    | Start                      |                          |
 *    | SCA Update Proc.           |                          |
 *    |--------------------------->|                          |
 *    |                            |                          |
 *    |                            | LL_CLOCK_ACCURACY_REQ    |
 *    |                            |------------------------->|
 *    |                            |                          |
 *    |                            | LL_<INVALID>_RSP         |
 *    |                            |<-------------------------|
 *    |                            |                          |
 *       ~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~
 *    |                            |                          |
 */
ZTEST(sca_periph, test_sca_peripheral_loc_invalid_rsp)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	/* Initiate another SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                          |
 *    | Start                      |                          |
 *    | SCA Update Proc.           |                          |
 *    |--------------------------->|                          |
 *    |                            |                          |
 *    |                            | LL_CLOCK_ACCURACY_REQ    |
 *    |                            |------------------------->|
 *    |                            |                          |
 *    |                            |    LL_CLOCK_ACCURACY_RSP |
 *    |                            |<-------------------------|
 *    |                            |                          |
 *    |                            |                          |
 */
ZTEST(sca_periph, test_ping_periph_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_sca scau = { .status = BT_HCI_ERR_SUCCESS, .sca = 2 };
	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = { };
	struct pdu_data_llctrl_clock_accuracy_rsp remote_sca_rsp = { .sca = 2 };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an SCA Procedure */
	err = ull_cp_req_peer_sca(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_REQ, &conn, &tx, &local_sca_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CLOCK_ACCURACY_RSP, &conn, &remote_sca_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Termination not 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, 0,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should be one notification due to Peer SCA Request */
	ut_rx_node(NODE_PEER_SCA_UPDATE, &ntf, &scau);
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+                   +-----+
 * | UT  |                     | LL_A  |                   | LT  |
 * +-----+                     +-------+                   +-----+
 *    |                            |                          |
 *    |                            | LL_CLOCK_ACCURACY_REQ    |
 *    |                            |------------------------->|
 *    |                            |                          |
 *    |                            |    LL_CLOCK_ACCURACY_RSP |
 *    |                            |<-------------------------|
 *    |                            |                          |
 *    |                            |                          |
 */
ZTEST(sca_central, test_ping_central_rem)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = { };

	struct pdu_data_llctrl_clock_accuracy_rsp remote_sca_rsp = { };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CLOCK_ACCURACY_REQ, &conn, &local_sca_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_RSP, &conn, &tx, &remote_sca_rsp);
	lt_rx_q_is_empty(&conn);

	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+                   +-----+
 * | UT  |                     | LL_A  |                   | LT  |
 * +-----+                     +-------+                   +-----+
 *    |                            |                          |
 *    |                            | LL_CLOCK_ACCURACY_REQ    |
 *    |                            |------------------------->|
 *    |                            |                          |
 *    |                            |    LL_CLOCK_ACCURACY_RSP |
 *    |                            |<-------------------------|
 *    |                            |                          |
 *    |                            |                          |
 */
ZTEST(sca_periph, test_ping_periph_rem)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_clock_accuracy_req local_sca_req = { };

	struct pdu_data_llctrl_clock_accuracy_rsp remote_sca_rsp = { };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CLOCK_ACCURACY_REQ, &conn, &local_sca_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CLOCK_ACCURACY_RSP, &conn, &tx, &remote_sca_rsp);
	lt_rx_q_is_empty(&conn);

	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST_SUITE(sca_central, NULL, NULL, sca_setup, NULL, NULL);
ZTEST_SUITE(sca_periph, NULL, NULL, sca_setup, NULL, NULL);
