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

#include "ull_tx_queue.h"

#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_conn_llcp_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "ll_feat.h"

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

#define LLCTRL_PDU_SIZE		(offsetof(struct pdu_data, llctrl) + sizeof(struct pdu_data_llctrl))
#define PROC_CTX_BUF_SIZE	WB_UP(sizeof(struct proc_ctx))
#define TX_CTRL_BUF_SIZE	WB_UP(offsetof(struct node_tx, pdu) + LLCTRL_PDU_SIZE)
#define NTF_BUF_SIZE		WB_UP(offsetof(struct node_rx_pdu, pdu) + LLCTRL_PDU_SIZE)

/* LLCP Allocations */

/* TODO(thoh): Placeholder until Kconfig setting is made */
#if !defined(TX_CTRL_BUF_NUM)
#define TX_CTRL_BUF_NUM 1
#endif

/* TODO(thoh): Placeholder until Kconfig setting is made */
#if !defined(PROC_CTX_BUF_NUM)
#define PROC_CTX_BUF_NUM 1
#endif

/* TODO: Determine correct number of tx nodes */
static uint8_t buffer_mem_tx[TX_CTRL_BUF_SIZE * TX_CTRL_BUF_NUM];
static struct mem_pool mem_tx = { .pool = buffer_mem_tx };

/* TODO: Determine correct number of ctx */
static uint8_t buffer_mem_ctx[PROC_CTX_BUF_SIZE * PROC_CTX_BUF_NUM];
static struct mem_pool mem_ctx = { .pool = buffer_mem_ctx };

/*
 * LLCP Resource Management
 */

static struct proc_ctx *proc_ctx_acquire(void)
{
	struct proc_ctx *ctx;

	ctx = (struct proc_ctx *) mem_acquire(&mem_ctx.free);
	return ctx;
}

void ull_cp_priv_proc_ctx_release(struct proc_ctx *ctx)
{
	mem_release(ctx, &mem_ctx.free);
}

bool ull_cp_priv_tx_alloc_is_available(void)
{
	return mem_tx.free != NULL;
}

struct node_tx *ull_cp_priv_tx_alloc(void)
{
	struct node_tx *tx;

	tx = (struct node_tx *) mem_acquire(&mem_tx.free);
	return tx;
}

static void tx_release(struct node_tx *tx)
{
	mem_release(tx, &mem_tx.free);
}

bool ull_cp_priv_ntf_alloc_is_available(void)
{
	return ll_pdu_rx_alloc_peek(1) != NULL;
}

bool ull_cp_priv_ntf_alloc_num_available(uint8_t count)
{
	return ll_pdu_rx_alloc_peek(count) != NULL;
}

struct node_rx_pdu *ull_cp_priv_ntf_alloc(void)
{
	return ll_pdu_rx_alloc();
}

/*
 * ULL -> LLL Interface
 */

void ull_cp_priv_tx_enqueue(struct ll_conn *conn, struct node_tx *tx)
{
	ull_tx_q_enqueue_ctrl(&conn->tx_q, tx);
}

void ull_cp_priv_tx_pause_data(struct ll_conn *conn)
{
	ull_tx_q_pause_data(&conn->tx_q);
}

void ull_cp_priv_tx_resume_data(struct ll_conn *conn)
{
	ull_tx_q_resume_data(&conn->tx_q);
}

void ull_cp_priv_tx_flush(struct ll_conn *conn)
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
	memset((void *) &ctx->data, 0, sizeof(ctx->data));

	/* Initialize opcodes fields to  known values */
	ctx->rx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->tx_opcode = ULL_LLCP_INVALID_OPCODE;
	ctx->response_opcode = ULL_LLCP_INVALID_OPCODE;

	return ctx;
}

struct proc_ctx *ull_cp_priv_create_local_procedure(enum llcp_proc proc)
{
	struct proc_ctx *ctx;

	ctx = create_procedure(proc);
	if (!ctx) {
		return NULL;
	}

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		lp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		lp_comm_init_proc(ctx);
		break;
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		lp_enc_init_proc(ctx);
		break;
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		lp_pu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_PARAM_REQ:
		lp_cu_init_proc(ctx);
		break;
	case PROC_TERMINATE:
		lp_comm_init_proc(ctx);
		break;
	case PROC_CHAN_MAP_UPDATE:
		lp_chmu_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		lp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	return ctx;
}

struct proc_ctx *ull_cp_priv_create_remote_procedure(enum llcp_proc proc)
{
	struct proc_ctx *ctx;

