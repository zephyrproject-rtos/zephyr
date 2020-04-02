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
#define CONFIG_BT_CTLR_LE_ENC 1

#define CONFIG_BT_MAX_CONN 1
#define CONFIG_BT_CONN 1
#define CONFIG_BT_CTLR_PHY 1

#define ULL_LLCP_UNITTEST

#define PROC_CTX_BUF_NUM 2
#define TX_CTRL_BUF_NUM 2
#define NTF_BUF_NUM 2

/* Implementation Under Test Begin */
#include "ll_sw/ull_llcp.c"
/* Implementation Under Test End */

struct ull_cp_conn conn;

sys_slist_t ut_rx_q;
sys_slist_t lt_tx_q;

#define PDU_DC_LL_HEADER_SIZE	(offsetof(struct pdu_data, lldata))
#define LL_LENGTH_OCTETS_RX_MAX 27
#define NODE_RX_HEADER_SIZE	(offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD	(NODE_RX_HEADER_SIZE)
#define PDU_DATA_SIZE		(PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)
#define PDU_RX_NODE_SIZE WB_UP(NODE_RX_STRUCT_OVERHEAD + PDU_DATA_SIZE)

K_MEM_SLAB_DEFINE(lt_tx_pdu_slab, PDU_RX_NODE_SIZE, 10, 4);

void ll_rx_enqueue(struct node_rx_pdu *rx)
{
	sys_slist_append(&ut_rx_q, (sys_snode_t *) rx);
}

void test_api_init(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);

	ull_cp_conn_init(&conn);
	zassert_equal(conn.llcp.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.llcp.remote.state, RR_STATE_DISCONNECT, NULL);
}

void test_int_mem_proc_ctx(void)
{
	struct proc_ctx *ctx1;
	struct proc_ctx *ctx2;

	ull_cp_init();

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		ctx1 = proc_ctx_acquire();

		/* The previous acquire should be valid */
		zassert_not_null(ctx1, NULL);
	}

	ctx2 = proc_ctx_acquire();

	/* The last acquire should fail */
	zassert_is_null(ctx2, NULL);

	proc_ctx_release(ctx1);
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

void test_int_mem_ntf(void)
{
	bool peek;
	struct node_rx_pdu *ntf;
	struct node_rx_pdu *ntfl[NTF_BUF_NUM];

	ull_cp_init();

	for (int i = 0U; i < NTF_BUF_NUM; i++) {
		peek = ntf_alloc_is_available();

		/* The previous ntf alloc peek should be valid */
		zassert_true(peek, NULL);

		ntfl[i] = ntf_alloc();

		/* The previous alloc should be valid */
		zassert_not_null(ntfl[i], NULL);
	}

	peek = ntf_alloc_is_available();

	/* The last ntf alloc peek should fail */
	zassert_false(peek, NULL);

	ntf = ntf_alloc();

	/* The last ntf alloc should fail */
	zassert_is_null(ntf, NULL);

	/* Release all */
	for (int i = 0U; i < NTF_BUF_NUM; i++) {
		ntf_release(ntfl[i]);
	}

	for (int i = 0U; i < NTF_BUF_NUM; i++) {
		peek = ntf_alloc_is_available();

		/* The previous ntf alloc peek should be valid */
		zassert_true(peek, NULL);

		ntfl[i] = ntf_alloc();

		/* The previous alloc should be valid */
		zassert_not_null(ntfl[i], NULL);
	}

	peek = ntf_alloc_is_available();

	/* The last ntf alloc peek should fail */
	zassert_false(peek, NULL);

	ntf = ntf_alloc();

	/* The last ntf alloc should fail */
	zassert_is_null(ntf, NULL);
}

