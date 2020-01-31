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
#include "lll.h"
#include "lll_conn.h"

#include "ll.h"
#include "ll_settings.h"

#include "ull_tx_queue.h"
#include "ull_llcp.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure Common FSM states */
enum {
	LP_COMMON_STATE_IDLE,
	LP_COMMON_STATE_WAIT_TX,
	LP_COMMON_STATE_WAIT_RX,
	LP_COMMON_STATE_WAIT_NTF,
};

/* LLCP Local Procedure Common FSM events */
enum {
	/* Procedure prepared */
	LP_COMMON_EVT_PREPARE,

	/* Response recieved */
	LP_COMMON_EVT_RESPONSE,

	/* Reject response recieved */
	LP_COMMON_EVT_REJECT,

	/* Unknown response recieved */
	LP_COMMON_EVT_UNKNOWN,

	/* Instant collision detected */
	LP_COMMON_EVT_COLLISION,
};

/* LLCP Procedure */
enum {
	PROC_UNKNOWN,
	PROC_VERSION_EXCHANGE
};

/* LLCP Local Request FSM State */
enum {
	LR_STATE_IDLE,
	LR_STATE_ACTIVE,
	LR_STATE_DISCONNECT
};

/* LLCP Local Request FSM Event */
enum {
	/* Procedure prepared */
	LR_EVT_PREPARE,

	/* Procedure completed */
	LR_EVT_COMPLETE,

	/* Link connected */
	LR_EVT_CONNECT,

	/* Link disconnected */
	LR_EVT_DISCONNECT,
};

/* LLCP Procedure Context */
struct proc_ctx {
	/* Must be the first for sys_slist to work */
	sys_snode_t node;

	/* PROC_ */
	u8_t proc;

	/* LP_STATE_ */
	u8_t state;

	u8_t opcode;

	int collision;
	int pause;
};

/* LLCP Memory Pool Descriptor */
struct mem_pool {
	void *free;
	u8_t *pool;
};

#define PROC_CTX_BUF_SIZE       WB_UP(sizeof(struct proc_ctx))
#define TX_CTRL_BUF_SIZE        WB_UP(offsetof(struct node_tx, pdu) + offsetof(struct pdu_data, llctrl) + sizeof(struct pdu_data_llctrl))

#if !defined(ULL_LLCP_UNITTEST)

/* LLCP Allocations (normal) */

static u8_t buffer_mem_tx[TX_CTRL_BUF_SIZE * 1];
static struct mem_pool mem_tx = { .pool = buffer_mem_tx };
static const size_t sz_mem_tx = TX_CTRL_BUF_SIZE;
static const u32_t ncount_mem_tx = 1;

static u8_t buffer_mem_ctx[TX_CTRL_BUF_SIZE * 1];
static struct mem_pool mem_ctx = { .pool = buffer_mem_ctx };
static const size_t sz_mem_ctx = PROC_CTX_BUF_SIZE;
static const u32_t ncount_mem_ctx = 1;

#else

/* LLCP Allocations (unittest) */

extern const size_t sz_mem_tx;
extern const u32_t ncount_mem_tx;
extern struct mem_pool mem_tx;

extern const size_t sz_mem_ctx;
extern const u32_t ncount_mem_ctx;
extern struct mem_pool mem_ctx;

#endif

/*
 * LLCP Resource Management
 */

static struct proc_ctx *proc_ctx_acquire(void)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) mem_acquire(&mem_ctx.free);
	return ctx;
}

static void proc_ctx_release(struct proc_ctx *ctx)
{
	mem_release(ctx, &mem_ctx.free);
}

static bool tx_alloc_peek(void)
{
	u16_t mem_free_count;

	mem_free_count = mem_free_count_get(mem_tx.free);
	return mem_free_count > 0;
}

static struct node_tx *tx_alloc(void)
{
	struct node_tx *tx;

	tx = (struct node_tx *) mem_acquire(&mem_tx.free);
	return tx;
}

/*
 * Version Exchange Procedure Helper
 */

static void pdu_encode_version_ind(struct pdu_data *pdu)
{
	u16_t cid;
	u16_t svn;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) + sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
	pdu->llctrl.version_ind.version_number = LL_VERSION_NUMBER;
	cid = sys_cpu_to_le16(ll_settings_company_id());
	svn = sys_cpu_to_le16(ll_settings_subversion_number());
	pdu->llctrl.version_ind.company_id = cid;
	pdu->llctrl.version_ind.sub_version_number = svn;
}

/*
 * LLCP Local Procedure Common FSM
 */

