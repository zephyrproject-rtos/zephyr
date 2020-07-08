/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_local
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Request FSM State */
enum lr_state {
	LR_STATE_IDLE,
	LR_STATE_ACTIVE,
	LR_STATE_DISCONNECT
};

/* LLCP Local Request FSM Event */
enum {
	/* Procedure run */
	LR_EVT_RUN,

	/* Procedure completed */
	LR_EVT_COMPLETE,

	/* Link connected */
	LR_EVT_CONNECT,

	/* Link disconnected */
	LR_EVT_DISCONNECT,
};

/*
 * LLCP Local Request FSM
 */

static void lr_set_state(struct ull_cp_conn *conn, enum lr_state state)
{
	conn->llcp.local.state = state;
}

void ull_cp_priv_lr_enqueue(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	sys_slist_append(&conn->llcp.local.pend_proc_list, &ctx->node);
}

static struct proc_ctx *lr_dequeue(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_get(&conn->llcp.local.pend_proc_list);
	return ctx;
}

struct proc_ctx *lr_peek(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_peek_head(&conn->llcp.local.pend_proc_list);
	return ctx;
}

void ull_cp_priv_lr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		lp_comm_rx(conn, ctx, rx);
		break;
	case PROC_FEATURE_EXCHANGE:
		lp_comm_rx(conn, ctx, rx);
		break;
	case PROC_MIN_USED_CHANS:
		lp_comm_rx(conn, ctx, rx);
		break;
	case PROC_VERSION_EXCHANGE:
		lp_comm_rx(conn, ctx, rx);
		break;
	case PROC_ENCRYPTION_START:
		lp_enc_rx(conn, ctx, rx);
		break;
	case PROC_PHY_UPDATE:
		lp_pu_rx(conn, ctx, rx);
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	switch (ctx->proc) {
	case PROC_MIN_USED_CHANS:
		lp_comm_tx_ack(conn, ctx, tx);
		break;
	default:
		break;
		/* Ignore tx_ack */
	}
}

static void lr_act_run(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = lr_peek(conn);

	switch (ctx->proc) {
	case PROC_LE_PING:
		lp_comm_run(conn, ctx, NULL);
		break;
	case PROC_FEATURE_EXCHANGE:
		lp_comm_run(conn, ctx, NULL);
		break;
	case PROC_MIN_USED_CHANS:
		lp_comm_run(conn, ctx, NULL);
		break;
	case PROC_VERSION_EXCHANGE:
		lp_comm_run(conn, ctx, NULL);
		break;
	case PROC_ENCRYPTION_START:
		lp_enc_run(conn, ctx, NULL);
		break;
	case PROC_PHY_UPDATE:
		lp_pu_run(conn, ctx, NULL);
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lr_act_complete(struct ull_cp_conn *conn)
{
	/* Dequeue pending request that just completed */
	(void) lr_dequeue(conn);
}

static void lr_act_connect(struct ull_cp_conn *conn)
{
	/* TODO */
}

static void lr_act_disconnect(struct ull_cp_conn *conn)
{
	lr_dequeue(conn);
}

static void lr_st_disconnect(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (evt) {
	case LR_EVT_CONNECT:
		lr_act_connect(conn);
		lr_set_state(conn, LR_STATE_IDLE);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_st_idle(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (evt) {
	case LR_EVT_RUN:
		if (lr_peek(conn)) {
			lr_act_run(conn);
			lr_set_state(conn, LR_STATE_ACTIVE);
		}
		break;
	case LR_EVT_DISCONNECT:
		lr_act_disconnect(conn);
		lr_set_state(conn, LR_STATE_DISCONNECT);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_st_active(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (evt) {
	case LR_EVT_RUN:
		if (lr_peek(conn)) {
			lr_act_run(conn);
		}
		break;
	case LR_EVT_COMPLETE:
		lr_act_complete(conn);
		lr_set_state(conn, LR_STATE_IDLE);
		break;
	case LR_EVT_DISCONNECT:
		lr_act_disconnect(conn);
		lr_set_state(conn, LR_STATE_DISCONNECT);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_execute_fsm(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (conn->llcp.local.state) {
	case LR_STATE_DISCONNECT:
		lr_st_disconnect(conn, evt, param);
		break;
	case LR_STATE_IDLE:
		lr_st_idle(conn, evt, param);
		break;
	case LR_STATE_ACTIVE:
		lr_st_active(conn, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
	/*
	 * EGON TODO: is this the right place to release ctx
	 * if we received the LR_EVT_COMPLETE event?
	 */
}

void ull_cp_priv_lr_init(struct ull_cp_conn *conn)
{
	lr_set_state(conn, LR_STATE_DISCONNECT);
}

void ull_cp_priv_lr_run(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_RUN, NULL);
}

void ull_cp_priv_lr_complete(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_COMPLETE, NULL);
}

void ull_cp_priv_lr_connect(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_CONNECT, NULL);
}

void ull_cp_priv_lr_disconnect(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_DISCONNECT, NULL);
}

#ifdef ZTEST_UNITTEST

bool lr_is_disconnected(struct ull_cp_conn *conn)
{
	return conn->llcp.local.state == LR_STATE_DISCONNECT;
}

bool lr_is_idle(struct ull_cp_conn *conn)
{
	return conn->llcp.local.state == LR_STATE_IDLE;
}

void test_int_local_pending_requests(void)
{
	struct ull_cp_conn conn;
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	peek_ctx = lr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = lr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	lr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *) sys_slist_peek_head(&conn.llcp.local.pend_proc_list);
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

#endif