void test_int_create_proc(void)
{
	struct proc_ctx *ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	ctx = create_procedure(PROC_VERSION_EXCHANGE);
	zassert_not_null(ctx, NULL);

	zassert_equal(ctx->proc, PROC_VERSION_EXCHANGE, NULL);
	zassert_equal(ctx->state, LP_COMMON_STATE_IDLE, NULL);
	zassert_equal(ctx->collision, 0, NULL);
	zassert_equal(ctx->pause, 0, NULL);

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		zassert_not_null(ctx, NULL);
		ctx = create_procedure(PROC_VERSION_EXCHANGE);
	}

	zassert_is_null(ctx, NULL);
}

void test_int_pending_requests(void)
{
	struct proc_ctx *peek_ctx;
	struct proc_ctx *dequeue_ctx;
	struct proc_ctx ctx;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	/* Local */

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

	/* Remote */

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

void test_api_connect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_equal(conn.llcp.local.state, LR_STATE_IDLE, NULL);
	zassert_equal(conn.llcp.remote.state, RR_STATE_IDLE, NULL);
}

void test_api_disconnect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_equal(conn.llcp.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.llcp.remote.state, RR_STATE_DISCONNECT, NULL);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_equal(conn.llcp.local.state, LR_STATE_IDLE, NULL);
	zassert_equal(conn.llcp.remote.state, RR_STATE_IDLE, NULL);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_equal(conn.llcp.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.llcp.remote.state, RR_STATE_DISCONNECT, NULL);
}

void helper_pdu_encode_version_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, version_ind) + sizeof(struct pdu_data_llctrl_version_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
	pdu->llctrl.version_ind.version_number = p->version_number;
	pdu->llctrl.version_ind.company_id = p->company_id;
	pdu->llctrl.version_ind.sub_version_number = p->sub_version_number;
}

void helper_pdu_encode_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_req) + sizeof(struct pdu_data_llctrl_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, enc_rsp) + sizeof(struct pdu_data_llctrl_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_start_enc_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_req) + sizeof(struct pdu_data_llctrl_start_enc_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_start_enc_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, start_enc_rsp) + sizeof(struct pdu_data_llctrl_start_enc_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_reject_ext_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
	pdu->llctrl.reject_ext_ind.reject_opcode = p->reject_opcode;
	pdu->llctrl.reject_ext_ind.error_code = p->error_code;
}


void helper_pdu_encode_phy_req(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_phy_rsp(struct pdu_data *pdu, void *param)
{
	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_phy_update_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_phy_upd_ind *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
	pdu->llctrl.phy_upd_ind.instant = p->instant;
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_encode_unknown_rsp(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	pdu->ll_id = PDU_DATA_LLID_CTRL;
	pdu->len = offsetof(struct pdu_data_llctrl, unknown_rsp) + sizeof(struct pdu_data_llctrl_unknown_rsp);
	pdu->llctrl.opcode = PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP;
	pdu->llctrl.unknown_rsp.type = p->type;
}

void helper_pdu_verify_version_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_VERSION_IND, "Not a LL_VERSION_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.version_number, p->version_number, "Wrong version number.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.company_id, p->company_id, "Wrong company id.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.version_ind.sub_version_number, p->sub_version_number, "Wrong sub version number.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_enc_req(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ, "Not a LL_ENC_REQ. Called at %s:%d\n", file, line);
}

void helper_pdu_verify_enc_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_RSP, "Not a LL_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_req(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ, "Not a LL_START_ENC_REQ.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_start_enc_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP, "Not a LL_START_ENC_RSP.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_IND, "Not a LL_REJECT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ind.error_code, p->error_code, "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_reject_ext_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND, "Not a LL_REJECT_EXT_IND.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.reject_opcode, p->reject_opcode, "Reject opcode mismatch.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.reject_ext_ind.error_code, p->error_code, "Error code mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_pdu_verify_phy_req(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_req) + sizeof(struct pdu_data_llctrl_phy_req), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_REQ, "Not a LL_PHY_REQ.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_phy_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_rsp) + sizeof(struct pdu_data_llctrl_phy_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_RSP, "Not a LL_PHY_RSP.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_phy_update_ind(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, phy_upd_ind) + sizeof(struct pdu_data_llctrl_phy_upd_ind), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND, "Not a LL_PHY_UPDATE_IND.\nCalled at %s:%d\n", file, line);
	/* TODO(thoh): Fill in correct data */
}

