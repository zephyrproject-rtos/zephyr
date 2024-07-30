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
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"
#include "ull_central_iso_internal.h"

#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_conn_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"
#include "ull_peripheral_internal.h"

#include <soc.h>
#include "hal/debug.h"

#define LLCTRL_PDU_SIZE (offsetof(struct pdu_data, llctrl) + sizeof(struct pdu_data_llctrl))
#define PROC_CTX_BUF_SIZE WB_UP(sizeof(struct proc_ctx))
#define TX_CTRL_BUF_SIZE WB_UP(offsetof(struct node_tx, pdu) + LLCTRL_PDU_SIZE)
#define NTF_BUF_SIZE WB_UP(offsetof(struct node_rx_pdu, pdu) + LLCTRL_PDU_SIZE)

/* LLCP Allocations */
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
sys_slist_t tx_buffer_wait_list;
static uint8_t common_tx_buffer_alloc;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

static uint8_t MALIGN(4) buffer_mem_tx[TX_CTRL_BUF_SIZE * LLCP_TX_CTRL_BUF_COUNT];
static struct llcp_mem_pool mem_tx = { .pool = buffer_mem_tx };

static uint8_t MALIGN(4) buffer_mem_local_ctx[PROC_CTX_BUF_SIZE *
				    CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM];
static struct llcp_mem_pool mem_local_ctx = { .pool = buffer_mem_local_ctx };

static uint8_t MALIGN(4) buffer_mem_remote_ctx[PROC_CTX_BUF_SIZE *
				     CONFIG_BT_CTLR_LLCP_REMOTE_PROC_CTX_BUF_NUM];
static struct llcp_mem_pool mem_remote_ctx = { .pool = buffer_mem_remote_ctx };

/*
 * LLCP Resource Management
 */
static struct proc_ctx *proc_ctx_acquire(struct llcp_mem_pool *owner)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *)mem_acquire(&owner->free);

	if (ctx) {
		/* Set the owner */
		ctx->owner = owner;
	}

	return ctx;
}

void llcp_proc_ctx_release(struct proc_ctx *ctx)
{
	/* We need to have an owner otherwise the memory allocated would leak */
	LL_ASSERT(ctx->owner);

	/* Release the memory back to the owner */
	mem_release(ctx, &ctx->owner->free);
}

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
/*
 * @brief Update 'global' tx buffer allowance
 */
void ull_cp_update_tx_buffer_queue(struct ll_conn *conn)
{
	if (conn->llcp.tx_buffer_alloc > CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM) {
		common_tx_buffer_alloc -= (conn->llcp.tx_buffer_alloc -
					   CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM);
	}
}


/*
 * @brief Check for per conn pre-allocated tx buffer allowance
 * @return true if buffer is available
 */
static inline bool static_tx_buffer_available(struct ll_conn *conn, struct proc_ctx *ctx)
{
#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	/* Check if per connection pre-aloted tx buffer is available */
	if (conn->llcp.tx_buffer_alloc < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM) {
		/* This connection has not yet used up all the pre-aloted tx buffers */
		return true;
	}
#endif /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0 */
	return false;
}

/*
 * @brief pre-alloc/peek of a tx buffer, leave requester on the wait list (@head if first up)
 *
 * @return true if alloc is allowed, false if not
 *
 */
bool llcp_tx_alloc_peek(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (!static_tx_buffer_available(conn, ctx)) {
		/* The conn already has spent its pre-aloted tx buffer(s),
		 * so we should consider the common tx buffer pool
		 */
		if (ctx->wait_reason == WAITING_FOR_NOTHING) {
			/* The current procedure is not in line for a tx buffer
			 * so sign up on the wait list
			 */
			sys_slist_append(&tx_buffer_wait_list, &ctx->wait_node);
			ctx->wait_reason = WAITING_FOR_TX_BUFFER;
		}

		/* Now check to see if this procedure context is @ head of the wait list */
		if (ctx->wait_reason == WAITING_FOR_TX_BUFFER &&
		    sys_slist_peek_head(&tx_buffer_wait_list) == &ctx->wait_node) {
			return (common_tx_buffer_alloc <
				CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM);
		}

		return false;
	}
	return true;
}

/*
 * @brief un-peek of a tx buffer, in case ongoing alloc is aborted
 *
 */
void llcp_tx_alloc_unpeek(struct proc_ctx *ctx)
{
	sys_slist_find_and_remove(&tx_buffer_wait_list, &ctx->wait_node);
	ctx->wait_reason = WAITING_FOR_NOTHING;
}

/*
 * @brief complete alloc of a tx buffer, must preceded by successful call to
 * llcp_tx_alloc_peek()
 *
 * @return node_tx* that was peek'ed by llcp_tx_alloc_peek()
 *
 */
struct node_tx *llcp_tx_alloc(struct ll_conn *conn, struct proc_ctx *ctx)
{
	conn->llcp.tx_buffer_alloc++;
#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	if (conn->llcp.tx_buffer_alloc > CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM) {
		common_tx_buffer_alloc++;
		/* global buffer allocated, so we're at the head and should just pop head */
		sys_slist_get(&tx_buffer_wait_list);
	} else {
		/* we're allocating conn_tx_buffer, so remove from wait list if waiting */
		if (ctx->wait_reason == WAITING_FOR_TX_BUFFER) {
			sys_slist_find_and_remove(&tx_buffer_wait_list, &ctx->wait_node);
		}
	}
#else /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0 */
	/* global buffer allocated, so remove head of wait list */
	common_tx_buffer_alloc++;
	sys_slist_get(&tx_buffer_wait_list);
#endif /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0 */
	ctx->wait_reason = WAITING_FOR_NOTHING;

	return (struct node_tx *)mem_acquire(&mem_tx.free);
}
#else /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
bool llcp_tx_alloc_peek(struct ll_conn *conn, struct proc_ctx *ctx)
{
	ARG_UNUSED(conn);
	return mem_tx.free != NULL;
}

