/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"


#define ULL_LLCP_UNITTEST

#define PROC_CTX_BUF_NUM 2
#define TX_CTRL_BUF_NUM 2
#define NTF_BUF_NUM 2


#define FEATURE_0 0xAA
/* Implementation Under Test Begin */
#include "ll_sw/ull_llcp.c"
/* Implementation Under Test End */

#include "helper_pdu.h"
#include "helper_util.h"

//struct ull_tx_q tx_q;
struct ull_cp_conn conn;

//sys_slist_t ll_rx_q;
//sys_slist_t ut_rx_q;

static void setup(void)
{
	test_setup(&conn);
}


void test_int_mem_proc_ctx(void)
{
	struct proc_ctx *ctx1;
	struct proc_ctx *ctx2;

	ull_cp_init();

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		ctx1 = proc_ctx_acquire();

		/* The previous acquire should be valid */
		zassert_not_null(ctx1, NULL);
	}

	ctx2 = proc_ctx_acquire();

	/* The last acquire should fail */
	zassert_is_null(ctx2, NULL);

	proc_ctx_release(ctx1);
	ctx1 = proc_ctx_acquire();

	/* Releasing returns the context to the avilable pool */
	zassert_not_null(ctx1, NULL);
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
		u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req = {
		.features[0] = FEATURE_0
	};

	struct pdu_data_llctrl_feature_rsp remote_feature_rsp = {
		.features[0] = FEATURE_0
	};


	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Version Exchange Procedure */
	err = ull_cp_feature_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Run */
//	ull_cp_run(&conn);
	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_FEATURE_REQ, &conn, &tx, &local_feature_req);
	lt_rx_q_is_empty(&conn);
//	ull_cp_release_tx(tx);
	printf("Sending feature response\n");
	/* Rx */
	lt_tx(LL_FEATURE_RSP, &conn, &remote_feature_rsp);

	event_done(&conn);
	/* There should be one host notification */
	ut_rx_pdu(LL_FEATURE_RSP, &ntf,  &remote_feature_rsp);
	ut_rx_q_is_empty(&ll_rx_q);

}

void test_feature_exchange_mas_loc_2(void)
{
//	EGON;
}



void test_main(void)
{

		ztest_test_suite(feature_exchange,
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_feature_exchange_mas_loc_2, setup, unit_test_noop)

			);



		ztest_run_test_suite(feature_exchange);
}
