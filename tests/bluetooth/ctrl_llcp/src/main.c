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

#define PROC_CTX_BUF_NUM 2
#define TX_CTRL_BUF_NUM 2
#define NTF_BUF_NUM 2

/* Implementation Under Test Begin */
#include "ll_sw/ull_llcp.c"
/* Implementation Under Test End */

struct ull_tx_q tx_q;
struct ull_cp_conn conn;

sys_slist_t ll_rx_q;

#define PDU_DC_LL_HEADER_SIZE	(offsetof(struct pdu_data, lldata))
#define LL_LENGTH_OCTETS_RX_MAX 27
#define NODE_RX_HEADER_SIZE	(offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD	(NODE_RX_HEADER_SIZE)
#define PDU_DATA_SIZE		(PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)
#define PDU_RX_NODE_SIZE WB_UP(NODE_RX_STRUCT_OVERHEAD + PDU_DATA_SIZE)

void test_api_init(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);

	ull_cp_conn_init(&conn);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.remote.state, RR_STATE_DISCONNECT, NULL);
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
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

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
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Local */

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

	/* Remote */

	peek_ctx = rr_peek(&conn);
	zassert_is_null(peek_ctx, NULL);

	dequeue_ctx = rr_dequeue(&conn);
	zassert_is_null(dequeue_ctx, NULL);

	rr_enqueue(&conn, &ctx);
	peek_ctx = (struct proc_ctx *) sys_slist_peek_head(&conn.remote.pend_proc_list);
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
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);
	zassert_equal(conn.remote.state, RR_STATE_IDLE, NULL);
}

void test_api_disconnect(void)
{
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.remote.state, RR_STATE_DISCONNECT, NULL);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_equal(conn.local.state, LR_STATE_IDLE, NULL);
	zassert_equal(conn.remote.state, RR_STATE_IDLE, NULL);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_equal(conn.local.state, LR_STATE_DISCONNECT, NULL);
	zassert_equal(conn.remote.state, RR_STATE_DISCONNECT, NULL);
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

void helper_pdu_verify_version_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_version_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_VERSION_IND, NULL);
	zassert_equal(pdu->llctrl.version_ind.version_number, p->version_number, NULL);
	zassert_equal(pdu->llctrl.version_ind.company_id, p->company_id, NULL);
	zassert_equal(pdu->llctrl.version_ind.sub_version_number, p->sub_version_number, NULL);
}

void helper_pdu_verify_enc_req(struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_REQ, NULL);
}

void helper_pdu_verify_enc_rsp(struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_ENC_RSP, NULL);
}

void helper_pdu_verify_start_enc_req(struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ, NULL);
}

void helper_pdu_verify_start_enc_rsp(struct pdu_data *pdu, void *param)
{
	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP, NULL);
}

void helper_pdu_verify_reject_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ind) + sizeof(struct pdu_data_llctrl_reject_ind), NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_IND, NULL);
	zassert_equal(pdu->llctrl.reject_ind.error_code, p->error_code, NULL);
}

void helper_pdu_verify_reject_ext_ind(struct pdu_data *pdu, void *param)
{
	struct pdu_data_llctrl_reject_ext_ind *p = param;

	zassert_equal(pdu->ll_id, PDU_DATA_LLID_CTRL, NULL);
	zassert_equal(pdu->len, offsetof(struct pdu_data_llctrl, reject_ext_ind) + sizeof(struct pdu_data_llctrl_reject_ext_ind), NULL);
	zassert_equal(pdu->llctrl.opcode, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND, NULL);
	zassert_equal(pdu->llctrl.reject_ext_ind.reject_opcode, p->reject_opcode, NULL);
	zassert_equal(pdu->llctrl.reject_ext_ind.error_code, p->error_code, NULL);
}

typedef enum {
	LL_VERSION_IND,
	LL_REJECT_IND,
	LL_REJECT_EXT_IND,
	LL_ENC_REQ,
	LL_ENC_RSP,
	LL_START_ENC_REQ,
	LL_START_ENC_RSP,
} helper_opcode_t;

typedef void (helper_func_t) (struct pdu_data * data, void *param);

helper_func_t * const helper_pdu_encode[] = {
	helper_pdu_encode_version_ind,
	NULL,
	helper_pdu_encode_reject_ext_ind,
	helper_pdu_encode_enc_req,
	helper_pdu_encode_enc_rsp,
	helper_pdu_encode_start_enc_req,
	helper_pdu_encode_start_enc_rsp
};

helper_func_t * const helper_pdu_verify[] = {
	helper_pdu_verify_version_ind,
	helper_pdu_verify_reject_ind,
	helper_pdu_verify_reject_ext_ind,
	helper_pdu_verify_enc_req,
	helper_pdu_verify_enc_rsp,
	helper_pdu_verify_start_enc_req,
	helper_pdu_verify_start_enc_rsp
};