void helper_pdu_verify_unknown_rsp(const char *file, u32_t line, struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_unknown_rsp *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, "Not a Control PDU.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, unknown_rsp) + sizeof(struct pdu_data_llctrl_unknown_rsp), "Wrong length.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP, "Not a LL_UNKNOWN_RSP.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->llctrl.unknown_rsp.type, p->type, "Type mismatch.\nCalled at %s:%d\n", file, line);
}

void helper_node_verify_phy_update(const char *file, u32_t line, struct node_rx_pdu *rx, void *param)
{
	struct node_rx_pu *pdu = (struct node_rx_pu *)rx->pdu;
	struct node_rx_pu *p = param;

	zassert_equal(rx->hdr.type, NODE_RX_TYPE_PHY_UPDATE, "Not a PHY_UPDATE node.\nCalled at %s:%d\n", file, line);
	zassert_equal(pdu->status, p->status, "Status mismatch.\nCalled at %s:%d\n", file, line);
}

typedef enum {
	LL_VERSION_IND,
	LL_REJECT_IND,
	LL_REJECT_EXT_IND,
	LL_ENC_REQ,
	LL_ENC_RSP,
	LL_START_ENC_REQ,
	LL_START_ENC_RSP,
	LL_PHY_REQ,
	LL_PHY_RSP,
	LL_PHY_UPDATE_IND,
	LL_UNKNOWN_RSP,
} helper_pdu_opcode_t;

typedef void (helper_pdu_encode_func_t) (struct pdu_data * data, void *param);
typedef void (helper_pdu_verify_func_t) (const char* file, u32_t line, struct pdu_data * data, void *param);

helper_pdu_encode_func_t * const helper_pdu_encode[] = {
	helper_pdu_encode_version_ind,
	NULL,
	helper_pdu_encode_reject_ext_ind,
	helper_pdu_encode_enc_req,
	helper_pdu_encode_enc_rsp,
	helper_pdu_encode_start_enc_req,
	helper_pdu_encode_start_enc_rsp,
	helper_pdu_encode_phy_req,
	helper_pdu_encode_phy_rsp,
	helper_pdu_encode_phy_update_ind,
	helper_pdu_encode_unknown_rsp,
};

helper_pdu_verify_func_t * const helper_pdu_verify[] = {
	helper_pdu_verify_version_ind,
	helper_pdu_verify_reject_ind,
	helper_pdu_verify_reject_ext_ind,
	helper_pdu_verify_enc_req,
	helper_pdu_verify_enc_rsp,
	helper_pdu_verify_start_enc_req,
	helper_pdu_verify_start_enc_rsp,
	helper_pdu_verify_phy_req,
	helper_pdu_verify_phy_rsp,
	helper_pdu_verify_phy_update_ind,
	helper_pdu_verify_unknown_rsp,
};

typedef enum {
	NODE_PHY_UPDATE
} helper_node_opcode_t;

typedef void (helper_node_verify_func_t) (const char* file, u32_t line, struct node_rx_pdu *rx, void *param);

helper_node_verify_func_t * const helper_node_verify[] = {
	helper_node_verify_phy_update,
};

#define lt_tx(_opcode, _conn, _param) lt_tx_real(__FILE__, __LINE__, _opcode, _conn, _param)