void llcp_tx_alloc_unpeek(struct proc_ctx *ctx)
{
	/* Empty on purpose, as unpeek is not needed when no buffer queueing is used */
	ARG_UNUSED(ctx);
}

struct node_tx *llcp_tx_alloc(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct pdu_data *pdu;
	struct node_tx *tx;

	ARG_UNUSED(conn);
	tx = (struct node_tx *)mem_acquire(&mem_tx.free);

	pdu = (struct pdu_data *)tx->pdu;
	ull_pdu_data_init(pdu);

	return tx;
}
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

static void tx_release(struct node_tx *tx)
{
	mem_release(tx, &mem_tx.free);
}

bool llcp_ntf_alloc_is_available(void)
{
	return ll_pdu_rx_alloc_peek(1) != NULL;
}

bool llcp_ntf_alloc_num_available(uint8_t count)
{
	return ll_pdu_rx_alloc_peek(count) != NULL;
}

struct node_rx_pdu *llcp_ntf_alloc(void)
{
	return ll_pdu_rx_alloc();
}

/*
 * ULL -> LLL Interface
 */

void llcp_tx_enqueue(struct ll_conn *conn, struct node_tx *tx)
{
	ull_tx_q_enqueue_ctrl(&conn->tx_q, tx);
}

void llcp_tx_pause_data(struct ll_conn *conn, enum llcp_tx_q_pause_data_mask pause_mask)
{
	/* Only pause the TX Q if we have not already paused it (by any procedure) */
	if (conn->llcp.tx_q_pause_data_mask == 0) {
		ull_tx_q_pause_data(&conn->tx_q);
	}

	/* Add the procedure that paused data */
	conn->llcp.tx_q_pause_data_mask |= pause_mask;
}

void llcp_tx_resume_data(struct ll_conn *conn, enum llcp_tx_q_pause_data_mask resume_mask)
{
	/* Remove the procedure that paused data */
	conn->llcp.tx_q_pause_data_mask &= ~resume_mask;

	/* Only resume the TX Q if we have removed all procedures that paused data */
	if (conn->llcp.tx_q_pause_data_mask == 0) {
		ull_tx_q_resume_data(&conn->tx_q);
	}
}

void llcp_rx_node_retain(struct proc_ctx *ctx)
{
	LL_ASSERT(ctx->node_ref.rx);

	/* Only retain if not already retained */
	if (ctx->node_ref.rx->hdr.type != NODE_RX_TYPE_RETAIN) {
		/* Mark RX node to NOT release */
		ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RETAIN;

		/* store link element reference to use once this node is moved up */
		ctx->node_ref.rx->hdr.link = ctx->node_ref.link;
	}
}

void llcp_rx_node_release(struct proc_ctx *ctx)
{
	LL_ASSERT(ctx->node_ref.rx);

	/* Only release if retained */
	if (ctx->node_ref.rx->hdr.type == NODE_RX_TYPE_RETAIN) {
		/* Mark RX node to release and release */
		ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RELEASE;
		ll_rx_put_sched(ctx->node_ref.rx->hdr.link, ctx->node_ref.rx);
	}
}

void llcp_nodes_release(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (ctx->node_ref.rx && ctx->node_ref.rx->hdr.type == NODE_RX_TYPE_RETAIN) {
		/* RX node retained, so release */
		ctx->node_ref.rx->hdr.link->mem = conn->llcp.rx_node_release;
		ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RELEASE;
		conn->llcp.rx_node_release = ctx->node_ref.rx;
	}
#if defined(CONFIG_BT_CTLR_PHY) && defined(CONFIG_BT_CTLR_DATA_LENGTH)
	if (ctx->proc == PROC_PHY_UPDATE && ctx->data.pu.ntf_dle_node) {
		/* RX node retained, so release */
		ctx->data.pu.ntf_dle_node->hdr.link->mem = conn->llcp.rx_node_release;
		ctx->data.pu.ntf_dle_node->hdr.type = NODE_RX_TYPE_RELEASE;
		conn->llcp.rx_node_release = ctx->data.pu.ntf_dle_node;
	}
#endif

	if (ctx->node_ref.tx) {
		ctx->node_ref.tx->next = conn->llcp.tx_node_release;
		conn->llcp.tx_node_release = ctx->node_ref.tx;
	}
}

/*
 * LLCP Procedure Creation
 */

static struct proc_ctx *create_procedure(enum llcp_proc proc, struct llcp_mem_pool *ctx_pool)
{
	struct proc_ctx *ctx;

	ctx = proc_ctx_acquire(ctx_pool);
	if (!ctx) {
		return NULL;
	}

	ctx->proc = proc;
	ctx->done = 0U;
	ctx->rx_greedy = 0U;
	ctx->node_ref.rx = NULL;
	ctx->node_ref.tx_ack = NULL;
	ctx->state = LLCP_STATE_IDLE;

	/* Clear procedure context data */
	memset((void *)&ctx->data, 0, sizeof(ctx->data));

	/* Initialize opcodes fields to known values */
	ctx->rx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->tx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->response_opcode = ULL_LLCP_INVALID_OPCODE;

	return ctx;
}

struct proc_ctx *llcp_create_local_procedure(enum llcp_proc proc)
{
	return create_procedure(proc, &mem_local_ctx);
}

struct proc_ctx *llcp_create_remote_procedure(enum llcp_proc proc)
{
	return create_procedure(proc, &mem_remote_ctx);
}

/*
 * LLCP Public API
 */

void ull_cp_init(void)
{
	mem_init(mem_local_ctx.pool, PROC_CTX_BUF_SIZE,
		 CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM,
		 &mem_local_ctx.free);
	mem_init(mem_remote_ctx.pool, PROC_CTX_BUF_SIZE,
		 CONFIG_BT_CTLR_LLCP_REMOTE_PROC_CTX_BUF_NUM,
		 &mem_remote_ctx.free);
	mem_init(mem_tx.pool, TX_CTRL_BUF_SIZE, LLCP_TX_CTRL_BUF_COUNT, &mem_tx.free);

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	/* Reset buffer alloc management */
	sys_slist_init(&tx_buffer_wait_list);
	common_tx_buffer_alloc = 0;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
}