	ctx = create_procedure(proc);
	if (!ctx) {
		return NULL;
	}

	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		rp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PROC_MIN_USED_CHANS:
		rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PROC_VERSION_EXCHANGE:
		rp_comm_init_proc(ctx);
		break;
	case PROC_ENCRYPTION_START:
	case PROC_ENCRYPTION_PAUSE:
		rp_enc_init_proc(ctx);
		break;
#ifdef CONFIG_BT_CTLR_PHY
	case PROC_PHY_UPDATE:
		rp_pu_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
	case PROC_CONN_PARAM_REQ:
		rp_cu_init_proc(ctx);
		break;
	case PROC_TERMINATE:
		rp_comm_init_proc(ctx);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		rp_comm_init_proc(ctx);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	return ctx;
}

/*
 * LLCP Public API
 */

void ull_cp_init(void)
{
	/**/
	mem_init(mem_ctx.pool, PROC_CTX_BUF_SIZE, PROC_CTX_BUF_NUM, &mem_ctx.free);
	mem_init(mem_tx.pool, TX_CTRL_BUF_SIZE, TX_CTRL_BUF_NUM, &mem_tx.free);
}

void ll_conn_init(struct ll_conn *conn)
{
	/* Reset local request fsm */
	lr_init(conn);
	sys_slist_init(&conn->llcp.local.pend_proc_list);

	/* Reset remote request fsm */
	rr_init(conn);
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

	/* Reset encryption related state */
	conn->lll.enc_tx = 0U;
	conn->lll.enc_rx = 0U;

	conn->lll.event_counter = 0;
	lr_init(conn);
	rr_init(conn);
}

void ull_cp_release_tx(struct node_tx *tx)
{
	tx_release(tx);
}

void ull_cp_release_ntf(struct node_rx_pdu *ntf)
{
	ntf->hdr.next = NULL;
	ll_rx_mem_release((void **)&ntf);
}

void ull_cp_run(struct ll_conn *conn)
{
	rr_run(conn);
	lr_run(conn);
}

void ull_cp_state_set(struct ll_conn *conn, uint8_t state)
{
	switch (state) {
	case ULL_CP_CONNECTED:
		rr_connect(conn);
		lr_connect(conn);
		break;
	case ULL_CP_DISCONNECTED:
		rr_disconnect(conn);
		lr_disconnect(conn);
		break;
	default:
		break;
	}
}

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
uint8_t ull_cp_min_used_chans(struct ll_conn *conn, uint8_t phys, uint8_t min_used_chans)
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_SLAVE ) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx = create_local_procedure(PROC_MIN_USED_CHANS);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.muc.phys = phys;
	ctx->data.muc.min_used_chans = min_used_chans;

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */

