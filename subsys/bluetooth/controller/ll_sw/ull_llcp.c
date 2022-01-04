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

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_conn_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"
#include "ull_periph_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Memory Pool Descriptor */
struct mem_pool {
	void *free;
	uint8_t *pool;
};

#define LLCTRL_PDU_SIZE (offsetof(struct pdu_data, llctrl) + sizeof(struct pdu_data_llctrl))
#define PROC_CTX_BUF_SIZE WB_UP(sizeof(struct proc_ctx))
#define TX_CTRL_BUF_SIZE WB_UP(offsetof(struct node_tx, pdu) + LLCTRL_PDU_SIZE)
#define NTF_BUF_SIZE WB_UP(offsetof(struct node_rx_pdu, pdu) + LLCTRL_PDU_SIZE)

/* LLCP Allocations */
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
sys_slist_t tx_buffer_wait_list;
static uint8_t common_tx_buffer_alloc;
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

/* TODO: Determine 'correct' number of tx nodes */
static uint8_t buffer_mem_tx[TX_CTRL_BUF_SIZE * LLCP_TX_CTRL_BUF_COUNT];
static struct mem_pool mem_tx = { .pool = buffer_mem_tx };

/* TODO: Determine 'correct' number of ctx */
static uint8_t buffer_mem_ctx[PROC_CTX_BUF_SIZE * CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM];
static struct mem_pool mem_ctx = { .pool = buffer_mem_ctx };

/*
 * LLCP Resource Management
 */
static struct proc_ctx *proc_ctx_acquire(void)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *)mem_acquire(&mem_ctx.free);
	return ctx;
}

void llcp_proc_ctx_release(struct proc_ctx *ctx)
{
	mem_release(ctx, &mem_ctx.free);
}

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
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
#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	conn->llcp.tx_buffer_alloc++;
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
	struct node_tx *tx;

	ARG_UNUSED(conn);
	tx = (struct node_tx *)mem_acquire(&mem_tx.free);
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

void llcp_tx_pause_data(struct ll_conn *conn)
{
	ull_tx_q_pause_data(&conn->tx_q);
}

void llcp_tx_resume_data(struct ll_conn *conn)
{
	ull_tx_q_resume_data(&conn->tx_q);
}

void llcp_tx_flush(struct ll_conn *conn)
{
	/* TODO(thoh): do something here to flush the TX Q */
}

/*
 * LLCP Procedure Creation
 */

static struct proc_ctx *create_procedure(enum llcp_proc proc)
{
	struct proc_ctx *ctx;

	ctx = proc_ctx_acquire();
	if (!ctx) {
		return NULL;
	}

	ctx->proc = proc;
	ctx->collision = 0U;
	ctx->pause = 0U;
	ctx->done = 0U;

	/* Clear procedure data */
	memset((void *)&ctx->data, 0, sizeof(ctx->data));

	/* Initialize opcodes fields to known values */
	ctx->rx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->tx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->response_opcode = ULL_LLCP_INVALID_OPCODE;

	return ctx;
}

struct proc_ctx *llcp_create_local_procedure(enum llcp_proc proc)
{
	struct proc_ctx *ctx;

	ctx = create_procedure(proc);
	if (!ctx) {
		return NULL;
	}

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_lp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		llcp_lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		llcp_lp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_CENTRAL)
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		llcp_lp_enc_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		llcp_lp_pu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_UPDATE:
	case PROC_CONN_PARAM_REQ:
		llcp_lp_cu_init_proc(ctx);
		break;
	case PROC_TERMINATE:
		llcp_lp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case PROC_CHAN_MAP_UPDATE:
		llcp_lp_chmu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		llcp_lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
		break;
	}

	return ctx;
}

struct proc_ctx *llcp_create_remote_procedure(enum llcp_proc proc)
{
	struct proc_ctx *ctx;

	ctx = create_procedure(proc);
	if (!ctx) {
		return NULL;
	}

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_rp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		llcp_rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		llcp_rp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_LE_ENC) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		llcp_rp_enc_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC && CONFIG_BT_PERIPHERAL */
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		llcp_rp_pu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_UPDATE:
	case PROC_CONN_PARAM_REQ:
		llcp_rp_cu_init_proc(ctx);
		break;
	case PROC_TERMINATE:
		llcp_rp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_PERIPHERAL)
	case PROC_CHAN_MAP_UPDATE:
		llcp_rp_chmu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		llcp_rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
		break;
	}

	return ctx;
}