void ull_llcp_init(struct ll_conn *conn)
{
	/* Reset local request fsm */
	llcp_lr_init(conn);
	sys_slist_init(&conn->llcp.local.pend_proc_list);
	conn->llcp.local.pause = 0U;

	/* Reset remote request fsm */
	llcp_rr_init(conn);
	sys_slist_init(&conn->llcp.remote.pend_proc_list);
	conn->llcp.remote.pause = 0U;
	conn->llcp.remote.incompat = INCOMPAT_NO_COLLISION;
	conn->llcp.remote.collision = 0U;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	conn->llcp.remote.paused_cmd = PROC_NONE;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

	/* Reset the Procedure Response Timeout to be disabled,
	 * 'ull_cp_prt_reload_set' must be called to setup this value.
	 */
	conn->llcp.prt_reload = 0U;

	/* Reset the cached version Information (PROC_VERSION_EXCHANGE) */
	memset(&conn->llcp.vex, 0, sizeof(conn->llcp.vex));

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	/* Reset the cached min used channels information (PROC_MIN_USED_CHANS) */
	memset(&conn->llcp.muc, 0, sizeof(conn->llcp.muc));
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

	/* Reset the feature exchange fields */
	memset(&conn->llcp.fex, 0, sizeof(conn->llcp.fex));
	conn->llcp.fex.features_used = ll_feat_get();

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Reset encryption related state */
	conn->lll.enc_tx = 0U;
	conn->lll.enc_rx = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	conn->llcp.cte_req.is_enabled = 0U;
	conn->llcp.cte_req.req_expire = 0U;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	conn->llcp.cte_rsp.is_enabled = 0U;
	conn->llcp.cte_rsp.is_active = 0U;
	conn->llcp.cte_rsp.disable_param = NULL;
	conn->llcp.cte_rsp.disable_cb = NULL;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	conn->llcp.tx_buffer_alloc = 0;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

	conn->llcp.tx_q_pause_data_mask = 0;
	conn->lll.event_counter = 0;

	conn->llcp.tx_node_release = NULL;
	conn->llcp.rx_node_release = NULL;
}

void ull_cp_release_tx(struct ll_conn *conn, struct node_tx *tx)
{
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	if (conn) {
		LL_ASSERT(conn->llcp.tx_buffer_alloc > 0);
		if (conn->llcp.tx_buffer_alloc > CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM) {
			common_tx_buffer_alloc--;
		}
		conn->llcp.tx_buffer_alloc--;
	}
#else /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	ARG_UNUSED(conn);
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	tx_release(tx);
}

static int prt_elapse(uint16_t *expire, uint16_t elapsed_event)
{
	if (*expire != 0U) {
		if (*expire > elapsed_event) {
			*expire -= elapsed_event;
		} else {
			/* Timer expired */
			return -ETIMEDOUT;
		}
	}

	/* Timer still running */
	return 0;
}

int ull_cp_prt_elapse(struct ll_conn *conn, uint16_t elapsed_event, uint8_t *error_code)
{
	int loc_ret;
	int rem_ret;

	loc_ret = prt_elapse(&conn->llcp.local.prt_expire, elapsed_event);
	if (loc_ret == -ETIMEDOUT) {
		/* Local Request Machine timed out */

		struct proc_ctx *ctx;

		ctx = llcp_lr_peek(conn);
		LL_ASSERT(ctx);

		if (ctx->proc == PROC_TERMINATE) {
			/* Active procedure is ACL Termination */
			*error_code = ctx->data.term.error_code;
		} else {
			*error_code = BT_HCI_ERR_LL_RESP_TIMEOUT;
		}

		return -ETIMEDOUT;
	}

	rem_ret = prt_elapse(&conn->llcp.remote.prt_expire, elapsed_event);
	if (rem_ret == -ETIMEDOUT) {
		/* Remote Request Machine timed out */

		*error_code = BT_HCI_ERR_LL_RESP_TIMEOUT;
		return -ETIMEDOUT;
	}

	/* Both timers are still running */
	*error_code = BT_HCI_ERR_SUCCESS;
	return 0;
}

void ull_cp_prt_reload_set(struct ll_conn *conn, uint32_t conn_intv_us)
{
	/* Convert 40s Procedure Response Timeout into events */
	conn->llcp.prt_reload = RADIO_CONN_EVENTS((40U * 1000U * 1000U), conn_intv_us);
}

void ull_cp_run(struct ll_conn *conn)
{
	llcp_rr_run(conn);
	llcp_lr_run(conn);
}

void ull_cp_state_set(struct ll_conn *conn, uint8_t state)
{
	switch (state) {
	case ULL_CP_CONNECTED:
		llcp_rr_connect(conn);
		llcp_lr_connect(conn);
		break;
	case ULL_CP_DISCONNECTED:
		llcp_rr_disconnect(conn);
		llcp_lr_disconnect(conn);
		break;
	default:
		break;
	}
}

void ull_cp_release_nodes(struct ll_conn *conn)
{
	struct node_rx_pdu *rx;
	struct node_tx *tx;

	/* release any llcp retained rx nodes */
	rx = conn->llcp.rx_node_release;
	while (rx) {
		struct node_rx_hdr *hdr;

		/* traverse to next rx node */
		hdr = &rx->hdr;
		rx = hdr->link->mem;

		/* enqueue rx node towards Thread */
		ll_rx_put(hdr->link, hdr);
	}
	conn->llcp.rx_node_release = NULL;

	/* release any llcp pre-allocated tx nodes */
	tx = conn->llcp.tx_node_release;
	while (tx) {
		struct node_tx *tx_release;

		tx_release = tx;
		tx = tx->next;

		ull_cp_release_tx(conn, tx_release);
	}
	conn->llcp.tx_node_release = NULL;
}

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
uint8_t ull_cp_min_used_chans(struct ll_conn *conn, uint8_t phys, uint8_t min_used_chans)
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_PERIPHERAL) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = llcp_create_local_procedure(PROC_MIN_USED_CHANS);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.muc.phys = phys;
	ctx->data.muc.min_used_chans = min_used_chans;

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

