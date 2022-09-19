/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "hal/ecb.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_llcp_features.h"
#include "ull_conn_internal.h"

#include "ull_iso_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_cis
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_PERIPHERAL)
static uint16_t cc_event_counter(struct ll_conn *conn)
{
	struct lll_conn *lll;
	uint16_t event_counter;

	uint16_t lazy = conn->llcp.prep.lazy;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

/* LLCP Remote Procedure FSM states */
enum {
	/* Establish Procedure */
	RP_CC_STATE_IDLE,
	RP_CC_STATE_WAIT_RX_CIS_REQ,
	RP_CC_STATE_WAIT_NTF_CIS_CREATE,
	RP_CC_STATE_WAIT_REPLY,
	RP_CC_STATE_WAIT_TX_CIS_RSP,
	RP_CC_STATE_WAIT_TX_REJECT_IND,
	RP_CC_STATE_WAIT_RX_CIS_IND,
	RP_CC_STATE_WAIT_INSTANT,
	RP_CC_STATE_WAIT_CIS_ESTABLISHED,
	RP_CC_STATE_WAIT_NTF,
};

/* LLCP Remote Procedure FSM events */
enum {
	/* Procedure prepared */
	RP_CC_EVT_RUN,

	/* Request received */
	RP_CC_EVT_CIS_REQ,

	/* Response received */
	RP_CC_EVT_CIS_RSP,

	/* Indication received */
	RP_CC_EVT_CIS_IND,

	/* Create request accept reply */
	RP_CC_EVT_CIS_REQ_ACCEPT,

	/* Create request decline reply */
	RP_CC_EVT_CIS_REQ_REJECT,

	/* Reject response received */
	RP_CC_EVT_REJECT,

	/* Unknown response received */
	RP_CC_EVT_UNKNOWN,
};
/*
 * LLCP Remote Procedure FSM
 */

static void llcp_rp_cc_tx_rsp(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	llcp_pdu_encode_cis_rsp(ctx, pdu);
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void llcp_rp_cc_tx_reject(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	llcp_pdu_encode_reject_ext_ind(pdu, opcode, ctx->data.cis_create.error);
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void rp_cc_ntf_create(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CIS_REQUEST;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_conn_iso_req *)ntf->pdu;

	pdu->cig_id = ctx->data.cis_create.cig_id;
	pdu->cis_id = ctx->data.cis_create.cis_id;
	pdu->cis_handle = ctx->data.cis_create.cis_handle;

	ctx->data.cis_create.host_request_to = 0U;

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}


static void rp_cc_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_estab *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CIS_ESTABLISHED;
	ntf->hdr.handle = conn->lll.handle;
	ntf->hdr.rx_ftr.param = ll_conn_iso_stream_get(ctx->data.cis_create.cis_handle);

	pdu = (struct node_rx_conn_iso_estab *)ntf->pdu;

	pdu->cis_handle = ctx->data.cis_create.cis_handle;
	pdu->status = ctx->data.cis_create.error;

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void rp_cc_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = RP_CC_STATE_WAIT_NTF;
	} else {
		rp_cc_ntf(conn, ctx);
		llcp_rr_complete(conn);
		ctx->state = RP_CC_STATE_IDLE;
	}
}

static void rp_cc_send_cis_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CC_STATE_WAIT_TX_CIS_RSP;
	} else {
		llcp_rp_cc_tx_rsp(conn, ctx);

		/* Wait for the LL_CIS_IND */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CIS_IND;
		ctx->state = RP_CC_STATE_WAIT_RX_CIS_IND;
	}
}

static void rp_cc_send_create_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = RP_CC_STATE_WAIT_NTF_CIS_CREATE;
	} else {
		rp_cc_ntf_create(conn, ctx);
		ctx->state = RP_CC_STATE_WAIT_REPLY;
	}
}

static void rp_cc_send_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CC_STATE_WAIT_TX_REJECT_IND;
	} else {
		llcp_rp_cc_tx_reject(conn, ctx, PDU_DATA_LLCTRL_TYPE_CIS_REQ);

		if (ctx->data.cis_create.error == BT_HCI_ERR_CONN_ACCEPT_TIMEOUT) {
			/* We reject due to an accept timeout, so we should generate NTF */
			rp_cc_complete(conn, ctx, evt, param);
		} else {
			/* Otherwise we quietly complete the procedure */
			llcp_rr_complete(conn);
			ctx->state = RP_CC_STATE_IDLE;
		}
	}
}

static void rp_cc_state_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			     void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		ctx->state = RP_CC_STATE_WAIT_RX_CIS_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static inline bool phy_valid(uint8_t phy)
{
	/* This is equivalent to:
	 * exactly one bit set, and no bit set is rfu's
	 */
	return (phy == PHY_1M || phy == PHY_2M || phy == PHY_CODED);
}

static uint8_t rp_cc_check_phy(struct ll_conn *conn, struct proc_ctx *ctx,
					    struct pdu_data *pdu)
{
	if (!phy_valid(pdu->llctrl.cis_req.c_phy) ||
	    !phy_valid(pdu->llctrl.cis_req.p_phy)) {
		/* zero, more than one or any rfu bit selected in either phy */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

#if defined(CONFIG_BT_CTLR_PHY)
	const uint8_t phys = pdu->llctrl.cis_req.p_phy | pdu->llctrl.cis_req.c_phy;

	if (((phys & PHY_2M) && !feature_phy_2m(conn)) ||
	    ((phys & PHY_CODED) && !feature_phy_coded(conn))) {
		/* Unsupported phy selected */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}
#endif /* CONFIG_BT_CTLR_PHY */

	return BT_HCI_ERR_SUCCESS;
}

static bool rp_cc_check_cis_established_lll(struct proc_ctx *ctx)
{
	const struct ll_conn_iso_stream *cis =
		ll_conn_iso_stream_get(ctx->data.cis_create.cis_handle);

	return cis->established;
}

static void rp_cc_state_wait_rx_cis_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case RP_CC_EVT_CIS_REQ:
		/* Handle CIS request */
		llcp_pdu_decode_cis_req(ctx, pdu);

