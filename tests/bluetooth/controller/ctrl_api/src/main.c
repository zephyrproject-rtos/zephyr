/*
 * Copyright (c) 2020 Demant
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
#include "ll_settings.h"
#include "ll_feat.h"

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
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;

/*
 * Note API and internal test are not yet split out here
 */

ZTEST(public, test_api_init)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);

	ull_llcp_init(&conn);

	zassert_true(llcp_lr_is_disconnected(&conn));
	zassert_true(llcp_rr_is_disconnected(&conn));
}

ZTEST(public, test_api_connect)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(llcp_lr_is_idle(&conn));
	zassert_true(llcp_rr_is_idle(&conn));
}

ZTEST(public, test_api_disconnect)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(llcp_lr_is_disconnected(&conn));
	zassert_true(llcp_rr_is_disconnected(&conn));

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(llcp_lr_is_idle(&conn));
	zassert_true(llcp_rr_is_idle(&conn));

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(llcp_lr_is_disconnected(&conn));
	zassert_true(llcp_rr_is_disconnected(&conn));
}

ZTEST(public, test_int_disconnect_loc)
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

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt() - 1);

	event_prepare(&conn);
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty(&conn);
	event_done(&conn);

	/*
	 * Now we disconnect before getting a response
	 */
	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	ut_rx_q_is_empty();

	/*
	 * nothing should happen when running a new event
	 */
	event_prepare(&conn);
	event_done(&conn);

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/*
	 * all buffers should still be empty
	 */
	lt_rx_q_is_empty(&conn);
	ut_rx_q_is_empty();
}

ZTEST(public, test_int_disconnect_rem)
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

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());
	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/* Disconnect before we reply */

	/* Done */
	event_done(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	nr_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_free_ctx, test_ctx_buffers_cnt());

	/* There should not be a host notifications */
	ut_rx_q_is_empty();
}

#define SIZE 2

ZTEST(public, test_int_pause_resume_data_path)
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

ZTEST(public, test_check_peek_proc)
{
	struct proc_ctx *ctx1, *ctx2;

	ctx1 = llcp_create_local_procedure(PROC_CHAN_MAP_UPDATE);
	llcp_lr_enqueue(&conn, ctx1);

	zassert_is_null(llcp_lr_peek_proc(&conn, PROC_CIS_CREATE), "CTX is not null");
	zassert_equal(llcp_lr_peek_proc(&conn, PROC_CHAN_MAP_UPDATE), ctx1, "CTX is not correct");

	ctx2 = llcp_create_local_procedure(PROC_CIS_CREATE);
	llcp_lr_enqueue(&conn, ctx2);

	zassert_equal(llcp_lr_peek_proc(&conn, PROC_CHAN_MAP_UPDATE), ctx1, "CTX is not correct");
	zassert_equal(llcp_lr_peek_proc(&conn, PROC_CIS_CREATE), ctx2, "CTX is not correct");
	zassert_is_null(llcp_lr_peek_proc(&conn, PROC_CIS_TERMINATE), "CTX is not null");
}

ZTEST(internal, test_int_mem_proc_ctx)
{
	struct proc_ctx *ctx1;
	struct proc_ctx *ctx2;
	int nr_of_free_ctx;

	ull_cp_init();

	nr_of_free_ctx = llcp_ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM +
		      CONFIG_BT_CTLR_LLCP_REMOTE_PROC_CTX_BUF_NUM, NULL);

	for (int i = 0U; i < CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM; i++) {
		ctx1 = llcp_proc_ctx_acquire();

		/* The previous acquire should be valid */
		zassert_not_null(ctx1, NULL);
	}

	nr_of_free_ctx = llcp_local_ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 0);

	ctx2 = llcp_proc_ctx_acquire();

	/* The last acquire should fail */
	zassert_is_null(ctx2, NULL);

	llcp_proc_ctx_release(ctx1);
	nr_of_free_ctx = llcp_local_ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 1);

	ctx1 = llcp_proc_ctx_acquire();

	/* Releasing returns the context to the avilable pool */
	zassert_not_null(ctx1, NULL);
}