#if defined(CONFIG_BT_CTLR_LE_PING)
uint8_t ull_cp_le_ping(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_create_local_procedure(PROC_LE_PING);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
uint8_t ull_cp_feature_exchange(struct ll_conn *conn, uint8_t host_initiated)
{
	struct proc_ctx *ctx;

	ctx = llcp_create_local_procedure(PROC_FEATURE_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.fex.host_initiated = host_initiated;

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */

uint8_t ull_cp_version_exchange(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
#if defined(CONFIG_BT_CENTRAL)
uint8_t ull_cp_encryption_start(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2],
				const uint8_t ltk[16])
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_CENTRAL) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = llcp_create_local_procedure(PROC_ENCRYPTION_START);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Copy input parameters */
	memcpy(ctx->data.enc.rand, rand, sizeof(ctx->data.enc.rand));
	ctx->data.enc.ediv[0] = ediv[0];
	ctx->data.enc.ediv[1] = ediv[1];
	memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));

	/* Enqueue request */
	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_encryption_pause(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2],
				const uint8_t ltk[16])
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_CENTRAL) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = llcp_create_local_procedure(PROC_ENCRYPTION_PAUSE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Copy input parameters */
	memcpy(ctx->data.enc.rand, rand, sizeof(ctx->data.enc.rand));
	ctx->data.enc.ediv[0] = ediv[0];
	ctx->data.enc.ediv[1] = ediv[1];
	memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));

	/* Enqueue request */
	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CENTRAL */

uint8_t ull_cp_encryption_paused(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_ENCRYPTION_PAUSE) {
		return 1;
	}

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_ENCRYPTION_PAUSE) {
		return 1;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PHY)
