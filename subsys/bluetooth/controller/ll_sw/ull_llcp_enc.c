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
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_enc
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure Encryption FSM states */
enum {
	LP_ENC_STATE_IDLE,
	LP_ENC_STATE_WAIT_TX_ENC_REQ,
	LP_ENC_STATE_WAIT_RX_ENC_RSP,
	LP_ENC_STATE_WAIT_RX_START_ENC_REQ,
	LP_ENC_STATE_WAIT_TX_START_ENC_RSP,
	LP_ENC_STATE_WAIT_RX_START_ENC_RSP,
	LP_ENC_STATE_WAIT_NTF,
};

/* LLCP Local Procedure Encryption FSM events */
enum {
	/* Procedure prepared */
	LP_ENC_EVT_RUN,

	/* Response recieved */
	LP_ENC_EVT_ENC_RSP,

	/* Request recieved */
	LP_ENC_EVT_START_ENC_REQ,

	/* Response recieved */
	LP_ENC_EVT_START_ENC_RSP,

	/* Reject response recieved */
	LP_ENC_EVT_REJECT,

	/* Unknown response recieved */
	LP_ENC_EVT_UNKNOWN,
};

/* LLCP Remote Procedure Encryption FSM states */
enum {
	RP_ENC_STATE_IDLE,
	RP_ENC_STATE_WAIT_RX_ENC_REQ,
	RP_ENC_STATE_WAIT_TX_ENC_RSP,
	RP_ENC_STATE_WAIT_NTF_LTK_REQ,
	RP_ENC_STATE_WAIT_LTK_REPLY,
	RP_ENC_STATE_WAIT_TX_START_ENC_REQ,
	RP_ENC_STATE_WAIT_TX_REJECT_IND,
	RP_ENC_STATE_WAIT_RX_START_ENC_RSP,
	RP_ENC_STATE_WAIT_NTF,
	RP_ENC_STATE_WAIT_TX_START_ENC_RSP,
};

/* LLCP Remote Procedure Encryption FSM events */
enum {
	/* Procedure prepared */
	RP_ENC_EVT_RUN,

	/* Request recieved */
	RP_ENC_EVT_ENC_REQ,

	/* Response recieved */
	RP_ENC_EVT_START_ENC_RSP,

	/* LTK request reply */
	RP_ENC_EVT_LTK_REQ_REPLY,

	/* LTK request negative reply */
	RP_ENC_EVT_LTK_REQ_NEG_REPLY,

	/* Reject response recieved */
	RP_ENC_EVT_REJECT,

	/* Unknown response recieved */
	RP_ENC_EVT_UNKNOWN,
};

/*
 * ULL -> LL Interface
 */

extern void ll_rx_enqueue(struct node_rx_pdu *rx);

/*
 * LLCP Local Procedure Encryption FSM
 */

static void lp_enc_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		pdu_encode_enc_req(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		pdu_encode_start_enc_rsp(pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_enc_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;

	if (ctx->data.enc.error == BT_HCI_ERR_SUCCESS) {
		/* TODO(thoh): is this correct? */
		pdu_encode_start_enc_rsp(pdu);
	} else {
		pdu_encode_reject_ind(pdu, ctx->data.enc.error);
	}

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void lp_enc_complete(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = LP_ENC_STATE_WAIT_NTF;
	} else {
		lp_enc_ntf(conn, ctx);
		lr_complete(conn);
		ctx->state = LP_ENC_STATE_IDLE;
	}
}

static void lp_enc_send_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_ENC_STATE_WAIT_TX_ENC_REQ;
	} else {
		lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_ENC_REQ);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
		ctx->state = LP_ENC_STATE_WAIT_RX_ENC_RSP;
	}
}

static void lp_enc_send_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_ENC_STATE_WAIT_TX_START_ENC_RSP;
	} else {
		lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
		ctx->state = LP_ENC_STATE_WAIT_RX_START_ENC_RSP;

		/* Tx Encryption enabled */
		conn->lll.enc_tx = 1U;

		/* Rx Decryption enabled */
		conn->lll.enc_rx = 1U;
	}
}

