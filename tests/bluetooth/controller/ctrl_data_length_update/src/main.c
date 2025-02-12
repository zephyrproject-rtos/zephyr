/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define ULL_LLCP_UNITTEST

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
#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"
#include "ull_llcp_features.h"

#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

static struct ll_conn conn;

static void dle_setup(void *data)
{
	test_setup(&conn);
}

/*
 * Locally triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (201,1720,251,2120)      |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    | (251,2120,201,1720)        |                              |
 *    | Data Length Update Proc.   |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

ZTEST(dle_central, test_data_length_update_central_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 201, 1720, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 201, 1720 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Locally triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    |                            |         LL_UNKNOWN_RSP       |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *  ~~~~~~~~~~~~~~~~~~~~~~~  Unmask DLE support ~~~~~~~~~~~~~~~~~~~~
 *    |                            |                              |
 *    |                            |                              |
 */
ZTEST(dle_central, test_data_length_update_central_loc_unknown_rsp)
{
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ
	};
	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Confirm DLE is indicated as supported */
	zassert_equal(feature_dle(&conn), true, "DLE Feature masked out");

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* Confirm DLE is no longer indicated as supported */
	zassert_equal(feature_dle(&conn), false, "DLE Feature not masked out");

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Locally triggered Data Length Update procedure
 *
 *
 * Start a Feature Exchange procedure and Data Length Update procedure.
 *
 * The Feature Exchange procedure completes, removing Data Length Update
 * procedure support.
 *
 * Expect that the already enqueued Data Length Update procedure completes
 * without doing anything.
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Feature Exchange Proc.     |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    |                            | LL_FEATURE_REQ               |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    |                            |               LL_FEATURE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *  ~~~~~~~~~~~~~~~~~~~~~~~  Unmask DLE support ~~~~~~~~~~~~~~~~~~~~
 *    |                            |                              |
 *    |     Feature Exchange Proc. |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */
ZTEST(dle_central, test_data_length_update_central_loc_unsupported)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;
	struct pdu_data_llctrl_feature_rsp remote_feature_rsp;
	struct pdu_data_llctrl_feature_rsp exp_remote_feature_rsp;

	sys_put_le64(DEFAULT_FEATURE, local_feature_req.features);
	sys_put_le64(0ULL, remote_feature_rsp.features);
	sys_put_le64(0ULL, exp_remote_feature_rsp.features);


	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Confirm DLE is indicated as supported */
	zassert_equal(feature_dle(&conn), true, "DLE Feature masked out");

	/* Initiate a Feature Exchange Procedure */
	err = ull_cp_feature_exchange(&conn, 1U);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_FEATURE_REQ, &conn, &tx, &local_feature_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_FEATURE_RSP, &conn, &remote_feature_rsp);

	event_done(&conn);
	/* There should be one host notification */

	ut_rx_pdu(LL_FEATURE_RSP, &ntf, &exp_remote_feature_rsp);

	ut_rx_q_is_empty();

	ull_cp_release_tx(&conn, tx);
	release_ntf(ntf);

	/* Confirm DLE is no longer indicated as supported */
	zassert_equal(feature_dle(&conn), false, "DLE Feature not masked out");

	/* Prepare another event for enqueued Data Length Update procedure */
	event_prepare(&conn);
	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);
	event_done(&conn);

	/* Confirm DLE is no longer indicated as supported */
	zassert_equal(feature_dle(&conn), false, "DLE Feature not masked out");

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Locally triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    |                            |         LL_<INVALID>_RSP     |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *   ~~~~~~~~~~~~~~~~~~~~  TERMINATE CONNECTION  ~~~~~~~~~~~~~~~~~~~
 *    |                            |                              |
 *    |                            |                              |
 */
ZTEST(dle_central, test_data_length_update_central_loc_invalid_rsp)
{
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

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

	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate another Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

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

/*
 * Locally triggered Data Length Update procedure - with no update to eff and thus no ntf
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (27,328,27,328)          |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 */
ZTEST(dle_central, test_data_length_update_central_loc_no_eff_change)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 27, 328, 27, 328 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}
/*
 * Locally triggered Data Length Update procedure -
 * - first updating effective DLE and then without update to eff and thus no ntf
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,221,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (101,920,251,2120)       |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    | (251,2120,101,920)         |                              |
 *    | Data Length Update Proc.   |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (101, 920,251,2120)      |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 */

ZTEST(dle_central, test_data_length_update_central_loc_no_eff_change2)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 101, 920, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 101, 920 };
	struct pdu_data_llctrl_length_req local_length_req2 = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp2 = { 101, 920, 251, 2120 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);

	/* Now lets generate another DLU, but one that should not result in
	 * change to effective numbers, thus not generate NTF
	 */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req2);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp2);

	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 2, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}

ZTEST(dle_periph, test_data_length_update_periph_loc)
{
	uint64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 211, 1800, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 211, 1800 };

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            |  (27, 328, 251, 2120)        |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |   (251, 2120, 211, 1800)     |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |  (251,2120,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

ZTEST(dle_central, test_data_length_update_central_rem)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 251, 2120 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 27, 328 };

	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            | (27, 328, 201, 1720)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *    |                            |     (251, 2120, 211, 1800)   |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |  (201,1720,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

ZTEST(dle_periph, test_data_length_update_periph_rem)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 201, 1720 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 201, 1720, 27, 328 };
	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

/*
 * Remotely triggered Data Length Update procedure with local request piggy back
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            | (27, 328, 211, 1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    |                            |   (251, 2120, 211, 1800)     |
 *    |                            |  LL_DATA_LENGTH_UPDATE_RSP   |
 *    |                            |----------------------------->|
 *    |  (211,1800,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

ZTEST(dle_periph, test_data_length_update_periph_rem_and_loc)
{
	uint64_t err;
	struct node_tx *tx;
	struct proc_ctx *ctx = NULL;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 211, 1800 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 211, 1800, 27, 328 };
	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	/* Allocate dummy procedure used to steal all buffers */
	ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);

	/* Steal all tx buffers */
	while (llcp_tx_alloc_peek(&conn, ctx)) {
		tx = llcp_tx_alloc(&conn, ctx);
		zassert_not_null(tx, NULL);
	}

	/* Dummy remove, as above loop might queue up ctx */
	llcp_tx_alloc_unpeek(ctx);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_done(&conn);

	ull_cp_release_tx(&conn, tx);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

ZTEST(dle_util, test_data_length_update_dle_max_time_get)
{
	uint16_t max_time = 0xffff;
	uint16_t max_octets = 211;

#ifdef CONFIG_BT_CTLR_PHY
	max_time = 2120;
#endif
	conn.llcp.fex.valid = 0;

	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Emulate complete feat exch without CODED */
	conn.llcp.fex.valid = 1;
	conn.llcp.fex.features_used = 0;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Check the case of CODED PHY support */
	conn.llcp.fex.features_used = LL_FEAT_BIT_PHY_CODED;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Finally check that MAX on max_tx_time works */
	max_time = 20000;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 17040, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Check that MIN works */
	max_time = 20;
	max_octets = 2;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#endif
}

ZTEST_SUITE(dle_central, NULL, NULL, dle_setup, NULL, NULL);
ZTEST_SUITE(dle_periph, NULL, NULL, dle_setup, NULL, NULL);
ZTEST_SUITE(dle_util, NULL, NULL, dle_setup, NULL, NULL);