static void lt_tx_real(const char *file, u32_t line, helper_pdu_opcode_t opcode, struct ull_cp_conn *conn, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *rx;

	int ret = k_mem_slab_alloc(&lt_tx_pdu_slab, (void **) &rx, K_NO_WAIT);
	zassert_equal(0, ret, "Out of memory.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *) rx->pdu;
	zassert_not_null(helper_pdu_encode[opcode], "PDU encode function cannot be NULL\n");
	helper_pdu_encode[opcode](pdu, param);

	sys_slist_append(&lt_tx_q, (sys_snode_t *) rx);
}

#define lt_rx(_opcode, _conn, _tx_ref, _param) lt_rx_real(__FILE__, __LINE__, _opcode, _conn, _tx_ref, _param)

static void lt_rx_real(const char* file, u32_t line, helper_pdu_opcode_t opcode, struct ull_cp_conn *conn, struct node_tx **tx_ref, void *param)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	tx = ull_tx_q_dequeue(&conn->tx_q);
	zassert_not_null(tx, "Tx Q empty.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *)tx->pdu;
	if (helper_pdu_verify[opcode]) {
		helper_pdu_verify[opcode](file, line, pdu, param);
	}

	*tx_ref = tx;
}

#define lt_rx_q_is_empty() lt_rx_q_is_empty_real(__FILE__, __LINE__)

static void lt_rx_q_is_empty_real(const char* file, u32_t line)
{
	struct node_tx *tx;

	tx = ull_tx_q_dequeue(&conn.tx_q);
	zassert_is_null(tx, "Tx Q not empty.\nCalled at %s:%d\n", file, line);
}

#define ut_rx_pdu(_opcode, _ntf_ref, _param) ut_rx_pdu_real(__FILE__, __LINE__, _opcode, _ntf_ref, _param)

static void ut_rx_pdu_real(const char* file, u32_t line, helper_pdu_opcode_t opcode, struct node_rx_pdu **ntf_ref, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU, "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *) ntf->pdu;
	if (helper_pdu_verify[opcode]) {
		helper_pdu_verify[opcode](file, line, pdu, param);
	}

	*ntf_ref = ntf;
}

#define ut_rx_node(_opcode, _ntf_ref, _param) ut_rx_node_real(__FILE__, __LINE__, _opcode, _ntf_ref, _param)

static void ut_rx_node_real(const char* file, u32_t line, helper_node_opcode_t opcode, struct node_rx_pdu **ntf_ref, void *param)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_not_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU, "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	if (helper_node_verify[opcode]) {
		helper_node_verify[opcode](file, line, ntf, param);
	}

	*ntf_ref = ntf;
}

#define ut_rx_q_is_empty() ut_rx_q_is_empty_real(__FILE__, __LINE__)

static void ut_rx_q_is_empty_real(const char* file, u32_t line)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_is_null(ntf, "Ntf Q not empty.\nCalled at %s:%d\n", file, line);
}

static u16_t lazy = 0;

void prepare(struct ull_cp_conn *conn)
{
	struct mocked_lll_conn *lll;

	/*** ULL Prepare ***/

	/* Handle any LL Control Procedures */
	ull_cp_run(conn);

	/*** LLL Prepare ***/
	lll = &conn->lll;

	/* Save the latency for use in event */
	lll->latency_prepare += lazy;

	/* Calc current event counter value */
	u16_t event_counter = lll->event_counter + lll->latency_prepare;

	/* Store the next event counter value */
	lll->event_counter = event_counter + 1;

	lll->latency_prepare = 0;

	/* Rest lazy */
	lazy = 0;
}

void done(struct ull_cp_conn *conn)
{
	struct node_rx_pdu *rx;

	while ((rx = (struct node_rx_pdu *) sys_slist_get(&lt_tx_q))) {
		ull_cp_rx(conn, rx);
		k_mem_slab_free(&lt_tx_pdu_slab, (void **) &rx);
	}
}

