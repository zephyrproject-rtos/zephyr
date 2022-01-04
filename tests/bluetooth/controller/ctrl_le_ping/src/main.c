/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"
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
 *    |                            |                   |
 */
void test_ping_mas_loc(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_ping_req local_ping_req = {};

	struct pdu_data_llctrl_ping_rsp remote_ping_rsp = {};

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

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
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
void test_ping_sla_loc(void)
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

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
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
void test_ping_mas_rem(void)
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

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
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
void test_ping_sla_rem(void)
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

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
		      "Free CTX buffers %d", ctx_buffers_free());
}

void test_main(void)
{
	ztest_test_suite(ping,
			 ztest_unit_test_setup_teardown(test_ping_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_sla_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_ping_sla_rem, setup, unit_test_noop));

	ztest_run_test_suite(ping);
}