static void lp_comm_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Assemble LL Control PDU */
	switch (ctx->proc) {
	case PROC_VERSION_EXCHANGE:
		pdu_encode_version_ind(pdu);
		ctx->opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	/* Enqueue LL Control PDU towards LLL */
	ull_tx_q_enqueue_ctrl(conn->tx_q, tx);
}


static void lp_comm_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt)
{
	switch (evt) {
	case LP_COMMON_EVT_PREPARE:
		if (!tx_alloc_peek() || ctx->pause || ctx->collision) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt)
{
	/* TODO */
}

static void lp_comm_st_wait_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt)
{
	/* TODO */
}

static void lp_comm_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt)
{
	/* TODO */
}

static void lp_comm_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt)
{
	switch (ctx->state) {
	case LP_COMMON_STATE_IDLE:
		lp_comm_st_idle(conn, ctx, evt);
		break;
	case LP_COMMON_STATE_WAIT_TX:
		lp_comm_st_wait_tx(conn, ctx, evt);
		break;
	case LP_COMMON_STATE_WAIT_RX:
		lp_comm_st_wait_rx(conn, ctx, evt);
		break;
	case LP_COMMON_STATE_WAIT_NTF:
		lp_comm_st_wait_ntf(conn, ctx, evt);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

/*
 * LLCP Local Request FSM
 */

static void lr_enqueue(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	sys_slist_append(&conn->local.pend_proc_list, &ctx->node);
}

static struct proc_ctx *lr_dequeue(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_get(&conn->local.pend_proc_list);
	return ctx;
}

static struct proc_ctx *lr_peek(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) sys_slist_peek_head(&conn->local.pend_proc_list);
	return ctx;
}

static void lr_act_prepare(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = lr_peek(conn);

	switch (ctx->proc) {
	case PROC_VERSION_EXCHANGE:
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_PREPARE);
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

static void lr_st_disconnect(struct ull_cp_conn *conn, u8_t evt)
{
	switch (evt) {
	case LR_EVT_CONNECT:
		lr_act_connect(conn);
		conn->local.state = LR_STATE_IDLE;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_st_idle(struct ull_cp_conn *conn, u8_t evt)
{
	switch (evt) {
	case LR_EVT_PREPARE:
		if (lr_peek(conn)) {
			lr_act_prepare(conn);
			conn->local.state = LR_STATE_ACTIVE;
		}
		break;
	case LR_EVT_DISCONNECT:
		lr_act_disconnect(conn);
		conn->local.state = LR_STATE_DISCONNECT;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_st_active(struct ull_cp_conn *conn, u8_t evt)
{
	switch (evt) {
	case LR_EVT_COMPLETE:
		lr_act_complete(conn);
		conn->local.state = LR_STATE_IDLE;
		break;
	case LR_EVT_DISCONNECT:
		lr_act_disconnect(conn);
		conn->local.state = LR_STATE_DISCONNECT;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lr_execute_fsm(struct ull_cp_conn *conn, u8_t evt)
{
	switch (conn->local.state) {
	case LR_STATE_DISCONNECT:
		lr_st_disconnect(conn, evt);
		break;
	case LR_STATE_IDLE:
		lr_st_idle(conn, evt);
		break;
	case LR_STATE_ACTIVE:
		lr_st_active(conn, evt);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

static void lr_prepare(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_PREPARE);
}

static void lr_complete(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_COMPLETE);
}

static void lr_connect(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_CONNECT);
}

static void lr_disconnect(struct ull_cp_conn *conn)
{
	lr_execute_fsm(conn, LR_EVT_DISCONNECT);
}

/*
 * LLCP Procedure Creation
 */

static struct proc_ctx *create_procedure(u8_t proc)
{
	struct proc_ctx *ctx;

	ctx = proc_ctx_acquire();
	if (!ctx) {
		return NULL;
	}

	ctx->proc = proc;
	ctx->state = LP_COMMON_STATE_IDLE;
	ctx->collision = 0U;
	ctx->pause = 0U;

	return ctx;
}

/*
 * LLCP Procedure Helper
 */

static int version_exchange(struct ull_cp_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

/*
 * LLCP Public API
 */

void ull_cp_init(void)
{
	/**/
	mem_init(mem_ctx.pool, sz_mem_ctx, ncount_mem_ctx, &mem_ctx.free);
	mem_init(mem_tx.pool, sz_mem_tx, ncount_mem_tx, &mem_tx.free);
}

void ull_cp_conn_init(struct ull_cp_conn *conn)
{
	/**/
	conn->local.state = LR_STATE_DISCONNECT;
	sys_slist_init(&conn->local.pend_proc_list);
}

void ull_cp_prepare(struct ull_cp_conn *conn)
{
	lr_prepare(conn);
}

void ull_cp_rx(struct ull_cp_conn *conn, struct node_rx_pdu *rx)
{
}