static void lp_enc_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_ENC_EVT_RUN:
		tx_pause_data(conn);
		tx_flush(conn);
		lp_enc_send_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_tx_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_send_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_rx_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_ENC_EVT_ENC_RSP:
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
		ctx->state = LP_ENC_STATE_WAIT_RX_START_ENC_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_rx_start_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	struct pdu_data *pdu = (struct pdu_data *) param;

	switch (evt) {
	case LP_ENC_EVT_START_ENC_REQ:
		lp_enc_send_start_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_EVT_REJECT:
		ctx->data.enc.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_tx_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_send_start_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_rx_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_ENC_EVT_START_ENC_RSP:
		ctx->data.enc.error = BT_HCI_ERR_SUCCESS;
		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_ENC_STATE_IDLE:
		lp_enc_st_idle(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_TX_ENC_REQ:
		lp_enc_st_wait_tx_enc_req(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_RX_ENC_RSP:
		lp_enc_st_wait_rx_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_RX_START_ENC_REQ:
		lp_enc_st_wait_rx_start_enc_req(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_TX_START_ENC_RSP:
		lp_enc_st_wait_tx_start_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_RX_START_ENC_RSP:
		lp_enc_st_wait_rx_start_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_NTF:
		lp_enc_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_ENC_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_START_ENC_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_START_ENC_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_enc_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_ENC_STATE_IDLE;
}

void ull_cp_priv_lp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Encryption FSM
 */

static void rp_enc_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		pdu_encode_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		pdu_encode_start_enc_req(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		pdu_encode_start_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* TODO(thoh): Select between LL_REJECT_IND and LL_REJECT_EXT_IND */
		pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_ENC_REQ, BT_HCI_ERR_PIN_OR_KEY_MISSING);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_enc_ntf_ltk(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;

	/* TODO(thoh): is this correct? */
	pdu_encode_enc_req(pdu);

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_enc_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;

	/* TODO(thoh): is this correct? */
	pdu_encode_start_enc_rsp(pdu);

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_enc_send_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param);

static void rp_enc_complete(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_NTF;
	} else {
		rp_enc_ntf(conn, ctx);
		rp_enc_send_start_enc_rsp(conn, ctx, evt, param);
	}
}

static void rp_enc_send_ltk_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_NTF_LTK_REQ;
	} else {
		rp_enc_ntf_ltk(conn, ctx);
		ctx->state = RP_ENC_STATE_WAIT_LTK_REPLY;
	}
}

static void rp_enc_send_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_TX_ENC_RSP;
	} else {
		rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_ENC_RSP);
		rp_enc_send_ltk_ntf(conn, ctx, evt, param);
	}
}

static void rp_enc_send_start_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_TX_START_ENC_REQ;
	} else {
		rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
		ctx->state = RP_ENC_STATE_WAIT_RX_START_ENC_RSP;

		/* Rx Decryption enabled */
		conn->lll.enc_rx = 1U;
	}
}

static void rp_enc_send_reject_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_TX_REJECT_IND;
	} else {
		rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_IND);
		rr_complete(conn);
		ctx->state = RP_ENC_STATE_IDLE;
	}
}

static void rp_enc_send_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_TX_START_ENC_RSP;
	} else {
		rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP);
		rr_complete(conn);
		ctx->state = RP_ENC_STATE_IDLE;

		/* Tx Encryption enabled */
		conn->lll.enc_tx = 1U;
	}
}

static void rp_enc_state_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		ctx->state = RP_ENC_STATE_WAIT_RX_ENC_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_rx_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_ENC_REQ:
		rp_enc_send_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ntf_ltk_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_ltk_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ltk_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_LTK_REQ_REPLY:
		rp_enc_send_start_enc_req(conn, ctx, evt, param);
		break;
	case RP_ENC_EVT_LTK_REQ_NEG_REPLY:
		rp_enc_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_start_enc_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_start_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_reject_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_enc_state_wait_rx_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_START_ENC_RSP:
		rp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_start_enc_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_start_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_ENC_STATE_IDLE:
		rp_enc_state_idle(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_RX_ENC_REQ:
		rp_enc_state_wait_rx_enc_req(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_TX_ENC_RSP:
		rp_enc_state_wait_tx_enc_rsp(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_NTF_LTK_REQ:
		rp_enc_state_wait_ntf_ltk_req(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_LTK_REPLY:
		rp_enc_state_wait_ltk_reply(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_TX_START_ENC_REQ:
		rp_enc_state_wait_tx_start_enc_req(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_TX_REJECT_IND:
		rp_enc_state_wait_tx_reject_ind(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_RX_START_ENC_RSP:
		rp_enc_state_wait_rx_start_enc_rsp(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_NTF:
		rp_enc_state_wait_ntf(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_TX_START_ENC_RSP:
		rp_enc_state_wait_tx_start_enc_rsp(conn, ctx, evt, param);
		break;

	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_enc_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_ENC_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_START_ENC_RSP, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_enc_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_ENC_STATE_IDLE;
}

void ull_cp_priv_rp_enc_ltk_req_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_LTK_REQ_REPLY, NULL);
}

void ull_cp_priv_rp_enc_ltk_req_neg_reply(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_LTK_REQ_NEG_REPLY, NULL);
}

void ull_cp_priv_rp_enc_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_RUN, param);
}