static u16_t event_counter(struct ull_cp_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct mocked_lll_conn *lll;
	u16_t event_counter;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static bool is_instant_reached(struct ull_cp_conn *conn, u16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

static void setup()
{
	/* Initialize the upper test rx queue */
	sys_slist_init(&ut_rx_q);

	/* Initialize the lower tester tx queue */
	sys_slist_init(&lt_tx_q);

	/* Initialize the control procedure code */
	ull_cp_init();

	/* Initialize the ULL TX Q */
	ull_tx_q_init(&conn.tx_q);

	/* Initialize the connection object */
	ull_cp_conn_init(&conn);
}

static void set_role(struct ull_cp_conn *conn, u8_t role)
{
	conn->lll.role = role;
}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Version Exchange Proc.     |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_VERSION_IND    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_VERSION_IND |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |     Version Exchange Proc. |                   |
 *    |                   Complete |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
void test_version_exchange_mas_loc(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_version_ind local_version_ind = {
		.version_number = LL_VERSION_NUMBER,
		.company_id = CONFIG_BT_CTLR_COMPANY_ID,
		.sub_version_number = CONFIG_BT_CTLR_SUBVERSION_NUMBER,
	};

	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Version Exchange Procedure */
	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_VERSION_IND, &ntf, &remote_version_ind);
	ut_rx_q_is_empty();
}

void test_version_exchange_mas_loc_2(void)
{
	u8_t err;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_cp_conn_init(&conn);

	err = ull_cp_version_exchange(&conn);

	for (int i = 0U; i < PROC_CTX_BUF_NUM; i++) {
		zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
		err = ull_cp_version_exchange(&conn);
	}

	zassert_not_equal(err, BT_HCI_ERR_SUCCESS, NULL);
}

/* +-----+ +-------+            +-----+
 * | UT  | | LL_A  |            | LT  |
 * +-----+ +-------+            +-----+
 *    |        |                   |
 *    |        |    LL_VERSION_IND |
 *    |        |<------------------|
 *    |        |                   |
 *    |        | LL_VERSION_IND    |
 *    |        |------------------>|
 *    |        |                   |
 */
void test_version_exchange_mas_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_version_ind local_version_ind = {
		.version_number = LL_VERSION_NUMBER,
		.company_id = CONFIG_BT_CTLR_COMPANY_ID,
		.sub_version_number = CONFIG_BT_CTLR_SUBVERSION_NUMBER,
	};

	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    |                            |    LL_VERSION_IND |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |                            | LL_VERSION_IND    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Version Exchange Proc.     |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |     Version Exchange Proc. |                   |
 *    |                   Complete |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
void test_version_exchange_mas_rem_2(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_version_ind local_version_ind = {
		.version_number = LL_VERSION_NUMBER,
		.company_id = CONFIG_BT_CTLR_COMPANY_ID,
		.sub_version_number = CONFIG_BT_CTLR_SUBVERSION_NUMBER,
	};

	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Initiate a Version Exchange Procedure */
	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_VERSION_IND, &ntf, &remote_version_ind);
	ut_rx_q_is_empty();
}


/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_REQ |
 *    |                            |<--------------------|
 *    |          ----------------\ |                     |
 *    |          | Tx Encryption |-|                     |
 *    |          | Rx Decryption | |                     |
 *    |          |---------------| |                     |
 *    |                            |                     |
 *    |                            | LL_START_ENC_RSP    |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
void test_encryption_start_mas_loc(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |         -----------------\ |                     |
 *    |         | Reserver all   |-|                     |
 *    |         | Tx/Ntf buffers | |                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_REQ |
 *    |                            |<--------------------|
 *    |          ----------------\ |                     |
 *    |          | Tx Encryption |-|                     |
 *    |          | Rx Decryption | |                     |
 *    |          |---------------| |                     |
 *    |                            |                     |
 *    |                            | LL_START_ENC_RSP    |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
void test_encryption_start_mas_loc_limited_memory(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Steal all tx buffers */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx = tx_alloc();
		zassert_not_null(tx, NULL);
	}

	/* Steal all ntf buffers */
	for (int i = 0U; i < NTF_BUF_NUM; i++) {
		ntf = ntf_alloc();
		zassert_not_null(ntf, NULL);
	}

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	prepare(&conn);

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |   LL_REJECT_EXT_IND |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
void test_encryption_start_mas_loc_no_ltk(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U, NULL);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.lll.enc_rx, 0U, NULL);
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_REQ    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Rx Decryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 *    |                       |    LL_START_ENC_RSP |
 *    |                       |<--------------------|
 *    |                       |                     |
 *    |     Encryption Change |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_RSP    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Tx Encryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 */
