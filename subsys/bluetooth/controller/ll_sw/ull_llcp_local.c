/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_internal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_conn_internal.h"

#include <soc.h>
#include "hal/debug.h"

static struct proc_ctx *lr_dequeue(struct ll_conn *conn);

/* LLCP Local Request FSM State */
enum lr_state {
	LR_STATE_IDLE,
	LR_STATE_ACTIVE,
	LR_STATE_DISCONNECT,
	LR_STATE_TERMINATE,
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

void llcp_lr_check_done(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (ctx->done) {
		struct proc_ctx *ctx_header;

		ctx_header = llcp_lr_peek(conn);
		LL_ASSERT(ctx_header == ctx);

		lr_dequeue(conn);

		llcp_proc_ctx_release(ctx);
	}
}

/*
 * LLCP Local Request Shared Data Locking
 */

static ALWAYS_INLINE uint32_t shared_data_access_lock(void)
{
	bool enabled;

	if (mayfly_is_running()) {
		/* We are in Mayfly context, nothing to be done */
		return false;
	}

	/* We are in thread context and have to disable TICKER_USER_ID_ULL_HIGH */
	enabled = mayfly_is_enabled(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH) != 0U;
	mayfly_enable(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH, 0U);

	return enabled;
}

static ALWAYS_INLINE void shared_data_access_unlock(bool key)
{
	if (key) {
		/* We are in thread context and have to reenable TICKER_USER_ID_ULL_HIGH */
		mayfly_enable(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH, 1U);
	}
}

/*
 * LLCP Local Request FSM
 */

static void lr_set_state(struct ll_conn *conn, enum lr_state state)
{
	conn->llcp.local.state = state;
}

void llcp_lr_enqueue(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* This function is called from both Thread and Mayfly (ISR),
	 * make sure only a single context have access at a time.
	 */

	bool key = shared_data_access_lock();

	sys_slist_append(&conn->llcp.local.pend_proc_list, &ctx->node);

	shared_data_access_unlock(key);
}

static struct proc_ctx *lr_dequeue(struct ll_conn *conn)
{
	/* This function is called from both Thread and Mayfly (ISR),
	 * make sure only a single context have access at a time.
	 */

	struct proc_ctx *ctx;

	bool key = shared_data_access_lock();

	ctx = (struct proc_ctx *)sys_slist_get(&conn->llcp.local.pend_proc_list);

	shared_data_access_unlock(key);

	return ctx;
}

struct proc_ctx *llcp_lr_peek(struct ll_conn *conn)
{
	/* This function is called from both Thread and Mayfly (ISR),
	 * make sure only a single context have access at a time.
	 */
	struct proc_ctx *ctx;

	bool key = shared_data_access_lock();

	ctx = (struct proc_ctx *)sys_slist_peek_head(&conn->llcp.local.pend_proc_list);

	shared_data_access_unlock(key);

	return ctx;
}

struct proc_ctx *llcp_lr_peek_proc(struct ll_conn *conn, uint8_t proc)
{
	/* This function is called from both Thread and Mayfly (ISR),
	 * make sure only a single context have access at a time.
	 */

	struct proc_ctx *ctx, *tmp;

	bool key = shared_data_access_lock();

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->llcp.local.pend_proc_list, ctx, tmp, node) {
		if (ctx->proc == proc) {
			break;
		}
	}

	shared_data_access_unlock(key);

	return ctx;
}

bool llcp_lr_ispaused(struct ll_conn *conn)
{
	return conn->llcp.local.pause == 1U;
}

void llcp_lr_pause(struct ll_conn *conn)
{
	conn->llcp.local.pause = 1U;
}

void llcp_lr_resume(struct ll_conn *conn)
{
	conn->llcp.local.pause = 0U;
}

void llcp_lr_prt_restart(struct ll_conn *conn)
{
	conn->llcp.local.prt_expire = conn->llcp.prt_reload;
}

void llcp_lr_prt_restart_with_value(struct ll_conn *conn, uint16_t value)
{
	conn->llcp.local.prt_expire = value;
}

void llcp_lr_prt_stop(struct ll_conn *conn)
{
	conn->llcp.local.prt_expire = 0U;
}

void llcp_lr_flush_procedures(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	/* Flush all pending procedures */
	ctx = lr_dequeue(conn);
	while (ctx) {
		llcp_nodes_release(conn, ctx);
		llcp_proc_ctx_release(ctx);
		ctx = lr_dequeue(conn);
	}
}

void llcp_lr_rx(struct ll_conn *conn, struct proc_ctx *ctx, memq_link_t *link,
		struct node_rx_pdu *rx)
{
	/* Store RX node and link */
	ctx->node_ref.rx = rx;
	ctx->node_ref.link = link;

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		llcp_lp_enc_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		llcp_lp_pu_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_UPDATE:
	case PROC_CONN_PARAM_REQ:
		llcp_lp_cu_rx(conn, ctx, rx);
		break;
	case PROC_TERMINATE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case PROC_CHAN_MAP_UPDATE:
		llcp_lp_chmu_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	case PROC_CIS_CREATE:
		llcp_lp_cc_rx(conn, ctx, rx);
		break;
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		llcp_lp_comm_rx(conn, ctx, rx);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
		break;
	}

	llcp_lr_check_done(conn, ctx);
}