uint8_t ull_cp_phy_update(struct ll_conn *conn, uint8_t tx, uint8_t flags, uint8_t rx,
			  uint8_t host_initiated)
{
	struct proc_ctx *ctx;

	ctx = llcp_create_local_procedure(PROC_PHY_UPDATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.pu.tx = tx;
	ctx->data.pu.flags = flags;
	ctx->data.pu.rx = rx;
	ctx->data.pu.host_initiated = host_initiated;

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_PHY */

uint8_t ull_cp_terminate(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

	llcp_lr_terminate(conn);
	llcp_rr_terminate(conn);

	ctx = llcp_create_local_procedure(PROC_TERMINATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.term.error_code = error_code;

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
uint8_t ull_cp_cis_terminate(struct ll_conn *conn,
			     struct ll_conn_iso_stream *cis,
			     uint8_t error_code)
{
	struct proc_ctx *ctx;

	if (conn->lll.handle != cis->lll.acl_handle) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = llcp_create_local_procedure(PROC_CIS_TERMINATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.cis_term.cig_id = cis->group->cig_id;
	ctx->data.cis_term.cis_id = cis->cis_id;
	ctx->data.cis_term.error_code = error_code;

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
uint8_t ull_cp_cis_create(struct ll_conn *conn, struct ll_conn_iso_stream *cis)
{
	struct ll_conn_iso_group *cig;
	struct proc_ctx *ctx;

	if (!conn->llcp.fex.valid) {
		/* No feature exchange was performed so initiate before CIS Create */
		if (ull_cp_feature_exchange(conn, 0U) != BT_HCI_ERR_SUCCESS) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	ctx = llcp_create_local_procedure(PROC_CIS_CREATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	cig = cis->group;
	ctx->data.cis_create.cis_handle = cis->lll.handle;

	ctx->data.cis_create.cig_id = cis->group->cig_id;
	ctx->data.cis_create.cis_id = cis->cis_id;
	ctx->data.cis_create.c_phy = cis->lll.tx.phy;
	ctx->data.cis_create.p_phy = cis->lll.rx.phy;
	ctx->data.cis_create.c_sdu_interval = cig->c_sdu_interval;
	ctx->data.cis_create.p_sdu_interval = cig->p_sdu_interval;
	ctx->data.cis_create.c_max_pdu = cis->lll.tx.max_pdu;
	ctx->data.cis_create.p_max_pdu = cis->lll.rx.max_pdu;
	ctx->data.cis_create.c_max_sdu = cis->c_max_sdu;
	ctx->data.cis_create.p_max_sdu = cis->p_max_sdu;
	ctx->data.cis_create.iso_interval = cig->iso_interval;
	ctx->data.cis_create.framed = cis->framed;
	ctx->data.cis_create.nse = cis->lll.nse;
	ctx->data.cis_create.sub_interval = cis->lll.sub_interval;
	ctx->data.cis_create.c_bn = cis->lll.tx.bn;
	ctx->data.cis_create.p_bn = cis->lll.rx.bn;
	ctx->data.cis_create.c_ft = cis->lll.tx.ft;
	ctx->data.cis_create.p_ft = cis->lll.rx.ft;

	/* ctx->data.cis_create.conn_event_count will be filled when Tx PDU is
	 * enqueued.
	 */

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* defined(CONFIG_BT_CTLR_CENTRAL_ISO) */

#if defined(CONFIG_BT_CENTRAL)
uint8_t ull_cp_chan_map_update(struct ll_conn *conn, const uint8_t chm[5])
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_CENTRAL) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = llcp_create_local_procedure(PROC_CHAN_MAP_UPDATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	memcpy(ctx->data.chmu.chm, chm, sizeof(ctx->data.chmu.chm));

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CENTRAL */

const uint8_t *ull_cp_chan_map_update_pending(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	if (conn->lll.role == BT_HCI_ROLE_CENTRAL) {
		ctx = llcp_lr_peek(conn);
	} else {
		ctx = llcp_rr_peek(conn);
	}

	if (ctx && ctx->proc == PROC_CHAN_MAP_UPDATE) {
		return ctx->data.chmu.chm;
	}

	return NULL;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint8_t ull_cp_data_length_update(struct ll_conn *conn, uint16_t max_tx_octets,
				  uint16_t max_tx_time)
{
	struct proc_ctx *ctx;

	if (!feature_dle(conn)) {
		/* Data Length Update procedure not supported */

		/* Returning BT_HCI_ERR_SUCCESS here might seem counter-intuitive,
		 * but nothing in the specification seems to suggests
		 * BT_HCI_ERR_UNSUPP_REMOTE_FEATURE.
		 */
		return BT_HCI_ERR_SUCCESS;
	}

	ctx = llcp_create_local_procedure(PROC_DATA_LENGTH_UPDATE);

	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Apply update to local */
	ull_dle_local_tx_update(conn, max_tx_octets, max_tx_time);

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
uint8_t ull_cp_req_peer_sca(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	if (!feature_sca(conn)) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	ctx = llcp_create_local_procedure(PROC_SCA_UPDATE);

	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_LE_ENC)
uint8_t ull_cp_ltk_req_reply(struct ll_conn *conn, const uint8_t ltk[16])
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE) &&
	    llcp_rp_enc_ltk_req_reply_allowed(conn, ctx)) {
		memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));
		llcp_rp_enc_ltk_req_reply(conn, ctx);
		return BT_HCI_ERR_SUCCESS;
	}
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ull_cp_ltk_req_neq_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE) &&
	    llcp_rp_enc_ltk_req_reply_allowed(conn, ctx)) {
		llcp_rp_enc_ltk_req_neg_reply(conn, ctx);
		return BT_HCI_ERR_SUCCESS;
	}
	return BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

uint8_t ull_cp_conn_update(struct ll_conn *conn, uint16_t interval_min, uint16_t interval_max,
			   uint16_t latency, uint16_t timeout, uint16_t *offsets)
{
	struct proc_ctx *ctx;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (feature_conn_param_req(conn)) {
		ctx = llcp_create_local_procedure(PROC_CONN_PARAM_REQ);
	} else if (conn->lll.role == BT_HCI_ROLE_CENTRAL) {
		ctx = llcp_create_local_procedure(PROC_CONN_UPDATE);
	} else {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}
#else /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */
	if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	ctx = llcp_create_local_procedure(PROC_CONN_UPDATE);
#endif /* !CONFIG_BT_CTLR_CONN_PARAM_REQ */

	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Store arguments in corresponding procedure context */
	if (ctx->proc == PROC_CONN_UPDATE) {
		ctx->data.cu.interval_max = interval_max;
		ctx->data.cu.latency = latency;
		ctx->data.cu.timeout = timeout;
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	} else if (ctx->proc == PROC_CONN_PARAM_REQ) {
		ctx->data.cu.interval_min = interval_min;
		ctx->data.cu.interval_max = interval_max;
		ctx->data.cu.latency = latency;
		ctx->data.cu.timeout = timeout;
		ctx->data.cu.offsets[0] = offsets ? offsets[0] : 0x0000;
		ctx->data.cu.offsets[1] = offsets ? offsets[1] : 0xffff;
		ctx->data.cu.offsets[2] = offsets ? offsets[2] : 0xffff;
		ctx->data.cu.offsets[3] = offsets ? offsets[3] : 0xffff;
		ctx->data.cu.offsets[4] = offsets ? offsets[4] : 0xffff;
		ctx->data.cu.offsets[5] = offsets ? offsets[5] : 0xffff;

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    (conn->lll.role == BT_HCI_ROLE_PERIPHERAL)) {
			uint16_t handle = ll_conn_handle_get(conn);

			ull_periph_latency_cancel(conn, handle);
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	} else {
		LL_ASSERT(0); /* Unknown procedure */
	}

	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint8_t ull_cp_remote_dle_pending(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);

	return (ctx && ctx->proc == PROC_DATA_LENGTH_UPDATE);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void ull_cp_conn_param_req_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		llcp_rp_conn_param_req_reply(conn, ctx);
	}
}

void ull_cp_conn_param_req_neg_reply(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		ctx->data.cu.error = error_code;
		llcp_rp_conn_param_req_neg_reply(conn, ctx);
	}
}

uint8_t ull_cp_remote_cpr_pending(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);

	return (ctx && ctx->proc == PROC_CONN_PARAM_REQ);
}

#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
bool ull_cp_remote_cpr_apm_awaiting_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);

	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		return llcp_rp_conn_param_req_apm_awaiting_reply(ctx);
	}

	return false;
}

void ull_cp_remote_cpr_apm_reply(struct ll_conn *conn, uint16_t *offsets)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);

	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		ctx->data.cu.offsets[0] = offsets[0];
		ctx->data.cu.offsets[1] = offsets[1];
		ctx->data.cu.offsets[2] = offsets[2];
		ctx->data.cu.offsets[3] = offsets[3];
		ctx->data.cu.offsets[4] = offsets[4];
		ctx->data.cu.offsets[5] = offsets[5];
		ctx->data.cu.error = 0U;
		llcp_rp_conn_param_req_apm_reply(conn, ctx);
	}
}