void test_encryption_start_mas_rem(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/* There should be a host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |    -----------------\ |                     |
 *    |    | Reserver all   |-|                     |
 *    |    | Tx/Ntf buffers | |                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_REQ    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Rx Decryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 *    |                       |    LL_START_ENC_RSP |
 *    |                       |<--------------------|
 *    |                       |                     |
 *    |     Encryption Change |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_RSP    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Tx Encryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 */
void test_encryption_start_mas_rem_limited_memory(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Steal all tx buffers */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx = tx_alloc();
		zassert_not_null(tx, NULL);
	}

	/* Steal all ntf buffers */
	for (int i = 0U; i < NTF_BUF_NUM; i++) {
		ntf = ntf_alloc();
		zassert_not_null(ntf, NULL);
	}

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	prepare(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should not have one LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	prepare(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_REJECT_EXT_IND   |
 *    |                       |-------------------->|
 */
void test_encryption_start_mas_rem_no_ltk(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_neq_reply(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U, NULL);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.lll.enc_rx, 0U, NULL);
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 */
void test_phy_update_mas_loc(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	u16_t instant;

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_PHY_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_mas_loc_unsupp_feat(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_PHY_REQ
	};

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_mas_rem(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	u16_t instant;

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_sla_loc(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	u16_t instant;

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_phy_upd_ind phy_update_ind = {0};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Rx */
	phy_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &phy_update_ind);

	/* Done */
	done(&conn);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_sla_rem(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	u16_t instant;

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_phy_upd_ind phy_update_ind = {0};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	phy_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &phy_update_ind);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_mas_loc_collision(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	u16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct node_rx_pu pu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/*** ***/

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/*** ***/

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/*** ***/

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_PHY_RSP, &conn, NULL);

	/* Done */
	done(&conn);

	/*** ***/

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/*** ***/

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

void test_phy_update_sla_loc_collision(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	u16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct node_rx_pu pu = {0};
	struct pdu_data_llctrl_phy_upd_ind phy_update_ind = {0};

	/* Role */
	set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*** ***/

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, NULL);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	done(&conn);

	/* There should be one host notification */
	pu.status = BT_HCI_ERR_LL_PROC_COLLISION;
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	prepare(&conn);

	/* Rx */
	phy_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &phy_update_ind);

	/* Done */
	done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant))
	{
		/* Prepare */
		prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty();

		/* Done */
		done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Done */
	done(&conn);

	/* There should be one host notification */
	pu.status = BT_HCI_ERR_SUCCESS;
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

}


void test_main(void)
{
	ztest_test_suite(internal,
			 ztest_unit_test(test_int_mem_proc_ctx),
			 ztest_unit_test(test_int_mem_tx),
			 ztest_unit_test(test_int_mem_ntf),
			 ztest_unit_test(test_int_create_proc),
			 ztest_unit_test(test_int_pending_requests)
			);

	ztest_test_suite(public,
			 ztest_unit_test(test_api_init),
			 ztest_unit_test(test_api_connect),
			 ztest_unit_test(test_api_disconnect)
			);

	ztest_test_suite(version_exchange,
			 ztest_unit_test_setup_teardown(test_version_exchange_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_version_exchange_mas_loc_2, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_version_exchange_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_version_exchange_mas_rem_2, setup, unit_test_noop)
			);

	ztest_test_suite(encryption,
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_loc_limited_memory, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_loc_no_ltk, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_rem_limited_memory, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_start_mas_rem_no_ltk, setup, unit_test_noop)
			);

	ztest_test_suite(phy,
			 ztest_unit_test_setup_teardown(test_phy_update_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_mas_loc_unsupp_feat, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_sla_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_sla_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_mas_loc_collision, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_phy_update_sla_loc_collision, setup, unit_test_noop)
			);

	ztest_run_test_suite(internal);
	ztest_run_test_suite(public);
	ztest_run_test_suite(version_exchange);
	ztest_run_test_suite(encryption);
	ztest_run_test_suite(phy);
}
