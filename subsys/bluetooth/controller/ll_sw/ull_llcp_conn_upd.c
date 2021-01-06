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
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_conn_upd
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure Connection Update FSM states */
enum {
	LP_CU_STATE_IDLE,
	LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ,
	LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP,
	LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_INSTANT,
	LP_CU_STATE_WAIT_NTF,
};

/* LLCP Local Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	LP_CU_EVT_RUN,

	/* Response recieved */
	LP_CU_EVT_CONN_PARAM_RSP,

	/* Indication recieved */
	LP_CU_EVT_CONN_UPDATE_IND,

	/* Reject response recieved */
	LP_CU_EVT_REJECT,

	/* Unknown response recieved */
	LP_CU_EVT_UNKNOWN,
};

/* LLCP Remote Procedure Connection Update FSM states */
enum {
	RP_CU_STATE_IDLE,
	RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY,
	RP_CU_STATE_WAIT_TX_REJECT_IND,
	RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP,
	RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_INSTANT,
	RP_CU_STATE_WAIT_NTF,
};

/* LLCP Remote Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	RP_CU_EVT_RUN,

	/* Request recieved */
	RP_CU_EVT_CONN_PARAM_REQ,

	/* Indication recieved */
	RP_CU_EVT_CONN_UPDATE_IND,

	/* CONN_PARAM_REQ reply */
	RP_CU_EVT_CONN_PARAM_REQ_REPLY,

	/* CONN_PARAM_REQ negative reply */
	RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY,
};

/*
 * ULL -> LL Interface
 */

extern void ll_rx_enqueue(struct node_rx_pdu *rx);

/*
 * LLCP Local Procedure Connection Update FSM
 */

static uint16_t lp_event_counter(struct ll_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct lll_conn *lll;
	uint16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	uint16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void lp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		pdu_encode_conn_param_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		pdu_encode_conn_update_ind(ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_cu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_cu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CONN_UPDATE;
	pdu = (struct node_rx_cu *)ntf->pdu;

	pdu->status = ctx->data.cu.error;

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void lp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = LP_CU_STATE_WAIT_NTF;
	} else {
		lp_cu_ntf(conn, ctx);
		lr_complete(conn);
		ctx->state = LP_CU_STATE_IDLE;
	}
}

static void lp_cu_send_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available() || rr_get_collision(conn)) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ;
	} else {
		rr_set_incompat(conn, INCOMPAT_RESOLVABLE);

		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ);

		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
			break;
		case BT_HCI_ROLE_SLAVE:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
	}
}

static void lp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.cu.instant = lp_event_counter(conn) + 6;
		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = LP_CU_STATE_WAIT_INSTANT;
	}
}

static void lp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_conn_param_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_tx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_conn_param_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_rx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *) param;

	switch (evt) {
	case LP_CU_EVT_CONN_PARAM_RSP:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_UNKNOWN:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *) param;

	switch (evt) {
	case LP_CU_EVT_CONN_UPDATE_IND:
		pdu_decode_conn_update_ind(ctx, param);
		ctx->state = LP_CU_STATE_WAIT_INSTANT;
		break;
	case LP_CU_EVT_UNKNOWN:
		ctx->data.cu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	uint16_t event_counter = lp_event_counter(conn);
	if (is_instant_reached_or_passed(ctx->data.cu.instant, event_counter)) {
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
		lp_cu_complete(conn, ctx, evt, param);
	}
}

static void lp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_CU_STATE_IDLE:
		lp_cu_st_idle(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ:
		lp_cu_st_wait_tx_conn_param_req(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP:
		lp_cu_st_wait_rx_conn_param_rsp(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		lp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		lp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_INSTANT:
		lp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_NTF:
		lp_cu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_CONN_PARAM_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_CONN_UPDATE_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_cu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_CU_STATE_IDLE;
}

void ull_cp_priv_lp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Connection Update FSM
 */

static uint16_t rp_event_counter(struct ll_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct lll_conn *lll;
	uint16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	uint16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void rp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		pdu_encode_conn_param_rsp(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		pdu_encode_conn_update_ind(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* TODO(thoh): Select between LL_REJECT_IND and LL_REJECT_EXT_IND */
		pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ, BT_HCI_ERR_UNACCEPT_CONN_PARAM);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_cu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_cu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CONN_UPDATE;
	pdu = (struct node_rx_cu *)ntf->pdu;

	pdu->status = ctx->data.cu.error;

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_cu_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;

	pdu_encode_conn_param_req(ctx, pdu);

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_NTF;
	} else {
		rp_cu_ntf(conn, ctx);
		rr_complete(conn);
		ctx->state = RP_CU_STATE_IDLE;
	}
}

static void rp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.cu.instant = rp_event_counter(conn) + 6;
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = RP_CU_STATE_WAIT_INSTANT;
	}
}

static void rp_cu_send_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_REJECT_IND;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_IND);
		rr_complete(conn);
		ctx->state = RP_CU_STATE_IDLE;
	}
}

static void rp_cu_send_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
		ctx->state = RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
	}
}

static void rp_cu_send_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ;
	} else {
		rp_cu_conn_param_req_ntf(conn, ctx);
		ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY;
	}
}

static void rp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_CU_EVT_RUN:
		ctx->state = RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_rx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ:
		rp_cu_send_conn_param_req_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_ntf_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_param_req_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ_REPLY:
		if (conn->lll.role == BT_HCI_ROLE_MASTER) {
			rp_cu_send_conn_update_ind(conn, ctx, evt, param);
		} else if (conn->lll.role == BT_HCI_ROLE_SLAVE) {
			rp_cu_send_conn_param_rsp(conn, ctx, evt, param);
		} else {
			LL_ASSERT(0);
		}
		break;
	case RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY:
		rp_cu_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_tx_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_tx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_param_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_UPDATE_IND:
		pdu_decode_conn_update_ind(ctx, param);
		ctx->state = RP_CU_STATE_WAIT_INSTANT;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	uint16_t event_counter = rp_event_counter(conn);
	if (((event_counter - ctx->data.cu.instant) & 0xFFFF) <= 0x7FFF) {
		ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
		rp_cu_complete(conn, ctx, evt, param);
	}
}

static void rp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_CU_STATE_IDLE:
		rp_cu_st_idle(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ:
		rp_cu_st_wait_rx_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ:
		rp_cu_state_wait_ntf_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY:
		rp_cu_state_wait_conn_param_req_reply(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_REJECT_IND:
		rp_cu_state_wait_tx_reject_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP:
		rp_cu_st_wait_tx_conn_param_rsp(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		rp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		rp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_INSTANT:
		rp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF:
		rp_cu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_UPDATE_IND, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_cu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_CU_STATE_IDLE;
}

void ull_cp_priv_rp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_RUN, param);
}

void ull_cp_priv_rp_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_REPLY, NULL);
}

void ull_cp_priv_rp_conn_param_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY, NULL);
}