void ull_cp_remote_cpr_apm_neg_reply(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);

	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		ctx->data.cu.error = error_code;
		llcp_rp_conn_param_req_apm_reply(conn, ctx);
	}
}
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
void ull_cp_cte_rsp_enable(struct ll_conn *conn, bool enable, uint8_t max_cte_len,
			   uint8_t cte_types)
{
	conn->llcp.cte_rsp.is_enabled = enable;

	if (enable) {
		conn->llcp.cte_rsp.max_cte_len = max_cte_len;
		conn->llcp.cte_rsp.cte_types = cte_types;
	}
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
uint8_t ull_cp_cte_req(struct ll_conn *conn, uint8_t min_cte_len, uint8_t cte_type)
{
	struct proc_ctx *ctx;

	/* If Controller gained, awareness:
	 * - by Feature Exchange control procedure that peer device does not support CTE response,
	 * - by reception LL_UNKNOWN_RSP with unknown type LL_CTE_REQ that peer device does not
	 *   recognize CTE request,
	 * then response to Host that CTE request enable command is not possible due to unsupported
	 * remote feature.
	 */
	if ((conn->llcp.fex.valid &&
	     (!(conn->llcp.fex.features_peer & BIT64(BT_LE_FEAT_BIT_CONN_CTE_RESP)))) ||
	    (!conn->llcp.fex.valid && !feature_cte_req(conn))) {
		return BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
	}

	/* The request may be started by periodic CTE request procedure, so it skips earlier
	 * verification of PHY. In case the PHY has changed to CODE the request should be stopped.
	 */
#if defined(CONFIG_BT_CTLR_PHY)
	if (conn->lll.phy_rx != PHY_CODED) {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PHY */
		ctx = llcp_create_local_procedure(PROC_CTE_REQ);
		if (!ctx) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		ctx->data.cte_req.min_len = min_cte_len;
		ctx->data.cte_req.type = cte_type;

		llcp_lr_enqueue(conn, ctx);

		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}

void ull_cp_cte_req_set_disable(struct ll_conn *conn)
{
	conn->llcp.cte_req.is_enabled = 0U;
	conn->llcp.cte_req.req_interval = 0U;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

void ull_cp_cc_offset_calc_reply(struct ll_conn *conn, uint32_t cis_offset_min,
				 uint32_t cis_offset_max)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		ctx->data.cis_create.cis_offset_min = cis_offset_min;
		ctx->data.cis_create.cis_offset_max = cis_offset_max;

		llcp_lp_cc_offset_calc_reply(conn, ctx);
	}
}

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
bool ull_cp_cc_awaiting_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return llcp_rp_cc_awaiting_reply(ctx);
	}

	return false;
}

uint16_t ull_cp_cc_ongoing_handle(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return ctx->data.cis_create.cis_handle;
	}

	return 0xFFFF;
}

void ull_cp_cc_accept(struct ll_conn *conn, uint32_t cis_offset_min)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		if (cis_offset_min > ctx->data.cis_create.cis_offset_min) {
			if (cis_offset_min > ctx->data.cis_create.cis_offset_max) {
				ctx->data.cis_create.error = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
				llcp_rp_cc_reject(conn, ctx);

				return;
			}

			ctx->data.cis_create.cis_offset_min = cis_offset_min;
		}

		llcp_rp_cc_accept(conn, ctx);
	}
}

void ull_cp_cc_reject(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		ctx->data.cis_create.error = error_code;
		llcp_rp_cc_reject(conn, ctx);
	}
}
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) || defined(CONFIG_BT_CTLR_CENTRAL_ISO)
bool ull_cp_cc_awaiting_established(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return llcp_rp_cc_awaiting_established(ctx);
	}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return llcp_lp_cc_awaiting_established(ctx);
	}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
	return false;
}

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
bool ull_cp_cc_cancel(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return llcp_lp_cc_cancel(conn, ctx);
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

void ull_cp_cc_established(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		ctx->data.cis_create.error = error_code;
		llcp_rp_cc_established(conn, ctx);
		llcp_rr_check_done(conn, ctx);
	}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		ctx->data.cis_create.error = error_code;
		llcp_lp_cc_established(conn, ctx);
		llcp_lr_check_done(conn, ctx);
	}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO || CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_CENTRAL_ISO)
bool ull_lp_cc_is_active(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->proc == PROC_CIS_CREATE) {
		return llcp_lp_cc_is_active(ctx);
	}
	return false;
}

bool ull_lp_cc_is_enqueued(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek_proc(conn, PROC_CIS_CREATE);

	return (ctx != NULL);
}
#endif /* defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_CENTRAL_ISO) */

static bool pdu_is_expected(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return (ctx->rx_opcode == pdu->llctrl.opcode || ctx->rx_greedy);
}

static bool pdu_is_unknown(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) &&
		(ctx->tx_opcode == pdu->llctrl.unknown_rsp.type));
}

static bool pdu_is_reject(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	/* For LL_REJECT_IND there is no simple way of confirming protocol validity of the PDU
	 * for the given procedure, so simply pass it on and let procedure engine deal with it
	 */
	return (pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_IND);
}

static bool pdu_is_reject_ext(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND) &&
		(ctx->tx_opcode == pdu->llctrl.reject_ext_ind.reject_opcode));
}

static bool pdu_is_any_reject(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return (pdu_is_reject_ext(pdu, ctx) || pdu_is_reject(pdu, ctx));
}

static bool pdu_is_terminate(struct pdu_data *pdu)
{
	return pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
}

#define VALIDATE_PDU_LEN(pdu, type) (pdu->len == PDU_DATA_LLCTRL_LEN(type))

#if defined(CONFIG_BT_PERIPHERAL)
static bool pdu_validate_conn_update_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, conn_update_ind);
}

static bool pdu_validate_chan_map_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, chan_map_ind);
}
#endif /* CONFIG_BT_PERIPHERAL */

static bool pdu_validate_terminate_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, terminate_ind);
}

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
static bool pdu_validate_enc_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, enc_req);
}
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
static bool pdu_validate_enc_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, enc_rsp);
}

static bool pdu_validate_start_enc_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, start_enc_req);
}
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
static bool pdu_validate_start_enc_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, start_enc_rsp);
}
#endif

static bool pdu_validate_unknown_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, unknown_rsp);
}

#if defined(CONFIG_BT_PERIPHERAL)
static bool pdu_validate_feature_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, feature_req);
}
#endif

