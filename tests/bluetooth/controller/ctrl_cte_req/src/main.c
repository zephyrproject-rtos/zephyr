/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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

/* Tests of successful execution of CTE Request Procedure */

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start initiation           |                   |
 *    | CTE Reqest Proc.           |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_CTE_REQ     |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_LE_CTE_RSP  |
 *    |                            |<------------------|
 *    |                            |                   |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                            |                   |
 *    | LE Connection IQ Report    |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 *    |                            |                   |
 */
void test_cte_req_central_local(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOA_CTE,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};
	struct pdu_data_llctrl_cte_rsp remote_cte_rsp = {};
	struct node_rx_pdu *ntf;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an CTE Request Procedure */
	err = ull_cp_cte_req(&conn, local_cte_req.min_cte_len_req, local_cte_req.cte_type_req);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_REQ, &conn, &tx, &local_cte_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CTE_RSP, &conn, &remote_cte_rsp);

	/* Done */
	event_done(&conn);

	/* Receive notification of sampled CTE response */
	ut_rx_pdu(LL_CTE_RSP, &ntf, &remote_cte_rsp);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
				  "Free CTX buffers %d", ctx_buffers_free());
}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start initiator            |                   |
 *    | CTE Reqest Proc.           |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_CTE_REQ     |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_LE_CTE_RSP  |
 *    |                            |<------------------|
 *    |                            |                   |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                            |                   |
 *    | LE Connection IQ Report    |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 *    |                            |                   |
 */
void test_cte_req_peripheral_local(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOA_CTE,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};

	struct pdu_data_llctrl_cte_rsp remote_cte_rsp = {};
	struct node_rx_pdu *ntf;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an CTE Request Procedure */
	err = ull_cp_cte_req(&conn, local_cte_req.min_cte_len_req, local_cte_req.cte_type_req);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_REQ, &conn, &tx, &local_cte_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CTE_RSP, &conn, &remote_cte_rsp);

	/* Done */
	event_done(&conn);

	/* Receive notification of sampled CTE response */
	ut_rx_pdu(LL_CTE_RSP, &ntf, &remote_cte_rsp);

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
 *    | Start responder            |                   |
 *    | CTE Reqest Proc.           |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_CTE_REQ     |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |                            |    LL_LE_CTE_RSP  |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |                   |
 */
void test_cte_req_central_remote(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOA_CTE,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};

	struct pdu_data_llctrl_cte_rsp remote_cte_rsp = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Enable response for CTE request */
	ull_cp_cte_rsp_enable(&conn, true, BT_HCI_LE_CTE_LEN_MAX,
			      (BT_HCI_LE_AOA_CTE | BT_HCI_LE_AOD_CTE_1US | BT_HCI_LE_AOD_CTE_2US));

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CTE_REQ, &conn, &local_cte_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_RSP, &conn, &tx, &remote_cte_rsp);
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

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start responder            |                   |
 *    | CTE Reqest Proc   .        |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_LE_CTE_REQ     |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |                            |    LL_LE_CTE_RSP  |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |                   |
 */
void test_cte_req_peripheral_remote(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOA_CTE,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};

	struct pdu_data_llctrl_cte_rsp remote_cte_rsp = {};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Enable response for CTE request */
	ull_cp_cte_rsp_enable(&conn, true, BT_HCI_LE_CTE_LEN_MAX,
			      (BT_HCI_LE_AOA_CTE | BT_HCI_LE_AOD_CTE_1US | BT_HCI_LE_AOD_CTE_2US));

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CTE_REQ, &conn, &local_cte_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_RSP, &conn, &tx, &remote_cte_rsp);
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

/* Tests of expected failures during execution of CTE Request Procedure */

/* +-----+                     +-------+                         +-----+
 * | UT  |                     | LL_A  |                         | LT  |
 * +-----+                     +-------+                         +-----+
 *    |                            |                                |
 *    | Start initiation           |                                |
 *    | CTE Reqest Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_LE_CTE_REQ                  |
 *    |                            |------------------------------->|
 *    |                            |                                |
 *    |                            | LL_REJECT_EXT_IND              |
 *    |                            | BT_HCI_ERR_UNSUPP_LL_PARAM_VAL |
 *    |                            | or BT_HCI_ERR_INVALID_LL_PARAM |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                            |                                |
 *    | LE CTE Request Failed      |                                |
 *    |<---------------------------|                                |
 *    |                            |                                |
 *    |                            |                                |
 */
void test_cte_req_rejected_inv_ll_param_central_local(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOD_CTE_1US,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};
	struct pdu_data_llctrl_reject_ext_ind remote_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL,
	};
	struct node_rx_pdu *ntf;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an CTE Request Procedure */
	err = ull_cp_cte_req(&conn, local_cte_req.min_cte_len_req, local_cte_req.cte_type_req);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_REQ, &conn, &tx, &local_cte_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &remote_reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Receive notification of sampled CTE response */
	ut_rx_pdu(LL_REJECT_EXT_IND, &ntf, &remote_reject_ext_ind);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
				  "Free CTX buffers %d", ctx_buffers_free());
}