void llcp_lr_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		llcp_lp_comm_tx_ack(conn, ctx, tx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_TERMINATE:
		llcp_lp_comm_tx_ack(conn, ctx, tx);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_lp_comm_tx_ack(conn, ctx, tx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		llcp_lp_pu_tx_ack(conn, ctx, tx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		llcp_lp_comm_tx_ack(conn, ctx, tx);
		break;
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */
	default:
		break;
		/* Ignore tx_ack */
	}

	/* Clear TX node reference */
	ctx->node_ref.tx_ack = NULL;

	llcp_lr_check_done(conn, ctx);
}

void llcp_lr_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_PHY)
	case PROC_PHY_UPDATE:
		llcp_lp_pu_tx_ntf(conn, ctx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	default:
		/* Ignore other procedures */
		break;
	}

	llcp_lr_check_done(conn, ctx);
}

static void lr_act_run(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		llcp_lp_enc_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		llcp_lp_pu_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_UPDATE:
	case PROC_CONN_PARAM_REQ:
		llcp_lp_cu_run(conn, ctx, NULL);
		break;
	case PROC_TERMINATE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case PROC_CHAN_MAP_UPDATE:
		llcp_lp_chmu_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		/* 3rd partam null? */
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	case PROC_CIS_CREATE:
		llcp_lp_cc_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		llcp_lp_comm_run(conn, ctx, NULL);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
		break;
	}

	llcp_lr_check_done(conn, ctx);
}

static void lr_act_complete(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	LL_ASSERT(ctx != NULL);

	/* Stop procedure response timeout timer */
	llcp_lr_prt_stop(conn);

	/* Mark the procedure as safe to delete */
	ctx->done = 1U;
}

static void lr_act_connect(struct ll_conn *conn)
{
	/* Empty on purpose */
}

static void lr_act_disconnect(struct ll_conn *conn)
{
	llcp_lr_flush_procedures(conn);
}

static void lr_st_disconnect(struct ll_conn *conn, uint8_t evt, void *param)
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

static void lr_st_idle(struct ll_conn *conn, uint8_t evt, void *param)
{
	struct proc_ctx *ctx;

	switch (evt) {
	case LR_EVT_RUN:
		ctx = llcp_lr_peek(conn);
		if (ctx) {
			/*
			 * since the call to lr_act_run may release the context we need to remember
			 * which procedure we are running
			 */
			const enum llcp_proc curr_proc = ctx->proc;
			lr_act_run(conn);
			if (curr_proc != PROC_TERMINATE) {
				lr_set_state(conn, LR_STATE_ACTIVE);
			} else {
				lr_set_state(conn, LR_STATE_TERMINATE);
			}
		}
		break;
	case LR_EVT_DISCONNECT:
		lr_act_disconnect(conn);
		lr_set_state(conn, LR_STATE_DISCONNECT);
		break;
	case LR_EVT_COMPLETE:
		/* Some procedures like CTE request may be completed without actual run due to
		 * change in conditions while the procedure was waiting in a queue.
		 */
		lr_act_complete(conn);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_st_active(struct ll_conn *conn, uint8_t evt, void *param)
{
	switch (evt) {
	case LR_EVT_RUN:
		if (llcp_lr_peek(conn)) {
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

static void lr_st_terminate(struct ll_conn *conn, uint8_t evt, void *param)
{
	switch (evt) {
	case LR_EVT_RUN:
		if (llcp_lr_peek(conn)) {
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

static void lr_execute_fsm(struct ll_conn *conn, uint8_t evt, void *param)
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
	case LR_STATE_TERMINATE:
		lr_st_terminate(conn, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_lr_init(struct ll_conn *conn)
{
	lr_set_state(conn, LR_STATE_DISCONNECT);
	conn->llcp.local.prt_expire = 0U;
}

void llcp_lr_run(struct ll_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_RUN, NULL);
}

void llcp_lr_complete(struct ll_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_COMPLETE, NULL);
}

void llcp_lr_connect(struct ll_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_CONNECT, NULL);
}

void llcp_lr_disconnect(struct ll_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_DISCONNECT, NULL);
}

void llcp_lr_terminate(struct ll_conn *conn)
{

	llcp_lr_flush_procedures(conn);
	llcp_lr_prt_stop(conn);
	llcp_rr_set_incompat(conn, 0U);
	lr_set_state(conn, LR_STATE_IDLE);
}

#ifdef ZTEST_UNITTEST

bool llcp_lr_is_disconnected(struct ll_conn *conn)
{
	return conn->llcp.local.state == LR_STATE_DISCONNECT;
}

bool llcp_lr_is_idle(struct ll_conn *conn)
{
	return conn->llcp.local.state == LR_STATE_IDLE;
}

struct proc_ctx *llcp_lr_dequeue(struct ll_conn *conn)
{
	return lr_dequeue(conn);
}

#endif
