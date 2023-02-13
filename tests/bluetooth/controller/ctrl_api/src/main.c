/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
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

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"
#include "ll_feat.h"

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
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

struct ll_conn conn;

/*
 * Note API and internal test are not yet split out here
 */

void test_api_init(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);

	ull_llcp_init(&conn);

	zassert_true(lr_is_disconnected(&conn));
	zassert_true(rr_is_disconnected(&conn));
}

extern void test_int_mem_proc_ctx(void);
extern void test_int_mem_tx(void);
extern void test_int_create_proc(void);
extern void test_int_llcp_init(void);
extern void test_int_local_pending_requests(void);
extern void test_int_remote_pending_requests(void);

void test_api_connect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(lr_is_idle(&conn));
	zassert_true(rr_is_idle(&conn));
}

void test_api_disconnect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(lr_is_disconnected(&conn));
	zassert_true(rr_is_disconnected(&conn));

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(lr_is_idle(&conn));
	zassert_true(rr_is_idle(&conn));

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(lr_is_disconnected(&conn));
	zassert_true(rr_is_disconnected(&conn));
}

void test_int_disconnect_loc(void)
{
	uint64_t err;
	int nr_free_ctx;
	struct node_tx *tx;
	struct ll_conn conn;

	struct pdu_data_llctrl_version_ind local_version_ind = {
		.version_number = LL_VERSION_NUMBER,
		.company_id = CONFIG_BT_CTLR_COMPANY_ID,
		.sub_version_number = CONFIG_BT_CTLR_SUBVERSION_NUMBER,
	};

	test_setup(&conn);
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt() - 1);

	event_prepare(&conn);
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty(&conn);
	event_done(&conn);

	/*
	 * Now we disconnect before getting a response
	 */
	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	ut_rx_q_is_empty();

	/*
	 * nothing should happen when running a new event
	 */
	event_prepare(&conn);
	event_done(&conn);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/*
	 * all buffers should still be empty
	 */
	lt_rx_q_is_empty(&conn);
	ut_rx_q_is_empty();
}

void test_int_disconnect_rem(void)
{
	int nr_free_ctx;
	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};
	struct ll_conn conn;

	test_setup(&conn);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());
	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/* Disconnect before we reply */

	/* Done */
	event_done(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	nr_free_ctx = ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/* There should not be a host notifications */
	ut_rx_q_is_empty();
}

#define SIZE 2

void test_int_pause_resume_data_path(void)
{
	struct node_tx *node;
	struct node_tx nodes[SIZE] = { 0 };

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);

	/*** #1: Not paused when initialized ***/

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&conn.tx_q, &nodes[i]);
	}

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&conn.tx_q);
		zassert_equal_ptr(node, &nodes[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");


	/*** #2: Single pause/resume ***/

	/* Pause data path */
	llcp_tx_pause_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&conn.tx_q, &nodes[i]);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume data path */
	llcp_tx_resume_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&conn.tx_q);
		zassert_equal_ptr(node, &nodes[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");


	/*** #3: Multiple pause/resume ***/

	/* Pause data path */
	llcp_tx_pause_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
	llcp_tx_pause_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&conn.tx_q, &nodes[i]);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume data path */
	llcp_tx_resume_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume data path */
	llcp_tx_resume_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&conn.tx_q);
		zassert_equal_ptr(node, &nodes[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");


	/*** #4: Asymetric pause/resume ***/

	/* Pause data path */
	llcp_tx_pause_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Enqueue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		ull_tx_q_enqueue_data(&conn.tx_q, &nodes[i]);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume data path (wrong mask) */
	llcp_tx_resume_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");

	/* Resume data path */
	llcp_tx_resume_data(&conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	/* Dequeue data nodes */
	for (int i = 0U; i < SIZE; i++) {
		node = ull_tx_q_dequeue(&conn.tx_q);
		zassert_equal_ptr(node, &nodes[i], NULL);
	}

	/* Tx Queue shall be empty */
	node = ull_tx_q_dequeue(&conn.tx_q);
	zassert_equal_ptr(node, NULL, "");
}

void test_main(void)
{
	ztest_test_suite(internal,
			 ztest_unit_test(test_int_mem_proc_ctx),
			 ztest_unit_test(test_int_mem_tx),
			 ztest_unit_test(test_int_create_proc),
			 ztest_unit_test(test_int_llcp_init),
			 ztest_unit_test(test_int_local_pending_requests),
			 ztest_unit_test(test_int_remote_pending_requests),
			 ztest_unit_test(test_int_disconnect_loc),
			 ztest_unit_test(test_int_disconnect_rem),
			 ztest_unit_test(test_int_pause_resume_data_path));

	ztest_test_suite(public, ztest_unit_test(test_api_init), ztest_unit_test(test_api_connect),
			 ztest_unit_test(test_api_disconnect));

	ztest_run_test_suite(internal);
	ztest_run_test_suite(public);
}