/*
 * LLCP Public API
 */

void ull_cp_init(void)
{
	mem_init(mem_ctx.pool, PROC_CTX_BUF_SIZE, CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM,
		 &mem_ctx.free);
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

	/* Reset remote request fsm */
	llcp_rr_init(conn);
	sys_slist_init(&conn->llcp.remote.pend_proc_list);
	conn->llcp.remote.incompat = INCOMPAT_NO_COLLISION;
	conn->llcp.remote.collision = 0U;

	/* Reset the cached version Information (PROC_VERSION_EXCHANGE) */
	memset(&conn->llcp.vex, 0, sizeof(conn->llcp.vex));

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	/* Reset the cached min used channels information (PROC_MIN_USED_CHANS) */
	memset(&conn->llcp.muc, 0, sizeof(conn->llcp.muc));
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

	/* Reset the feature exchange fields */
	memset(&conn->llcp.fex, 0, sizeof(conn->llcp.fex));
	conn->llcp.fex.features_used = LL_FEAT;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Reset encryption related state */
	conn->lll.enc_tx = 0U;
	conn->lll.enc_rx = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	conn->llcp.tx_buffer_alloc = 0;
#endif /* (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0) */
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */

	conn->lll.event_counter = 0;

	llcp_lr_init(conn);
	llcp_rr_init(conn);
}

void ull_cp_release_tx(struct ll_conn *conn, struct node_tx *tx)
{
#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	if (conn->llcp.tx_buffer_alloc > CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM) {
		common_tx_buffer_alloc--;
	}
	conn->llcp.tx_buffer_alloc--;
#else /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0 */
	ARG_UNUSED(conn);
	common_tx_buffer_alloc--;
#endif /* CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0 */
#else /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	ARG_UNUSED(conn);
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	tx_release(tx);
}

void ull_cp_release_ntf(struct node_rx_pdu *ntf)
{
	ntf->hdr.next = NULL;
	ll_rx_mem_release((void **)&ntf);
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
uint8_t ull_cp_feature_exchange(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = llcp_create_local_procedure(PROC_FEATURE_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

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

	/* TODO(thoh): Proper checks for role, parameters etc. */

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

	/* TODO(thoh): Proper checks for role, parameters etc. */

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

	/* TODO(thoh): Proper checks for role, parameters etc. */

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

	llcp_lr_abort(conn);

	ctx = llcp_create_local_procedure(PROC_TERMINATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.term.error_code = error_code;

	/* TODO
	 * Termination procedure may be initiated at any time, even if other
	 * LLCP is active.
	 */
	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

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

#if defined(CONFIG_BT_CTLR_LE_ENC)
void ull_cp_ltk_req_reply(struct ll_conn *conn, const uint8_t ltk[16])
{
	/* TODO(thoh): Call rp_enc to query if LTK request reply is allowed */
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE)) {
		memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));
		llcp_rp_enc_ltk_req_reply(conn, ctx);
	}
}

void ull_cp_ltk_req_neq_reply(struct ll_conn *conn)
{
	/* TODO(thoh): Call rp_enc to query if LTK negative request reply is allowed */
	struct proc_ctx *ctx;

	ctx = llcp_rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE)) {
		llcp_rp_enc_ltk_req_neg_reply(conn, ctx);
	}
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

uint8_t ull_cp_conn_update(struct ll_conn *conn, uint16_t interval_min, uint16_t interval_max,
			   uint16_t latency, uint16_t timeout)
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

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    (conn->lll.role == BT_HCI_ROLE_PERIPHERAL)) {
			uint16_t handle = ll_conn_handle_get(conn);

			ull_periph_latency_cancel(conn, handle);
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	} else {
		LL_ASSERT(0); /* Unknown procedure */
	}

	/* TODO(tosk): Check what to handle (ADV_SCHED) from this legacy fct. */
	/* event_conn_upd_prep() (event_conn_upd_init()) */

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

	ctx = llcp_create_local_procedure(PROC_CTE_REQ);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.cte_req.min_len = min_cte_len;
	ctx->data.cte_req.type = cte_type;
	llcp_lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

