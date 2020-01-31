/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>

/* Kconfig Cheats */
#define CONFIG_BT_LOG_LEVEL 1
#define CONFIG_BT_CTLR_COMPANY_ID 0x1234
#define CONFIG_BT_CTLR_SUBVERSION_NUMBER 0x5678

#define ULL_LLCP_UNITTEST

/* Implementation Under Test Begin */
#include "ll_sw/ull_llcp.c"
/* Implementation Under Test End */

struct ull_tx_q tx_q;
struct ull_cp_conn conn;

u8_t buffer_mem_tx[TX_CTRL_BUF_SIZE * 1];
u8_t buffer_mem_ctx[TX_CTRL_BUF_SIZE * 1];
struct mem_pool mem_tx = { .pool = buffer_mem_tx };
struct mem_pool mem_ctx = { .pool = buffer_mem_ctx };
const size_t sz_mem_tx = TX_CTRL_BUF_SIZE;
const u32_t ncount_mem_tx = 1;
const size_t sz_mem_ctx = PROC_CTX_BUF_SIZE;
const u32_t ncount_mem_ctx = 1;

void test_init(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);

	ull_cp_conn_init(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
}

void test_mem(void)
{
	struct proc_ctx *ctx1;
	struct proc_ctx *ctx2;
	bool peek;
	struct node_tx *tx;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	ctx1 = proc_ctx_acquire();
	zassert_not_null(ctx1, NULL);
	zassert_equal_ptr(ctx1, mem_ctx.pool, NULL);

	ctx2 = proc_ctx_acquire();
	zassert_is_null(ctx2, NULL);

	proc_ctx_release(ctx1);
	ctx1 = proc_ctx_acquire();
	zassert_not_null(ctx1, NULL);
	zassert_equal_ptr(ctx1, mem_ctx.pool, NULL);

	peek = tx_alloc_peek();
	zassert_true(peek, NULL);

	tx = tx_alloc();
	zassert_not_null(tx, NULL);
	zassert_equal_ptr(tx, mem_tx.pool, NULL);

	peek = tx_alloc_peek();
	zassert_false(peek, NULL);
}

void test_create_proc(void)
{
	struct proc_ctx *ctx;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);
	zassert_equal_ptr(ctx, mem_ctx.pool, NULL);

	zassert_equal(ctx->proc, PROC_VERSION_EXCHANGE, NULL);
	zassert_equal(ctx->state, LP_COMMON_STATE_IDLE, NULL);
	zassert_equal(ctx->collision, 0, NULL);
	zassert_equal(ctx->pause, 0, NULL);

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	zassert_is_null(ctx, NULL);
}

void test_llcp_api_proc(void)
{
	int err;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	err = version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	err = version_exchange(&conn);
	zassert_not_equal(err, BT_HCI_ERR_SUCCESS, NULL);
}

void test_lr_pending_requests(void)
{
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	peek_ctx = lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = lr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	lr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *) sys_slist_peek_head(&conn.local.pend_proc_list);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	peek_ctx = lr_peek(&conn);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	dequeue_ctx = lr_dequeue(&conn);
	zassert_equal_ptr(dequeue_ctx, &ctx, NULL);

	peek_ctx = lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = lr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);
}

void test_lr_connect(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	lr_connect(&conn);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);
}

void test_lr_disconnect(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	lr_disconnect(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);

	lr_connect(&conn);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);

	lr_disconnect(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
}

void helper_prepare_version_ind(void)
{
	int err;
	struct proc_ctx *peek_ctx;
	struct node_tx *tx;
	struct pdu_data *pdu;
	u16_t cid;
	u16_t svn;

	lr_prepare(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);

	lr_connect(&conn);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);

	lr_prepare(&conn);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);

	err = version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	peek_ctx = lr_peek(&conn);
	zassert_not_null(peek_ctx, NULL);

	zassert_equal(peek_ctx->proc, PROC_VERSION_EXCHANGE, NULL);
	zassert_equal(peek_ctx->state, LP_COMMON_STATE_IDLE, NULL);
	zassert_equal(peek_ctx->collision, 0, NULL);
	zassert_equal(peek_ctx->pause, 0, NULL);

	lr_prepare(&conn);
	zassert_equal(conn.local.state, LR_STATE_ACTIVE, NULL);

	peek_ctx = lr_peek(&conn);
	zassert_equal(peek_ctx->state, LP_COMMON_STATE_WAIT_RX, NULL);

	tx = ull_tx_q_dequeue(&tx_q);
	zassert_not_null(tx, NULL);

	pdu = (struct pdu_data *)tx->pdu;
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_VERSION_IND, NULL);

	zassert_equal(pdu->llctrl.version_ind.version_number, LL_VERSION_NUMBER, NULL);

	cid = sys_cpu_to_le16(CONFIG_BT_CTLR_COMPANY_ID);
	svn = sys_cpu_to_le16(CONFIG_BT_CTLR_SUBVERSION_NUMBER);
	zassert_equal(pdu->llctrl.version_ind.company_id, cid, NULL);
	zassert_equal(pdu->llctrl.version_ind.sub_version_number, svn, NULL);

	tx = ull_tx_q_dequeue(&tx_q);
	zassert_is_null(tx, NULL);
}

void test_lr_idle_prepare(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	helper_prepare_version_ind();
}

void test_lr_active_complete(void)
{
	struct proc_ctx *peek_ctx;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	helper_prepare_version_ind();

	lr_complete(&conn);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);
	peek_ctx = lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);
}

void test_lr_active_disconnect(void)
{
	struct proc_ctx *peek_ctx;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	helper_prepare_version_ind();

	lr_disconnect(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
	peek_ctx = lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);
}

void test_main(void)
{
	ztest_test_suite(test,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_mem),
			 ztest_unit_test(test_create_proc),
			 ztest_unit_test(test_llcp_api_proc),
			 ztest_unit_test(test_lr_pending_requests),
			 ztest_unit_test(test_lr_connect),
			 ztest_unit_test(test_lr_disconnect),
			 ztest_unit_test(test_lr_idle_prepare),
			 ztest_unit_test(test_lr_active_complete),
			 ztest_unit_test(test_lr_active_disconnect)
			 );
	ztest_run_test_suite(test);
}
