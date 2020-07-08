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
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_phy
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure PHY Update FSM states */
enum {
	LP_PU_STATE_IDLE,
	LP_PU_STATE_WAIT_TX_PHY_REQ,
	LP_PU_STATE_WAIT_RX_PHY_RSP,
	LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND,
	LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND,
	LP_PU_STATE_WAIT_INSTANT,
	LP_PU_STATE_WAIT_NTF,
};

/* LLCP Local Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	LP_PU_EVT_RUN,

	/* Response recieved */
	LP_PU_EVT_PHY_RSP,

	/* Indication recieved */
	LP_PU_EVT_PHY_UPDATE_IND,

	/* Reject response recieved */
	LP_PU_EVT_REJECT,

	/* Unknown response recieved */
	LP_PU_EVT_UNKNOWN,
};

/* LLCP Remote Procedure PHY Update FSM states */
enum {
	RP_PU_STATE_IDLE,
	RP_PU_STATE_WAIT_RX_PHY_REQ,
	RP_PU_STATE_WAIT_TX_PHY_RSP,
	RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND,
	RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND,
	RP_PU_STATE_WAIT_INSTANT,
	RP_PU_STATE_WAIT_NTF,
};

/* LLCP Remote Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	RP_PU_EVT_RUN,

	/* Request recieved */
	RP_PU_EVT_PHY_REQ,

	/* Indication recieved */
	RP_PU_EVT_PHY_UPDATE_IND,
};

/*
 * ULL -> LL Interface
 */

extern void ll_rx_enqueue(struct node_rx_pdu *rx);

/*
 * LLCP Local Procedure PHY Update FSM
 */

static u16_t lp_event_counter(struct ull_cp_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct mocked_lll_conn *lll;
	u16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	u16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void lp_pu_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		pdu_encode_phy_req(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pdu_encode_phy_update_ind(pdu, ctx->data.pu.instant);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_pu_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_pu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_PHY_UPDATE;
	pdu = (struct node_rx_pu *)ntf->pdu;

	pdu->status = ctx->data.pu.error;

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void lp_pu_complete(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = LP_PU_STATE_WAIT_NTF;
	} else {
		lp_pu_ntf(conn, ctx);
		lr_complete(conn);
		ctx->state = LP_PU_STATE_IDLE;
	}
}

static void lp_pu_send_phy_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available() || rr_get_collision(conn)) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_REQ;
	} else {
		rr_set_incompat(conn, INCOMPAT_RESOLVABLE);

		lp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_REQ);

		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
			break;
		case BT_HCI_ROLE_SLAVE:
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
	}
}

static void lp_pu_send_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.pu.instant = lp_event_counter(conn) + 6;
		lp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = LP_PU_STATE_WAIT_INSTANT;
	}
}

static void lp_pu_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_phy_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_rx_phy_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		if (conn->lll.role == BT_HCI_ROLE_SLAVE && rr_get_collision(conn)) {
			rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ctx->data.pu.error = BT_HCI_ERR_LL_PROC_COLLISION;
			lp_pu_complete(conn, ctx, evt, param);
		}
		break;
	case LP_PU_EVT_PHY_RSP:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		lp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_EVT_UNKNOWN:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_pu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_rx_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_PHY_UPDATE_IND:
		pdu_decode_phy_update_ind(ctx, param);
		ctx->state = LP_PU_STATE_WAIT_INSTANT;
		break;
	case LP_PU_EVT_REJECT:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_LL_PROC_COLLISION;
		lp_pu_complete(conn, ctx, evt, param);
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_check_instant(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	u16_t event_counter = lp_event_counter(conn);
	if (((event_counter - ctx->data.pu.instant) & 0xFFFF) <= 0x7FFF) {
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		lp_pu_complete(conn, ctx, evt, param);
	}
}

static void lp_pu_st_wait_instant(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_PU_STATE_IDLE:
		lp_pu_st_idle(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_PHY_REQ:
		lp_pu_st_wait_tx_phy_req(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_RX_PHY_RSP:
		lp_pu_st_wait_rx_phy_rsp(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		lp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		lp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_INSTANT:
		lp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_NTF:
		lp_pu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_PU_STATE_IDLE;
}

void ull_cp_priv_lp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure PHY Update FSM
 */

static u16_t rp_event_counter(struct ull_cp_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct mocked_lll_conn *lll;
	u16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	u16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void rp_pu_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		pdu_encode_phy_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pdu_encode_phy_update_ind(pdu, ctx->data.pu.instant);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_pu_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_pu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_PHY_UPDATE;
	pdu = (struct node_rx_pu *)ntf->pdu;

	pdu->status = ctx->data.pu.error;

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_pu_complete(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_NTF;
	} else {
		rp_pu_ntf(conn, ctx);
		rr_complete(conn);
		ctx->state = RP_PU_STATE_IDLE;
	}
}

static void rp_pu_send_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.pu.instant = rp_event_counter(conn) + 6;
		rp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = RP_PU_STATE_WAIT_INSTANT;
	}
}

static void rp_pu_send_phy_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_RSP;
	} else {
		rp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
		ctx->state = RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
	}
}

static void rp_pu_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_PU_EVT_RUN:
		ctx->state = RP_PU_STATE_WAIT_RX_PHY_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_rx_phy_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_PHY_REQ:
		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			rp_pu_send_phy_update_ind(conn, ctx, evt, param);
			break;
		case BT_HCI_ROLE_SLAVE:
			rp_pu_send_phy_rsp(conn, ctx, evt, param);
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_tx_phy_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_send_phy_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_tx_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_rx_phy_update_ind(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_PHY_UPDATE_IND:
		pdu_decode_phy_update_ind(ctx, param);
		ctx->state = RP_PU_STATE_WAIT_INSTANT;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_check_instant(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	u16_t event_counter = rp_event_counter(conn);
	if (((event_counter - ctx->data.pu.instant) & 0xFFFF) <= 0x7FFF) {
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		rp_pu_complete(conn, ctx, evt, param);
	}
}

static void rp_pu_st_wait_instant(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_PU_STATE_IDLE:
		rp_pu_st_idle(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_REQ:
		rp_pu_st_wait_rx_phy_req(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_PHY_RSP:
		rp_pu_st_wait_tx_phy_rsp(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		rp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		rp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_INSTANT:
		rp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_NTF:
		rp_pu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_pu_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_PU_STATE_IDLE;
}

void ull_cp_priv_rp_pu_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_RUN, param);
}