void lt_tx(helper_opcode_t opcode, struct ull_cp_conn *conn, void *param)
{
	struct pdu_data *pdu;
	u8_t node_rx_pdu_buf[PDU_RX_NODE_SIZE];
	struct node_rx_pdu *rx;

	rx = (struct node_rx_pdu *) &node_rx_pdu_buf[0];
	pdu = (struct pdu_data *) rx->pdu;
	helper_pdu_encode[opcode](pdu, param);
	ull_cp_rx(conn, rx);
}

void lt_rx(helper_opcode_t opcode, struct ull_cp_conn *conn, struct node_tx **tx_ref, void *param)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	tx = ull_tx_q_dequeue(&tx_q);
	zassert_not_null(tx, NULL);

	pdu = (struct pdu_data *)tx->pdu;
	helper_pdu_verify[opcode](pdu, param);

	*tx_ref = tx;
}

void lt_rx_q_is_empty()
{
	struct node_tx *tx;

	tx = ull_tx_q_dequeue(&tx_q);
	zassert_is_null(tx, NULL);
}

void ut_rx(helper_opcode_t opcode, struct node_rx_pdu **ntf_ref, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ll_rx_q);
	zassert_not_null(ntf, NULL);

	pdu = (struct pdu_data *) ntf->pdu;
	helper_pdu_verify[opcode](pdu, param);

	*ntf_ref = ntf;
}

void ut_rx_q_is_empty()
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ll_rx_q);
	zassert_is_null(ntf, NULL);
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
void test_api_local_version_exchange(void)
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

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Version Exchange Procedure */
	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* There should be one host notification */
	ut_rx(LL_VERSION_IND, &ntf, &remote_version_ind);
	ut_rx_q_is_empty();
}

void test_api_local_version_exchange_2(void)
{
	u8_t err;

	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

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
void test_api_remote_version_exchange(void)
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

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

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
void test_api_both_version_exchange(void)
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

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Initiate a Version Exchange Procedure */
	err = ull_cp_version_exchange(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_VERSION_IND, &conn, &tx, &local_version_ind);
	lt_rx_q_is_empty();

	/* There should be one host notification */
	ut_rx(LL_VERSION_IND, &ntf, &remote_version_ind);
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
void test_api_local_encryption_start(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* There should be one host notification */
	ut_rx(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.enc_rx, 1U, NULL);
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
void test_api_local_encryption_start_limited_memory(void)
{
	u8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

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

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Run */
	ull_cp_run(&conn);

	/* There should be one host notification */
	ut_rx(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.enc_rx, 1U, NULL);
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
void test_api_local_encryption_start_no_ltk(void)
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

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, NULL);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* There should be one host notification */
	ut_rx(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.enc_tx, 0U, NULL);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.enc_rx, 0U, NULL);
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
void test_api_remote_encryption_start(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.enc_rx, 1U, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* There should be a host notification */
	ut_rx(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.enc_tx, 1U, NULL);
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
void test_api_remote_encryption_start_limited_memory(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

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

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Run */
	ull_cp_run(&conn);

	/* There should be one host notification */
	ut_rx(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn);

	/* Tx Queue should not have one LL Control PDU */
	lt_rx_q_is_empty();

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Rx Decryption should be enabled */
	zassert_equal(conn.enc_rx, 1U, NULL);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Run */
	ull_cp_run(&conn);

	/* There should be one host notification */
	ut_rx(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty();

	/* Release tx */
	ull_cp_release_tx(tx);

	/* Run */
	ull_cp_run(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Tx Encryption should be enabled */
	zassert_equal(conn.enc_tx, 1U, NULL);
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
void test_api_remote_encryption_start_no_ltk(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	/* Setup */
	sys_slist_init(&ll_rx_q);
	ull_cp_init();
	ull_tx_q_init(&tx_q);
	ull_cp_conn_init(&conn);
	conn.tx_q = &tx_q;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, NULL);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx(LL_ENC_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_neq_reply(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty();

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Tx Encryption should be disabled */
	zassert_equal(conn.enc_tx, 0U, NULL);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.enc_rx, 0U, NULL);
}



void test_main(void)
{
	ztest_test_suite(test,
			 ztest_unit_test(test_api_init),
			 ztest_unit_test(test_int_mem_proc_ctx),
			 ztest_unit_test(test_int_mem_tx),
			 ztest_unit_test(test_int_mem_ntf),
			 ztest_unit_test(test_int_create_proc),
			 ztest_unit_test(test_int_pending_requests),
			 ztest_unit_test(test_api_connect),
			 ztest_unit_test(test_api_disconnect),
			 ztest_unit_test(test_api_local_version_exchange),
			 ztest_unit_test(test_api_local_version_exchange_2),
			 ztest_unit_test(test_api_remote_version_exchange),
			 ztest_unit_test(test_api_both_version_exchange),
			 ztest_unit_test(test_api_local_encryption_start),
			 ztest_unit_test(test_api_local_encryption_start_limited_memory),
			 ztest_unit_test(test_api_local_encryption_start_no_ltk),
			 ztest_unit_test(test_api_remote_encryption_start),
			 ztest_unit_test(test_api_remote_encryption_start_limited_memory),
			 ztest_unit_test(test_api_remote_encryption_start_no_ltk)
			 );
	ztest_run_test_suite(test);
}
