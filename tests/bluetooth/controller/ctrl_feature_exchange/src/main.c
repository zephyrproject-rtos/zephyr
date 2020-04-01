/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"


#define ULL_LLCP_UNITTEST


#define TX_CTRL_BUF_NUM 2
#define NTF_BUF_NUM 2

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

/* Implementation Under Test Begin */
// #include "ll_sw/ull_llcp.c"
/* Implementation Under Test End */

#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

struct ull_cp_conn conn;

static void setup(void)
{
	test_setup(&conn);
}


/*
 * +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Feature Exchange Proc.     |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_FEATURE_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_FEATURE_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |     Feature Exchange Proc. |                   |
 *    |                   Complete |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
void test_feature_exchange_mas_loc(void)
{
	u64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;
	struct pdu_data_llctrl_feature_rsp remote_feature_rsp;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Feature Exchange Procedure */
	err = ull_cp_feature_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_FEATURE_REQ, &conn, &tx, &local_feature_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_FEATURE_RSP, &conn, &remote_feature_rsp);

	event_done(&conn);
	/* There should be one host notification */

	ut_rx_pdu(LL_FEATURE_RSP, &ntf,  &remote_feature_rsp);

	ut_rx_q_is_empty();

	zassert_equal(conn.lll.event_counter, 1,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}

void test_feature_exchange_mas_loc_2(void)
{
	u8_t err;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_feature_exchange(&conn);
	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
		err = ull_cp_feature_exchange(&conn);
	}

	zassert_not_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	zassert_equal(conn.lll.event_counter, 0,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}


void test_feature_exchange_mas_rem(void)
{
//	u64_t err;
	struct node_tx *tx;
//	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req remote_feature_req;
	struct pdu_data_llctrl_feature_rsp local_feature_rsp;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);

	lt_tx(LL_SLAVE_FEATURE_REQ, &conn, &remote_feature_req);

	event_done(&conn);

	event_prepare(&conn);

	lt_rx(LL_FEATURE_RSP, &conn, &tx, &local_feature_rsp);
	lt_rx_q_is_empty(&conn);

	event_done(&conn);

	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 2,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}

void test_feature_exchange_mas_rem_2(void)
{
	u64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req remote_feature_req;
	struct pdu_data_llctrl_feature_rsp local_feature_rsp;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);
	lt_tx(LL_SLAVE_FEATURE_REQ, &conn, &remote_feature_req);
	event_done(&conn);
	err = ull_cp_feature_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	lt_rx(LL_FEATURE_RSP, &conn, &tx, &local_feature_rsp);
	lt_rx_q_is_empty(&conn);
	event_done(&conn);

	ut_rx_pdu(LL_SLAVE_FEATURE_REQ, &ntf, &remote_feature_req);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 2,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}


void test_slave_feature_exchange_sla_loc(void)
{
	u64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;
	struct pdu_data_llctrl_feature_rsp remote_feature_rsp;

	test_set_role(&conn, BT_HCI_ROLE_SLAVE);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Feature Exchange Procedure */
	err = ull_cp_feature_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_SLAVE_FEATURE_REQ, &conn, &tx, &local_feature_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_FEATURE_RSP, &conn, &remote_feature_rsp);

	event_done(&conn);
	/* There should be one host notification */

	ut_rx_pdu(LL_FEATURE_RSP, &ntf,  &remote_feature_rsp);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}


void test_feature_exchange_mas_loc_unknown_rsp(void)
{
	u64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_FEATURE_REQ
	};

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Feature Exchange Procedure */
	err = ull_cp_feature_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_FEATURE_REQ, &conn, &tx, &local_feature_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	event_done(&conn);

	// ull_cp_release_tx(tx);

	/* There should be one host notification */

	ut_rx_pdu(LL_UNKNOWN_RSP, &ntf,  &unknown_rsp);
	ut_rx_q_is_empty();

	test_print_conn(&conn);
	zassert_equal(conn.lll.event_counter, 1,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}


void test_main(void)
{
	ztest_test_suite(feature_exchange_master,
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_loc_2, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_rem_2, setup, unit_test_noop)
		);

	ztest_test_suite(feature_exchange_slave,
			 ztest_unit_test_setup_teardown(test_slave_feature_exchange_sla_loc, setup, unit_test_noop)
		);

	ztest_test_suite(feature_exchange_unknown,
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_loc_unknown_rsp, setup, unit_test_noop)
		);

	ztest_run_test_suite(feature_exchange_master);
	ztest_run_test_suite(feature_exchange_slave);
	ztest_run_test_suite(feature_exchange_unknown);
}