/* +-----+                     +-------+                         +-----+
 * | UT  |                     | LL_A  |                         | LT  |
 * +-----+                     +-------+                         +-----+
 *    |                            |                                |
 *    | Start initiation           |                                |
 *    | CTE Reqest Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_LE_CTE_REQ                  |
 *    |                            |------------------------------->|
 *    |                            |                                |
 *    |                            | LL_REJECT_EXT_IND              |
 *    |                            | BT_HCI_ERR_UNSUPP_LL_PARAM_VAL |
 *    |                            | or BT_HCI_ERR_INVALID_LL_PARAM |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                            |                                |
 *    | LE CTE Request Failed      |                                |
 *    |<---------------------------|                                |
 *    |                            |                                |
 *    |                            |                                |
 */
void test_cte_req_rejected_inv_ll_param_peripheral_local(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOD_CTE_1US,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};
	struct pdu_data_llctrl_reject_ext_ind remote_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL,
	};
	struct node_rx_pdu *ntf;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an CTE Request Procedure */
	err = ull_cp_cte_req(&conn, local_cte_req.min_cte_len_req, local_cte_req.cte_type_req);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CTE_REQ, &conn, &tx, &local_cte_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &remote_reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Receive notification of sampled CTE response */
	ut_rx_pdu(LL_REJECT_EXT_IND, &ntf, &remote_reject_ext_ind);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
				  "Free CTX buffers %d", ctx_buffers_free());
}

/* +-----+                     +-------+                         +-----+
 * | UT  |                     | LL_A  |                         | LT  |
 * +-----+                     +-------+                         +-----+
 *    |                            |                                |
 *    | Start initiation           |                                |
 *    | CTE Reqest Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_LE_CTE_REQ                  |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 *    |                            | LL_REJECT_EXT_IND              |
 *    |                            | BT_HCI_ERR_UNSUPP_LL_PARAM_VAL |
 *    |                            |------------------------------->|
 *    |                            |                                |
 */
void test_cte_req_reject_inv_ll_param_central_remote(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOD_CTE_2US,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};

	struct pdu_data_llctrl_reject_ext_ind remote_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL,
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Enable response for CTE request */
	ull_cp_cte_rsp_enable(&conn, true, BT_HCI_LE_CTE_LEN_MAX,
			      (BT_HCI_LE_AOA_CTE | BT_HCI_LE_AOD_CTE_1US));

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CTE_REQ, &conn, &local_cte_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &remote_reject_ext_ind);
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

/* +-----+                     +-------+                         +-----+
 * | UT  |                     | LL_A  |                         | LT  |
 * +-----+                     +-------+                         +-----+
 *    |                            |                                |
 *    | Start initiation           |                                |
 *    | CTE Reqest Proc.           |                                |
 *    |--------------------------->|                                |
 *    |                            |                                |
 *    |                            | LL_LE_CTE_REQ                  |
 *    |                            |<-------------------------------|
 *    |                            |                                |
 *    |                            | LL_REJECT_EXT_IND              |
 *    |                            | BT_HCI_ERR_UNSUPP_LL_PARAM_VAL |
 *    |                            |------------------------------->|
 *    |                            |                                |
 */
void test_cte_req_reject_inv_ll_param_peripheral_remote(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_cte_req local_cte_req = {
		.cte_type_req = BT_HCI_LE_AOD_CTE_2US,
		.min_cte_len_req = BT_HCI_LE_CTE_LEN_MIN,
	};

	struct pdu_data_llctrl_reject_ext_ind remote_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CTE_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL,
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Enable response for CTE request */
	ull_cp_cte_rsp_enable(&conn, true, BT_HCI_LE_CTE_LEN_MAX,
			      (BT_HCI_LE_AOA_CTE | BT_HCI_LE_AOD_CTE_1US));

	/* Prepare */
	event_prepare(&conn);

	/* Tx */
	lt_tx(LL_CTE_REQ, &conn, &local_cte_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &remote_reject_ext_ind);
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
	ztest_test_suite(
		cte_req,
		ztest_unit_test_setup_teardown(test_cte_req_central_local, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_peripheral_local, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_central_remote, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_peripheral_remote, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_rejected_inv_ll_param_central_local,
					       setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_rejected_inv_ll_param_peripheral_local,
					       setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_reject_inv_ll_param_central_remote,
					       setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_cte_req_reject_inv_ll_param_peripheral_remote,
					       setup, unit_test_noop));

	ztest_run_test_suite(cte_req);
}