#if defined(CONFIG_BT_CTLR_LE_PING)
uint8_t ull_cp_le_ping(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = create_local_procedure(PROC_LE_PING);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_LE_PING */
uint8_t ull_cp_feature_exchange(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = create_local_procedure(PROC_FEATURE_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_version_exchange(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = create_local_procedure(PROC_VERSION_EXCHANGE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_encryption_start(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2], const uint8_t ltk[16])
{
	struct proc_ctx *ctx;

	/* TODO(thoh): Proper checks for role, parameters etc. */

	ctx = create_local_procedure(PROC_ENCRYPTION_START);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Copy input parameters */
	memcpy(ctx->data.enc.rand, rand, sizeof(ctx->data.enc.rand));
	ctx->data.enc.ediv[0] = ediv[0];
	ctx->data.enc.ediv[1] = ediv[1];
	memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));

	/* Enqueue request */
	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_encryption_pause(struct ll_conn *conn, const uint8_t rand[8], const uint8_t ediv[2], const uint8_t ltk[16])
{
	struct proc_ctx *ctx;

	/* TODO(thoh): Proper checks for role, parameters etc. */

	ctx = create_local_procedure(PROC_ENCRYPTION_PAUSE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Copy input parameters */
	memcpy(ctx->data.enc.rand, rand, sizeof(ctx->data.enc.rand));
	ctx->data.enc.ediv[0] = ediv[0];
	ctx->data.enc.ediv[1] = ediv[1];
	memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));

	/* Enqueue request */
	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_encryption_paused(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);
	if (ctx && ctx->proc == PROC_ENCRYPTION_PAUSE) {
		return 1;
	}

	ctx = lr_peek(conn);
	if (ctx && ctx->proc == PROC_ENCRYPTION_PAUSE) {
		return 1;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_PHY)
uint8_t ull_cp_phy_update(struct ll_conn *conn, uint8_t tx, uint8_t flags, uint8_t rx, uint8_t host_initiated)
{
	struct proc_ctx *ctx;

	/* TODO(thoh): Proper checks for role, parameters etc. */

	ctx = create_local_procedure(PROC_PHY_UPDATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.pu.tx = tx;
	ctx->data.pu.flags = flags;
	ctx->data.pu.rx = rx;
	ctx->data.pu.host_initiated = host_initiated;

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_PHY */

uint8_t ull_cp_terminate(struct ll_conn *conn, uint8_t error_code)
{
	struct proc_ctx *ctx;

	lr_abort(conn);

	ctx = create_local_procedure(PROC_TERMINATE);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ctx->data.term.error_code = error_code;

	/* TODO
	 * Termination procedure may be initiated at any time, even if other
	 * LLCP is active.
	 */
	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_cp_chan_map_update(struct ll_conn *conn, uint8_t chm[5])
{
	struct proc_ctx *ctx;

	if (conn->lll.role != BT_HCI_ROLE_MASTER) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO
	 * HCI requires at least 1 channel to be unknown but LL requires 2...
	 * Figure out whom should be responsible for checking this.
	 */
	if ((chm[0] == 0) && (chm[1] == 0) && (chm[2] == 0) && (chm[3] == 0) &&
	    (chm[4] == 0)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* RFU bits */
	if (chm[4] & 0xE0) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	ctx = create_local_procedure(PROC_CHAN_MAP_UPDATE);

	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO
	 * Should probably be stored in conn when integrated with LL
	 */
	memcpy(ctx->data.chmu.chm, chm, sizeof(ctx->data.chmu.chm));

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint8_t ull_cp_data_length_update(struct ll_conn *conn, uint16_t max_tx_octets, uint16_t max_tx_time)
{
	struct proc_ctx *ctx;

	ctx = create_local_procedure(PROC_DATA_LENGTH_UPDATE);

	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Apply update to local */
	ull_dle_local_tx_update(conn, max_tx_octets, max_tx_time);

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

void ull_cp_ltk_req_reply(struct ll_conn *conn, const uint8_t ltk[16])
{
	/* TODO(thoh): Call rp_enc to query if LTK request reply is allowed */
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE)) {
		memcpy(ctx->data.enc.ltk, ltk, sizeof(ctx->data.enc.ltk));
		rp_enc_ltk_req_reply(conn, ctx);
	}
}

void ull_cp_ltk_req_neq_reply(struct ll_conn *conn)
{
	/* TODO(thoh): Call rp_enc to query if LTK negative request reply is allowed */
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);
	if (ctx && (ctx->proc == PROC_ENCRYPTION_START || ctx->proc == PROC_ENCRYPTION_PAUSE)) {
		rp_enc_ltk_req_neg_reply(conn, ctx);
	}
}

uint8_t ull_cp_conn_update(struct ll_conn *conn, uint16_t interval_min, uint16_t interval_max, uint16_t latency, uint16_t timeout)
{
	struct proc_ctx *ctx;

	/* TODO(thoh): Proper checks for role, parameters etc. */

	/* TODO(thoh): Determine proper procedure:
	 *	Role == Slave => CPR
	 *	Role == Master:
	 *		CPR in Peer Feats. => CPR
	 *		CPR not in Peer Feats. => CU
	 *		FEX not performed => CPR
	 *	*/
	ctx = create_local_procedure(PROC_CONN_PARAM_REQ);
	if (!ctx) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lr_enqueue(conn, ctx);

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
uint8_t ull_cp_remote_dle_pending(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);

	return (ctx && ctx->proc == PROC_DATA_LENGTH_UPDATE);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

void ull_cp_conn_param_req_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);
	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		rp_conn_param_req_reply(conn, ctx);
	}
}

void ull_cp_conn_param_req_neg_reply(struct ll_conn *conn)
{
	struct proc_ctx *ctx;

	ctx = rr_peek(conn);
	if (ctx && ctx->proc == PROC_CONN_PARAM_REQ) {
		rp_conn_param_req_neg_reply(conn, ctx);
	}
}

static bool pdu_is_expected(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ctx->rx_opcode == pdu->llctrl.opcode;
}

static bool pdu_is_unknown(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	return ((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) && (ctx->tx_opcode == pdu->llctrl.unknown_rsp.type));
}

static bool pdu_is_reject(struct pdu_data *pdu, struct proc_ctx *ctx)
{
	/* TODO(thoh): For LL_REJECT_IND check if the active procedure is supporting the PDU */
	return (((pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND) && (ctx->tx_opcode == pdu->llctrl.reject_ext_ind.reject_opcode)) || (pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_IND));
}

static bool pdu_is_terminate(struct pdu_data *pdu)
{
	return pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_TERMINATE_IND;
}

void ull_cp_tx_ack(struct ll_conn *conn, struct node_tx *tx)
{
	struct proc_ctx *ctx;

	ctx = lr_peek(conn);
	if (ctx && ctx->tx_ack == tx) {
		/* TX ack re. local request */
		lr_tx_ack(conn, ctx, tx);
	}

	ctx = rr_peek(conn);
	if (ctx && ctx->tx_ack == tx) {
		/* TX ack re. remote response */
		rr_tx_ack(conn, ctx, tx);
	}
}

void ull_cp_rx(struct ll_conn *conn, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu;
	struct proc_ctx *ctx;

	pdu = (struct pdu_data *) rx->pdu;

	if (!pdu_is_terminate(pdu)) {
		/* Process non LL_TERMINATE_IND PDU's as responses to active
		 * procedures */

		ctx = lr_peek(conn);
		if (ctx && (pdu_is_expected(pdu, ctx) || pdu_is_unknown(pdu, ctx) || pdu_is_reject(pdu, ctx))) {
			/* Response on local procedure */
			lr_rx(conn, ctx, rx);
			return;
		}

		ctx = rr_peek(conn);
		if (ctx && (pdu_is_expected(pdu, ctx) || pdu_is_unknown(pdu, ctx) || pdu_is_reject(pdu, ctx))) {
			/* Response on remote procedure */
			rr_rx(conn, ctx, rx);
			return;
		}
	}

	/* New remote request */
	rr_new(conn, rx);
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
	zassert_equal(nr_of_free_ctx, PROC_CTX_BUF_NUM, NULL);

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		ctx1 = proc_ctx_acquire();

		/* The previous acquire should be valid */
		zassert_not_null(ctx1, NULL);
	}

	nr_of_free_ctx = ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 0, NULL);

	ctx2 = proc_ctx_acquire();

	/* The last acquire should fail */
	zassert_is_null(ctx2, NULL);

	proc_ctx_release(ctx1);
	nr_of_free_ctx = ctx_buffers_free();
	zassert_equal(nr_of_free_ctx, 1, NULL);

	ctx1 = proc_ctx_acquire();

	/* Releasing returns the context to the avilable pool */
	zassert_not_null(ctx1, NULL);
}

void test_int_mem_tx(void)
{
	bool peek;
	struct node_tx *tx;
	struct node_tx *txl[TX_CTRL_BUF_NUM];

	ull_cp_init();

	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		peek = tx_alloc_is_available();

		/* The previous tx alloc peek should be valid */
		zassert_true(peek, NULL);

		txl[i] = tx_alloc();

		/* The previous alloc should be valid */
		zassert_not_null(txl[i], NULL);
	}

	peek = tx_alloc_is_available();

	/* The last tx alloc peek should fail */
	zassert_false(peek, NULL);

	tx = tx_alloc();

	/* The last tx alloc should fail */
	zassert_is_null(tx, NULL);

	/* Release all */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx_release(txl[i]);
	}

	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		peek = tx_alloc_is_available();

		/* The previous tx alloc peek should be valid */
		zassert_true(peek, NULL);

		txl[i] = tx_alloc();

		/* The previous alloc should be valid */
		zassert_not_null(txl[i], NULL);
	}

	peek = tx_alloc_is_available();

	/* The last tx alloc peek should fail */
	zassert_false(peek, NULL);

	tx = tx_alloc();

	/* The last tx alloc should fail */
	zassert_is_null(tx, NULL);

	/* Release all */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx_release(txl[i]);
	}
}

void test_int_create_proc(void)
{
	struct ll_conn conn;
	struct proc_ctx *ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ll_conn_init(&conn);

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);

	zassert_equal(ctx->proc, PROC_VERSION_EXCHANGE, NULL);
	zassert_equal(ctx->collision, 0, NULL);
	zassert_equal(ctx->pause, 0, NULL);

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		zassert_not_null(ctx, NULL);
		ctx = create_procedure(PROC_VERSION_EXCHANGE);
	}

	zassert_is_null(ctx, NULL);
}

#endif
