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
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_remote
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Remote Request FSM State */
enum rr_state {
	RR_STATE_IDLE,
	RR_STATE_REJECT,
	RR_STATE_ACTIVE,
	RR_STATE_DISCONNECT
};

/* LLCP Remote Request FSM Event */
enum {
	/* Procedure prepare */
	RR_EVT_PREPARE,

	/* Procedure run */
	RR_EVT_RUN,

	/* Procedure completed */
	RR_EVT_COMPLETE,

	/* Link connected */
	RR_EVT_CONNECT,

	/* Link disconnected */
	RR_EVT_DISCONNECT,
};

static bool proc_with_instant(struct proc_ctx *ctx)
{
	switch (ctx->proc) {
	case PROC_FEATURE_EXCHANGE:
		return 0U;
		break;
	case PROC_MIN_USED_CHANS:
		return 0U;
		break;
	case PROC_LE_PING:
		return 0U;
		break;
	case PROC_VERSION_EXCHANGE:
		return 0U;
		break;
	case PROC_ENCRYPTION_START:
		return 0U;
		break;
	case PROC_PHY_UPDATE:
		return 1U;
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	return 0U;
}

/*
 * LLCP Remote Request FSM
 */

static void rr_set_state(struct ull_cp_conn *conn, enum rr_state state)
{
	conn->llcp.remote.state = state;
}

void ull_cp_priv_rr_set_incompat(struct ull_cp_conn *conn, enum proc_incompat incompat)
{
	conn->llcp.remote.incompat = incompat;
}

static enum proc_incompat rr_get_incompat(struct ull_cp_conn *conn)
{
	return conn->llcp.remote.incompat;
}

static void rr_set_collision(struct ull_cp_conn *conn, bool collision)
{
	conn->llcp.remote.collision = collision;
}

bool ull_cp_priv_rr_get_collision(struct ull_cp_conn *conn)
{
	return conn->llcp.remote.collision;
}

static void rr_enqueue(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	sys_slist_append(&conn->llcp.remote.pend_proc_list, &ctx->node);
}

static struct proc_ctx *rr_dequeue(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_get(&conn->llcp.remote.pend_proc_list);
	return ctx;
}

struct proc_ctx *rr_peek(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_peek_head(&conn->llcp.remote.pend_proc_list);
	return ctx;
}

void ull_cp_priv_rr_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		rp_comm_rx(conn, ctx, rx);
		break;
	case PROC_FEATURE_EXCHANGE:
		rp_comm_rx(conn, ctx, rx);
		break;
	case PROC_MIN_USED_CHANS:
		rp_comm_rx(conn, ctx, rx);
		break;
	case PROC_VERSION_EXCHANGE:
		rp_comm_rx(conn, ctx, rx);
		break;
	case PROC_ENCRYPTION_START:
		rp_enc_rx(conn, ctx, rx);
		break;
	case PROC_PHY_UPDATE:
		rp_pu_rx(conn, ctx, rx);
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rr_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	switch (ctx->proc) {
	default:
		/* Ignore tx_ack */
		break;
	}
}

static void rr_act_run(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);

	switch (ctx->proc) {
	case PROC_LE_PING:
		rp_comm_run(conn, ctx, NULL);
		break;
	case PROC_FEATURE_EXCHANGE:
		rp_comm_run(conn, ctx, NULL);
		break;
	case PROC_MIN_USED_CHANS:
		rp_comm_run(conn, ctx, NULL);
		break;
	case PROC_VERSION_EXCHANGE:
		rp_comm_run(conn, ctx, NULL);
		break;
	case PROC_ENCRYPTION_START:
		rp_enc_run(conn, ctx, NULL);
		break;
	case PROC_PHY_UPDATE:
		rp_pu_run(conn, ctx, NULL);
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void rr_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* TODO(thoh): Select between LL_REJECT_IND and LL_REJECT_EXT_IND */
		pdu_encode_reject_ext_ind(pdu, conn->llcp.remote.reject_opcode, BT_HCI_ERR_LL_PROC_COLLISION);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rr_act_reject(struct ull_cp_conn *conn)
{
	if (!tx_alloc_is_available()) {
		rr_set_state(conn, RR_STATE_REJECT);
	} else {
		struct proc_ctx *ctx = rr_peek(conn);

		rr_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_IND);

		/* Dequeue pending request that just completed */
		(void) rr_dequeue(conn);

		rr_set_state(conn, RR_STATE_IDLE);
	}
}

static void rr_act_complete(struct ull_cp_conn *conn)
{
	rr_set_collision(conn, 0U);

	/* Dequeue pending request that just completed */
	(void) rr_dequeue(conn);
}

static void rr_act_connect(struct ull_cp_conn *conn)
{
	/* TODO */
}

static void rr_act_disconnect(struct ull_cp_conn *conn)
{
	rr_dequeue(conn);
}