#if defined(CONFIG_BT_CENTRAL)
static bool pdu_validate_feature_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, feature_rsp);
}
#endif

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
static bool pdu_validate_pause_enc_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, pause_enc_req);
}
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
static bool pdu_validate_pause_enc_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, pause_enc_rsp);
}
#endif

static bool pdu_validate_version_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, version_ind);
}

static bool pdu_validate_reject_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, reject_ind);
}

#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_CENTRAL)
static bool pdu_validate_per_init_feat_xchg(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, per_init_feat_xchg);
}
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static bool pdu_validate_conn_param_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, conn_param_req);
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static bool pdu_validate_conn_param_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, conn_param_rsp);
}

static bool pdu_validate_reject_ext_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, reject_ext_ind);
}

#if defined(CONFIG_BT_CTLR_LE_PING)
static bool pdu_validate_ping_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, ping_req);
}
#endif /* CONFIG_BT_CTLR_LE_PING */

static bool pdu_validate_ping_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, ping_rsp);
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static bool pdu_validate_length_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, length_req);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static bool pdu_validate_length_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, length_rsp);
}

#if defined(CONFIG_BT_CTLR_PHY)
static bool pdu_validate_phy_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, phy_req);
}
#endif /* CONFIG_BT_CTLR_PHY */

static bool pdu_validate_phy_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, phy_rsp);
}

static bool pdu_validate_phy_upd_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, phy_upd_ind);
}

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_CENTRAL)
static bool pdu_validate_min_used_chan_ind(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, min_used_chans_ind);
}
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
static bool pdu_validate_cte_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, cte_req);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
static bool pdu_validate_cte_resp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, cte_rsp);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
static bool pdu_validate_clock_accuracy_req(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, clock_accuracy_req);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static bool pdu_validate_clock_accuracy_rsp(struct pdu_data *pdu)
{
	return VALIDATE_PDU_LEN(pdu, clock_accuracy_rsp);
}

typedef bool (*pdu_param_validate_t)(struct pdu_data *pdu);

struct pdu_validate {
	/* TODO can be just size if no other sanity checks here */
	pdu_param_validate_t validate_cb;
};

static const struct pdu_validate pdu_validate[] = {
#if defined(CONFIG_BT_PERIPHERAL)
	[PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND] = { pdu_validate_conn_update_ind },
	[PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND] = { pdu_validate_chan_map_ind },
#endif /* CONFIG_BT_PERIPHERAL */
	[PDU_DATA_LLCTRL_TYPE_TERMINATE_IND] = { pdu_validate_terminate_ind },
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
	[PDU_DATA_LLCTRL_TYPE_ENC_REQ] = { pdu_validate_enc_req },
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	[PDU_DATA_LLCTRL_TYPE_ENC_RSP] = { pdu_validate_enc_rsp },
	[PDU_DATA_LLCTRL_TYPE_START_ENC_REQ] = { pdu_validate_start_enc_req },
#endif
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
	[PDU_DATA_LLCTRL_TYPE_START_ENC_RSP] = { pdu_validate_start_enc_rsp },
#endif
	[PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP] = { pdu_validate_unknown_rsp },
#if defined(CONFIG_BT_PERIPHERAL)
	[PDU_DATA_LLCTRL_TYPE_FEATURE_REQ] = { pdu_validate_feature_req },
#endif
#if defined(CONFIG_BT_CENTRAL)
	[PDU_DATA_LLCTRL_TYPE_FEATURE_RSP] = { pdu_validate_feature_rsp },
#endif
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
	[PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ] = { pdu_validate_pause_enc_req },
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	[PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP] = { pdu_validate_pause_enc_rsp },
#endif
	[PDU_DATA_LLCTRL_TYPE_VERSION_IND] = { pdu_validate_version_ind },
	[PDU_DATA_LLCTRL_TYPE_REJECT_IND] = { pdu_validate_reject_ind },
#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_CENTRAL)
	[PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG] = { pdu_validate_per_init_feat_xchg },
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	[PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ] = { pdu_validate_conn_param_req },
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	[PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP] = { pdu_validate_conn_param_rsp },
	[PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND] = { pdu_validate_reject_ext_ind },
#if defined(CONFIG_BT_CTLR_LE_PING)
	[PDU_DATA_LLCTRL_TYPE_PING_REQ] = { pdu_validate_ping_req },
#endif /* CONFIG_BT_CTLR_LE_PING */
	[PDU_DATA_LLCTRL_TYPE_PING_RSP] = { pdu_validate_ping_rsp },
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	[PDU_DATA_LLCTRL_TYPE_LENGTH_REQ] = { pdu_validate_length_req },
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	[PDU_DATA_LLCTRL_TYPE_LENGTH_RSP] = { pdu_validate_length_rsp },
#if defined(CONFIG_BT_CTLR_PHY)
	[PDU_DATA_LLCTRL_TYPE_PHY_REQ] = { pdu_validate_phy_req },
#endif /* CONFIG_BT_CTLR_PHY */
	[PDU_DATA_LLCTRL_TYPE_PHY_RSP] = { pdu_validate_phy_rsp },
	[PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND] = { pdu_validate_phy_upd_ind },
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_CENTRAL)
	[PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND] = { pdu_validate_min_used_chan_ind },
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	[PDU_DATA_LLCTRL_TYPE_CTE_REQ] = { pdu_validate_cte_req },
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	[PDU_DATA_LLCTRL_TYPE_CTE_RSP] = { pdu_validate_cte_resp },
#endif /* PDU_DATA_LLCTRL_TYPE_CTE_RSP */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	[PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ] = { pdu_validate_clock_accuracy_req },
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	[PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP] = { pdu_validate_clock_accuracy_rsp },
};

