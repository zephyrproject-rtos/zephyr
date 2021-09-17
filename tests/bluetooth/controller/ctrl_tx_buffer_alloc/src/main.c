/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#define ULL_LLCP_UNITTEST

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/ufifo.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_llcp_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

#if (CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE > 0)

static struct ll_conn *conn[CONFIG_BT_MAX_CONN];

static void setup(void)
{
	ull_conn_init();
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		test_setup(&conn[i]);
	}
}

static uint8_t inc_mod(uint8_t idx, uint8_t add)
{
	idx += add;
	if (idx >= CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE) {
		idx %= CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE;
	}
	return idx;
}

EXTERN_UFIFO(tx_bufferq, sizeof(ctx_handle_t), CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE);


void test_tx_buffer_queue(void)
{
	struct proc_ctx *ctx;
	struct proc_ctx *ctx2;
	struct proc_ctx *ctxs[CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE];
	uint8_t handle_ctxs[CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE];
	struct node_tx *tx[CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM +
		CONFIG_BT_MAX_CONN * CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM] = { NULL };
	uint16_t tx_alloc_idx = 0;
	uint8_t handle_ctx, handle_ctx2;
	int i;

	ctx = llcp_create_local_procedure(ll_conn_handle_get(conn[0]), PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);
	ctx2 = llcp_create_local_procedure(ll_conn_handle_get(conn[1]), PROC_LE_PING);
	zassert_not_null(ctx2, NULL);

	handle_ctx = llcp_proc_ctx_handle_get(ctx);
	handle_ctx2 = llcp_proc_ctx_handle_get(ctx2);


	zassert_equal(ufifo_tx_bufferq.in, ufifo_tx_bufferq.out, NULL);

	/* Run through add/remove/unqueue testing - incl. crossing the size boundary */
	for (int ctx_idx = 0; ctx_idx < CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE*2; ctx_idx++) {
		/* queue is empty so request should be accepted */
		zassert_true(UFIFO_ENQUEUE(tx_bufferq, &handle_ctx), NULL);
		zassert_not_equal(ufifo_tx_bufferq.in, ufifo_tx_bufferq.out, NULL);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 1), NULL);
		/* queue is not empty, but ctx is up, so request should be accepted */
		zassert_true(UFIFO_ENQUEUE(tx_bufferq, &handle_ctx), NULL);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 1), NULL);

		/* queue is not empty, and ctx2 is NOT up, so request should be rejected */
		zassert_false(UFIFO_ENQUEUE(tx_bufferq, &handle_ctx2), NULL);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 2), NULL);

		UFIFO_UNQUEUE(tx_bufferq, &handle_ctx);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 1), NULL);
		UFIFO_UNQUEUE(tx_bufferq, &handle_ctx);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 1), NULL);
		/* queue is not empty, but ctx2 should be up, so request is accepted */
		zassert_true(UFIFO_ENQUEUE(tx_bufferq, &handle_ctx2), NULL);
		UFIFO_DEQUEUE_HEAD(tx_bufferq, NULL);
		zassert_equal(ufifo_tx_bufferq.in, inc_mod(ctx_idx, 1), NULL);
		zassert_equal(ufifo_tx_bufferq.in, ufifo_tx_bufferq.out, NULL);
	}

	llcp_proc_ctx_release(ctx);
	llcp_proc_ctx_release(ctx2);

	/* Check that empty flag works */
	zassert_equal(ufifo_tx_bufferq.empty, 1, NULL);
	for (int ctx_idx = 0; ctx_idx < CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE; ctx_idx++) {
		ctxs[ctx_idx] = llcp_create_local_procedure(ll_conn_handle_get(conn[0]),
							    PROC_VERSION_EXCHANGE);
		handle_ctxs[ctx_idx] = llcp_proc_ctx_handle_get(ctxs[ctx_idx]);

		/* This 'hacks' the ctx->conn_handle to mimick
		 * that ctxs relate to different connections
		 */
		ctxs[ctx_idx]->conn_handle = ctx_idx;
		UFIFO_ENQUEUE(tx_bufferq, &handle_ctxs[ctx_idx]);
	}

	for (int ctx_idx = 0; ctx_idx < CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE; ctx_idx++) {
		zassert_equal(ufifo_tx_bufferq.empty, 0, NULL);
		UFIFO_DEQUEUE_HEAD(tx_bufferq, NULL);
	}
	zassert_equal(ufifo_tx_bufferq.empty, 1, NULL);

	/* Now let's check alloc flow */
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(ctxs[0]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[0]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(ctxs[0]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[0]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	zassert_false(llcp_tx_alloc_peek(ctxs[0]), NULL);

	/* Now global pool is exausted, but conn pool is not */
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(ctxs[1]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[1]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}

	zassert_false(llcp_tx_alloc_peek(ctxs[1]), NULL);
	ull_cp_release_tx(ctxs[0]->conn_handle, tx[1]);

	/* global pool is now 'open' again, but ctxs[1] is NOT next in line */
	zassert_false(llcp_tx_alloc_peek(ctxs[1]), NULL);
	/* ... ctxs[0] is */
	zassert_true(llcp_tx_alloc_peek(ctxs[0]), NULL);
	tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[0]);
	zassert_not_null(tx[tx_alloc_idx], NULL);
	tx_alloc_idx++;
	ull_cp_release_tx(ctxs[0]->conn_handle, tx[tx_alloc_idx - 1]);

	/* conn pool allows */
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(ctxs[2]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[2]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	/* global pool does not as ctxs[2] is NOT next up */
	zassert_false(llcp_tx_alloc_peek(ctxs[2]), NULL);

	/* ... now ctxs[1] is */
	zassert_true(llcp_tx_alloc_peek(ctxs[1]), NULL);
	tx[tx_alloc_idx] = llcp_tx_alloc(ctxs[0]);
	zassert_not_null(tx[tx_alloc_idx], NULL);
	tx_alloc_idx++;
}

#endif /* (CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE > 0) */

void test_main(void)
{
#if (CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE > 0)
	ztest_test_suite(
		tx_buffer_alloc, ztest_unit_test_setup_teardown(test_tx_buffer_queue, setup,
								unit_test_noop));

	ztest_run_test_suite(tx_buffer_alloc);
#endif /* (CONFIG_BT_CTLR_LLCP_TX_BUFFER_QUEUE_SIZE > 0) */

}