		/* Check PHY */
		ctx->data.cis_create.error = rp_cc_check_phy(conn, ctx, pdu);

		if (ctx->data.cis_create.error == BT_HCI_ERR_SUCCESS) {
			ctx->data.cis_create.error =
				ull_peripheral_iso_acquire(conn, &pdu->llctrl.cis_req,
							   &ctx->data.cis_create.cis_handle);
		}

		if (ctx->data.cis_create.error == BT_HCI_ERR_SUCCESS) {
			/* Now controller accepts, so go ask the host to accept or decline */
			rp_cc_send_create_ntf(conn, ctx, evt, param);
		} else {
			/* Now controller rejects, right out */
			rp_cc_send_reject_ind(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_tx_cis_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_send_cis_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_tx_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_rx_cis_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case RP_CC_EVT_CIS_IND:
		if (!ull_peripheral_iso_setup(&pdu->llctrl.cis_ind, ctx->data.cis_create.cig_id,
					 ctx->data.cis_create.cis_handle)) {

			/* CIS has been setup, go wait for 'instant' before starting */
			ctx->state = RP_CC_STATE_WAIT_INSTANT;

			/* Fixme - Implement CIS Supervision timeout
			 * Spec:
			 * When establishing a CIS, the Peripheral shall start the CIS supervision
			 * timer at the start of the next CIS event after receiving the LL_CIS_IND.
			 * If the CIS supervision timer reaches 6 * ISO_Interval before the CIS is
			 * established, the CIS shall be considered lost.
			 */

			break;
		}
		/* If we get to here the CIG_ID referred in req/acquire has become void/invalid */
		/* This cannot happen unless the universe has started to deflate */
		LL_ASSERT(0);
	case RP_CC_EVT_REJECT:
		/* Handle CIS creation rejection */
		break;

	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_ntf_cis_create(struct ll_conn *conn, struct proc_ctx *ctx,
					    uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_send_create_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_state_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				 void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	uint16_t start_event_count;

	start_event_count = ctx->data.cis_create.conn_event_count;

	if (is_instant_reached_or_passed(start_event_count,
					 cc_event_counter(conn))) {
		/* Start CIS */
		ull_peripheral_iso_start(conn, conn->llcp.prep.ticks_at_expire,
					 ctx->data.cis_create.cis_handle);

		/* Now we can wait for CIS to become established */
		ctx->state = RP_CC_STATE_WAIT_CIS_ESTABLISHED;
	}
}

static void rp_cc_state_wait_reply(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{

	switch (evt) {
	case RP_CC_EVT_CIS_REQ_ACCEPT:
		/* Continue procedure in next prepare run */
		ctx->state = RP_CC_STATE_WAIT_TX_CIS_RSP;
		break;
	case RP_CC_EVT_RUN:
		/* Update 'time' and check for timeout on the Reply */
		ctx->data.cis_create.host_request_to += (conn->lll.interval * CONN_INT_UNIT_US);
		if (ctx->data.cis_create.host_request_to < conn->connect_accept_to) {
			break;
		}
		/* Reject 'reason/error' */
		ctx->data.cis_create.error = BT_HCI_ERR_CONN_ACCEPT_TIMEOUT;
		/* If timeout is hit, fall through and reject */
	case RP_CC_EVT_CIS_REQ_REJECT:
		/* Continue procedure in next prepare run */
		ctx->state = RP_CC_STATE_WAIT_TX_REJECT_IND;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_state_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_state_wait_cis_established(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		/* Check for CIS state */
		if (rp_cc_check_cis_established_lll(ctx)) {
			/* CIS was established, so let's got ahead and complete procedure */
			rp_cc_complete(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	/* Create Procedure */
	case RP_CC_STATE_IDLE:
		rp_cc_state_idle(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_RX_CIS_REQ:
		rp_cc_state_wait_rx_cis_req(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_NTF_CIS_CREATE:
		rp_cc_state_wait_ntf_cis_create(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_TX_REJECT_IND:
		rp_cc_state_wait_tx_reject_ind(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_TX_CIS_RSP:
		rp_cc_state_wait_tx_cis_rsp(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_REPLY:
		rp_cc_state_wait_reply(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_RX_CIS_IND:
		rp_cc_state_wait_rx_cis_ind(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_INSTANT:
		rp_cc_state_wait_instant(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_CIS_ESTABLISHED:
		rp_cc_state_wait_cis_established(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_NTF:
		rp_cc_state_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_cc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CIS_REQ:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CIS_IND:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void llcp_rp_cc_init_proc(struct proc_ctx *ctx)
{
	switch (ctx->proc) {
	case PROC_CIS_CREATE:
		ctx->state = RP_CC_STATE_IDLE;
		break;
	default:
		LL_ASSERT(0);
	}
}

bool llcp_rp_cc_awaiting_reply(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CC_STATE_WAIT_REPLY);
}

void llcp_rp_cc_accept(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ_ACCEPT, NULL);
}

void llcp_rp_cc_reject(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ_REJECT, NULL);
}

void llcp_rp_cc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_RUN, param);
}
#endif /* CONFIG_BT_PERIPHERAL */