static void rr_st_disconnect(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (evt) {
	case RR_EVT_CONNECT:
		rr_act_connect(conn);
		rr_set_state(conn, RR_STATE_IDLE);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rr_st_idle(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	struct proc_ctx *ctx;

	switch (evt) {
	case RR_EVT_PREPARE:
		if ((ctx = rr_peek(conn))){
			const enum proc_incompat incompat = rr_get_incompat(conn);
			const bool slave = !!(conn->lll.role == BT_HCI_ROLE_SLAVE);
			const bool master = !!(conn->lll.role == BT_HCI_ROLE_MASTER);
			const bool with_instant = proc_with_instant(ctx);

			if (incompat == INCOMPAT_NO_COLLISION) {
				/* No collision
				 * => Run procedure
				 *
				 * Local incompatible procedure request is kept pending.
				 */

				/* Pause local incompatible procedure */
				rr_set_collision(conn, with_instant);

				/* Run remote procedure */
				rr_act_run(conn);
				conn->llcp.remote.state = RR_STATE_ACTIVE;
			} else if (slave && incompat == INCOMPAT_RESOLVABLE) {
				/* Slave collision
				 * => Run procedure
				 *
				 * Local slave procedure completes with error.
				 */

				/* Run remote procedure */
				rr_act_run(conn);
				rr_set_state(conn, RR_STATE_ACTIVE);
			} else if (with_instant && master && incompat == INCOMPAT_RESOLVABLE) {
				/* Master collision
				 * => Send reject
				 *
				 * Local master incompatible procedure continues unaffected.
				 */

				/* Send reject */
				struct node_rx_pdu *rx = (struct node_rx_pdu *) param;
				struct pdu_data *pdu = (struct pdu_data *) rx->pdu;
				conn->llcp.remote.reject_opcode = pdu->llctrl.opcode;
				rr_act_reject(conn);
			} else if (with_instant && incompat == INCOMPAT_RESERVED) {
				 /* Protocol violation.
				 * => Disconnect
				 *
				 */

				/* TODO */
				LL_ASSERT(0);
			}
		}
		break;
	case RR_EVT_DISCONNECT:
		rr_act_disconnect(conn);
		rr_set_state(conn, RR_STATE_DISCONNECT);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
static void rr_st_reject(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	/* TODO */
	LL_ASSERT(0);
}

static void rr_st_active(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (evt) {
	case RR_EVT_RUN:
		if (rr_peek(conn)) {
			rr_act_run(conn);
		}
		break;
	case RR_EVT_COMPLETE:
		rr_act_complete(conn);
		rr_set_state(conn, RR_STATE_IDLE);
		break;
	case RR_EVT_DISCONNECT:
		rr_act_disconnect(conn);
		rr_set_state(conn, RR_STATE_DISCONNECT);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rr_execute_fsm(struct ull_cp_conn *conn, u8_t evt, void *param)
{
	switch (conn->llcp.remote.state) {
	case RR_STATE_DISCONNECT:
		rr_st_disconnect(conn, evt, param);
		break;
	case RR_STATE_IDLE:
		rr_st_idle(conn, evt, param);
		break;
	case RR_STATE_REJECT:
		rr_st_reject(conn, evt, param);
		break;
	case RR_STATE_ACTIVE:
		rr_st_active(conn, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
	/*
	 * EGON TODO: is this the right place to release the ctx if we
	 * received the RR_EVT_COMPLETE?
	 */
}

void ull_cp_priv_rr_init(struct ull_cp_conn *conn)
{
	rr_set_state(conn, RR_STATE_DISCONNECT);
}

void ull_cp_priv_rr_prepare(struct ull_cp_conn *conn, struct node_rx_pdu *rx)
{
	rr_execute_fsm(conn, RR_EVT_PREPARE, rx);
}

void ull_cp_priv_rr_run(struct ull_cp_conn *conn)
{
	rr_execute_fsm(conn, RR_EVT_RUN, NULL);
}

void ull_cp_priv_rr_complete(struct ull_cp_conn *conn)
{
	rr_execute_fsm(conn, RR_EVT_COMPLETE, NULL);
}

void ull_cp_priv_rr_connect(struct ull_cp_conn *conn)
{
	rr_execute_fsm(conn, RR_EVT_CONNECT, NULL);
}

void ull_cp_priv_rr_disconnect(struct ull_cp_conn *conn)
{
	rr_execute_fsm(conn, RR_EVT_DISCONNECT, NULL);
}

void ull_cp_priv_rr_new(struct ull_cp_conn *conn, struct node_rx_pdu *rx)
{
	struct proc_ctx *ctx;
	struct pdu_data *pdu;
	u8_t proc;

	pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		proc = PROC_FEATURE_EXCHANGE;
		break;
	case PDU_DATA_LLCTRL_TYPE_PING_REQ:
		proc = PROC_LE_PING;
		break;
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		proc = PROC_VERSION_EXCHANGE;
		break;
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		proc = PROC_ENCRYPTION_START;
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		proc = PROC_PHY_UPDATE;
		break;
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		proc = PROC_MIN_USED_CHANS;
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
		break;
	}

	ctx = create_remote_procedure(proc);
	if (!ctx) {
		return;
	}

	/* Enqueue procedure */
	rr_enqueue(conn, ctx);

	/* Prepare procedure */
	rr_prepare(conn, rx);

	/* Handle PDU */
	ctx = rr_peek(conn);
	if (ctx) {
		rr_rx(conn, ctx, rx);
	}
}

#ifdef ZTEST_UNITTEST

bool rr_is_disconnected(struct ull_cp_conn *conn)
{
	return conn->llcp.remote.state == RR_STATE_DISCONNECT;
}

bool rr_is_idle(struct ull_cp_conn *conn)
{
	return conn->llcp.remote.state == RR_STATE_IDLE;
}

void test_int_remote_pending_requests(void)
{
	struct ull_cp_conn conn;
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	peek_ctx = rr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = rr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	rr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *) sys_slist_peek_head(&conn.llcp.remote.pend_proc_list);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	peek_ctx = rr_peek(&conn);
	zassert_equal_ptr(peek_ctx, &ctx, NULL);

	dequeue_ctx = rr_dequeue(&conn);
	zassert_equal_ptr(dequeue_ctx, &ctx, NULL);

	peek_ctx = rr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = rr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);
}

#endif