ZTEST(internal, test_int_mem_tx)
{
	bool peek;
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	#define TX_BUFFER_POOL_SIZE (CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM  + \
					CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM)
#else /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	#define TX_BUFFER_POOL_SIZE (CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM  + \
					CONFIG_BT_CTLR_LLCP_CONN * \
					CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM)
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	struct ll_conn conn;
	struct node_tx *txl[TX_BUFFER_POOL_SIZE];
	struct proc_ctx *ctx;

	ull_cp_init();
	ull_llcp_init(&conn);

	ctx = llcp_create_local_procedure(PROC_CONN_UPDATE);

	for (int i = 0U; i < TX_BUFFER_POOL_SIZE; i++) {
		peek = llcp_tx_alloc_peek(&conn, ctx);

		/* The previous tx alloc peek should be valid */
		zassert_true(peek, NULL);

		txl[i] = llcp_tx_alloc(&conn, ctx);

		/* The previous alloc should be valid */
		zassert_not_null(txl[i], NULL);
	}

	peek = llcp_tx_alloc_peek(&conn, ctx);

	/* The last tx alloc peek should fail */
	zassert_false(peek, NULL);

	/* Release all */
	for (int i = 0U; i < TX_BUFFER_POOL_SIZE; i++) {
		ull_cp_release_tx(&conn, txl[i]);
	}

	for (int i = 0U; i < TX_BUFFER_POOL_SIZE; i++) {
		peek = llcp_tx_alloc_peek(&conn, ctx);

		/* The previous tx alloc peek should be valid */
		zassert_true(peek, NULL);

		txl[i] = llcp_tx_alloc(&conn, ctx);

		/* The previous alloc should be valid */
		zassert_not_null(txl[i], NULL);
	}

	peek = llcp_tx_alloc_peek(&conn, ctx);

	/* The last tx alloc peek should fail */
	zassert_false(peek, NULL);

	/* Release all */
	for (int i = 0U; i < TX_BUFFER_POOL_SIZE; i++) {
		ull_cp_release_tx(&conn, txl[i]);
	}
}

ZTEST(internal, test_int_create_proc)
{
	struct proc_ctx *ctx;

	ull_cp_init();

	ctx = llcp_create_procedure(PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);

	zassert_equal(ctx->proc, PROC_VERSION_EXCHANGE);
	zassert_equal(ctx->collision, 0);

	for (int i = 0U; i < CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM; i++) {
		zassert_not_null(ctx, NULL);
		ctx = llcp_create_procedure(PROC_VERSION_EXCHANGE);
	}

	zassert_is_null(ctx, NULL);
}

ZTEST(internal, test_int_llcp_init)
{
	struct ll_conn conn;

	ull_cp_init();

	ull_llcp_init(&conn);

	memset(&conn.llcp, 0xAA, sizeof(conn.llcp));

	ull_llcp_init(&conn);

	zassert_equal(conn.llcp.local.pause, 0);
	zassert_equal(conn.llcp.remote.pause, 0);
}

ZTEST(internal, test_int_local_pending_requests)
{
	struct ll_conn conn;
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	peek_ctx = llcp_lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = llcp_lr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	llcp_lr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *)sys_slist_peek_head(&conn.llcp.local.pend_proc_list);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	peek_ctx = llcp_lr_peek(&conn);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	dequeue_ctx = llcp_lr_dequeue(&conn);
	zassert_equal_ptr(dequeue_ctx, &ctx, NULL);

	peek_ctx = llcp_lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = llcp_lr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);
}

ZTEST(internal, test_int_remote_pending_requests)
{
	struct ll_conn conn;
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);

	peek_ctx = llcp_rr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = llcp_rr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	llcp_rr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *)sys_slist_peek_head(&conn.llcp.remote.pend_proc_list);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	peek_ctx = llcp_rr_peek(&conn);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	dequeue_ctx = llcp_rr_dequeue(&conn);
	zassert_equal_ptr(dequeue_ctx, &ctx, NULL);

	peek_ctx = llcp_rr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = llcp_rr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);
}

ZTEST_SUITE(public, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(internal, NULL, NULL, NULL, NULL, NULL);