static bool pdu_is_valid(struct pdu_data *pdu)
{
	/* the should be at least 1 byte of data with opcode*/
	if (pdu->len < 1) {
		/* fake opcode */
		pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		return false;
	}

	if (pdu->llctrl.opcode < ARRAY_SIZE(pdu_validate)) {
		pdu_param_validate_t cb;

		cb = pdu_validate[pdu->llctrl.opcode].validate_cb;
		if (cb) {
			return cb(pdu);
		}
	}

	/* consider unsupported and unknown PDUs as valid */
	return true;
}

void ull_cp_tx_ack(struct ll_conn *conn, struct node_tx *tx)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->node_ref.tx_ack == tx) {
		/* TX ack re. local request */
		llcp_lr_tx_ack(conn, ctx, tx);
	}

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->node_ref.tx_ack == tx) {
		/* TX ack re. remote response */
		llcp_rr_tx_ack(conn, ctx, tx);
	}
}

void ull_cp_tx_ntf(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx) {
		/* TX notifications towards Host */
		llcp_lr_tx_ntf(conn, ctx);
	}

	ctx = llcp_rr_peek(conn);
	if (ctx) {
		/* TX notifications towards Host */
		llcp_rr_tx_ntf(conn, ctx);
	}
}

void ull_cp_rx(struct ll_conn *conn, memq_link_t *link, struct node_rx_pdu *rx)
{
	struct proc_ctx *ctx_l;
	struct proc_ctx *ctx_r;
	struct pdu_data *pdu;
	bool unexpected_l;
	bool unexpected_r;
	bool pdu_valid;

	pdu = (struct pdu_data *)rx->pdu;

	pdu_valid = pdu_is_valid(pdu);

	if (!pdu_valid) {
		struct proc_ctx *ctx;

		ctx = llcp_lr_peek(conn);
		if (ctx && pdu_is_expected(pdu, ctx)) {
			return;
		}

		ctx = llcp_rr_peek(conn);
		if (ctx && pdu_is_expected(pdu, ctx)) {
			return;
		}

		/*  Process invalid PDU's as new procedure */
		ctx_l = NULL;
		ctx_r = NULL;
	} else if (pdu_is_terminate(pdu)) {
		/*  Process LL_TERMINATE_IND PDU's as new procedure */
		ctx_l = NULL;
		ctx_r = NULL;
	} else {
		/* Query local and remote activity */
		ctx_l = llcp_lr_peek(conn);
		ctx_r = llcp_rr_peek(conn);
	}

	if (ctx_l) {
		/* Local active procedure */

		if (ctx_r) {
			/* Local active procedure
			 * Remote active procedure
			 */
			unexpected_l = !(pdu_is_expected(pdu, ctx_l) ||
					 pdu_is_unknown(pdu, ctx_l) ||
					 pdu_is_any_reject(pdu, ctx_l));

			unexpected_r = !(pdu_is_expected(pdu, ctx_r) ||
					 pdu_is_unknown(pdu, ctx_r) ||
					 pdu_is_reject_ext(pdu, ctx_r));

			if (unexpected_l == unexpected_r) {
				/* Both Local and Remote procedure active
				 * and PDU is either
				 * unexpected by both
				 * or
				 * expected by both
				 *
				 * Both situations is a result of invalid behaviour
				 */
				conn->llcp_terminate.reason_final =
					unexpected_r ? BT_HCI_ERR_LMP_PDU_NOT_ALLOWED :
						       BT_HCI_ERR_UNSPECIFIED;
			} else if (unexpected_l) {
				/* Local active procedure
				 * Unexpected local procedure PDU
				 * Remote active procedure
				 * Expected remote procedure PDU
				 */

				/* Process PDU in remote procedure */
				llcp_rr_rx(conn, ctx_r, link, rx);
			} else if (unexpected_r) {
				/* Local active procedure
				 * Expected local procedure PDU
				 * Remote active procedure
				 * Unexpected remote procedure PDU
				 */

				/* Process PDU in local procedure */
				llcp_lr_rx(conn, ctx_l, link, rx);
			}
			/* no else clause as this cannot occur with the logic above:
			 * if they are not identical then one must be true
			 */
		} else {
			/* Local active procedure
			 * No remote active procedure
			 */

			unexpected_l = !(pdu_is_expected(pdu, ctx_l) ||
					 pdu_is_unknown(pdu, ctx_l) ||
					 pdu_is_any_reject(pdu, ctx_l));

			if (unexpected_l) {
				/* Local active procedure
				 * Unexpected local procedure PDU
				 * No remote active procedure
				 */

				/* Process PDU as a new remote request */
				LL_ASSERT(pdu_valid);
				llcp_rr_new(conn, link, rx, true);
			} else {
				/* Local active procedure
				 * Expected local procedure PDU
				 * No remote active procedure
				 */

				/* Process PDU in local procedure */
				llcp_lr_rx(conn, ctx_l, link, rx);
			}
		}
	} else if (ctx_r) {
		/* No local active procedure
		 * Remote active procedure
		 */

		/* Process PDU in remote procedure */
		llcp_rr_rx(conn, ctx_r, link, rx);
	} else {
		/* No local active procedure
		 * No remote active procedure
		 */

		/* Process PDU as a new remote request */
		llcp_rr_new(conn, link, rx, pdu_valid);
	}
}

#ifdef ZTEST_UNITTEST

uint16_t llcp_local_ctx_buffers_free(void)
{
	return mem_free_count_get(mem_local_ctx.free);
}

uint16_t llcp_remote_ctx_buffers_free(void)
{
	return mem_free_count_get(mem_remote_ctx.free);
}

uint16_t llcp_ctx_buffers_free(void)
{
	return llcp_local_ctx_buffers_free() + llcp_remote_ctx_buffers_free();
}

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
uint8_t llcp_common_tx_buffer_alloc_count(void)
{
	return common_tx_buffer_alloc;
}
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

struct proc_ctx *llcp_proc_ctx_acquire(void)
{
	return proc_ctx_acquire(&mem_local_ctx);
}

struct proc_ctx *llcp_create_procedure(enum llcp_proc proc)
{
	return create_procedure(proc, &mem_local_ctx);
}
#endif