static bool pdu_is_expected(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ctx->rx_opcode == pdu->llctrl.opcode;
}

static bool pdu_is_unknown(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) &&
		(ctx->tx_opcode == pdu->llctrl.unknown_rsp.type));
}

static bool pdu_is_reject(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	/* TODO(thoh): For LL_REJECT_IND check if the active procedure is supporting the PDU */
	return (((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND) &&
		 (ctx->tx_opcode == pdu->llctrl.reject_ext_ind.reject_opcode)) ||
		(pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_IND));
}

static bool pdu_is_terminate(struct pdu_data *pdu)
{
	return pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
}

void ull_cp_tx_ack(struct ll_conn *conn, struct node_tx *tx)
{
	struct proc_ctx *ctx;

	ctx = llcp_lr_peek(conn);
	if (ctx && ctx->tx_ack == tx) {
		/* TX ack re. local request */
		llcp_lr_tx_ack(conn, ctx, tx);
	}

	ctx = llcp_rr_peek(conn);
	if (ctx && ctx->tx_ack == tx) {
		/* TX ack re. remote response */
		llcp_rr_tx_ack(conn, ctx, tx);
	}
}

void ull_cp_rx(struct ll_conn *conn, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu;
	struct proc_ctx *ctx;

	pdu = (struct pdu_data *)rx->pdu;

	if (!pdu_is_terminate(pdu)) {
		/*
		 * Process non LL_TERMINATE_IND PDU's as responses to active
		 * procedures
		 */

		ctx = llcp_lr_peek(conn);
		if (ctx && (pdu_is_expected(pdu, ctx) || pdu_is_unknown(pdu, ctx) ||
			    pdu_is_reject(pdu, ctx))) {
			/* Response on local procedure */
			llcp_lr_rx(conn, ctx, rx);
			return;
		}

		ctx = llcp_rr_peek(conn);
		if (ctx && (pdu_is_expected(pdu, ctx) || pdu_is_unknown(pdu, ctx) ||
			    pdu_is_reject(pdu, ctx))) {
			/* Response on remote procedure */
			llcp_rr_rx(conn, ctx, rx);
			return;
		}
	}

	/* New remote request */
	llcp_rr_new(conn, rx);
}

#ifdef ZTEST_UNITTEST

int ctx_buffers_free(void)
{
	int nr_of_free_ctx;

	nr_of_free_ctx = mem_free_count_get(mem_ctx.free);

	return nr_of_free_ctx;
}

void test_int_mem_proc_ctx(void)
{
	struct proc_ctx *ctx1;
	struct proc_ctx *ctx2;
	int nr_of_free_ctx;

	ull_cp_init();

	nr_of_free_ctx = ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM, NULL);

	for (int i = 0U; i < CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM; i++) {
		ctx1 = proc_ctx_acquire();

		/* The previous acquire should be valid */
		zassert_not_null(ctx1, NULL);
	}

	nr_of_free_ctx = ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 0, NULL);

	ctx2 = proc_ctx_acquire();

	/* The last acquire should fail */
	zassert_is_null(ctx2, NULL);

	llcp_proc_ctx_release(ctx1);
	nr_of_free_ctx = ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 1, NULL);

	ctx1 = proc_ctx_acquire();

	/* Releasing returns the context to the avilable pool */
	zassert_not_null(ctx1, NULL);
}

void test_int_mem_tx(void)
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

void test_int_create_proc(void)
{
	struct proc_ctx *ctx;

	ull_cp_init();

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);

	zassert_equal(ctx->proc, PROC_VERSION_EXCHANGE, NULL);
	zassert_equal(ctx->collision, 0, NULL);
	zassert_equal(ctx->pause, 0, NULL);

	for (int i = 0U; i < CONFIG_BT_CTLR_LLCP_PROC_CTX_BUF_NUM; i++) {
		zassert_not_null(ctx, NULL);
		ctx = create_procedure(PROC_VERSION_EXCHANGE);
	}

	zassert_is_null(ctx, NULL);
}

#endif
