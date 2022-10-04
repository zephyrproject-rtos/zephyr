/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

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

#include "lll.h"
#include "lll_df_types.h"
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

#include "helper_pdu.h"
#include "helper_util.h"

struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);
}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | LE Ping Proc.              |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_PING_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_LE_PING_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 *    | Start                      |                   |
 *    | LE Ping Proc.              |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_PING_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_UNKNOWN_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 */
void test_ping_central_loc(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_ping_req local_ping_req = {};

	struct pdu_data_llctrl_ping_rsp remote_ping_rsp = {};

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_PING_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an LE Ping Procedure */
	err = ull_cp_le_ping(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_REQ, &conn, &tx, &local_ping_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_LE_PING_RSP, &conn, &remote_ping_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());

	/* Initiate another LE Ping Procedure */
	err = ull_cp_le_ping(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_REQ, &conn, &tx, &local_ping_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());

}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | LE Ping Proc.              |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_PING_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            | LL_<INVALID>_RSP  |
 *    |                            |<------------------|
 *    |                            |                   |
 *  ~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~
 *    |                            |                   |
 */
void test_ping_central_loc_invalid_rsp(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_PING_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_ping_req local_ping_req = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an LE Ping Procedure */
	err = ull_cp_le_ping(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_REQ, &conn, &tx, &local_ping_req);
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

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());

	/* Initiate another LE Ping Procedure */
	err = ull_cp_le_ping(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_REQ, &conn, &tx, &local_ping_req);
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

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());

}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | LE Ping Proc.              |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_PING_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_LE_PING_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |                            |                   |
 */
void test_ping_periph_loc(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_ping_req local_ping_req = {};

	struct pdu_data_llctrl_ping_rsp remote_ping_rsp = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an LE Ping Procedure */
	err = ull_cp_le_ping(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_REQ, &conn, &tx, &local_ping_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_LE_PING_RSP, &conn, &remote_ping_rsp);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

/* +-----+ +-------+            +-----+
 * | UT  | | LL_A  |            | LT  |
 * +-----+ +-------+            +-----+
 *    |        |                   |
 *    |        |    LL_LE_PING_REQ |
 *    |        |<------------------|
 *    |        |                   |
 *    |        | LL_LE_PING_RSP    |
 *    |        |------------------>|
 *    |        |                   |
 */
void test_ping_central_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_ping_req local_ping_req = {};

	struct pdu_data_llctrl_ping_rsp remote_ping_rsp = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_LE_PING_REQ, &conn, &local_ping_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_RSP, &conn, &tx, &remote_ping_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

/* +-----+ +-------+            +-----+
 * | UT  | | LL_A  |            | LT  |
 * +-----+ +-------+            +-----+
 *    |        |                   |
 *    |        |    LL_LE_PING_REQ |
 *    |        |<------------------|
 *    |        |                   |
 *    |        | LL_LE_PING_RSP    |
 *    |        |------------------>|
 *    |        |                   |
 */
void test_ping_periph_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_ping_req local_ping_req = {};

	struct pdu_data_llctrl_ping_rsp remote_ping_rsp = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_LE_PING_REQ, &conn, &local_ping_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LE_PING_RSP, &conn, &tx, &remote_ping_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

void test_main(void)
{
	ztest_test_suite(ping,
			 ztest_unit_test_setup_teardown(test_ping_central_loc, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_central_loc_invalid_rsp, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_periph_loc, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_central_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_periph_rem, setup,
							unit_test_noop)
		);

	ztest_run_test_suite(ping);
}
