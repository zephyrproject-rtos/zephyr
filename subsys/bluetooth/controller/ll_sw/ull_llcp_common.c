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
#include "ll_settings.h"

#include "lll.h"
#include "ll_feat.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_chan_internal.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_internal.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_common
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure FSM states */
enum {
	LP_COMMON_STATE_IDLE,
	LP_COMMON_STATE_WAIT_TX,
	LP_COMMON_STATE_WAIT_TX_ACK,
	LP_COMMON_STATE_WAIT_RX,
	LP_COMMON_STATE_WAIT_NTF,
};

/* LLCP Local Procedure Common FSM events */
enum {
	/* Procedure run */
	LP_COMMON_EVT_RUN,

	/* Response received */
	LP_COMMON_EVT_RESPONSE,

	/* Reject response received */
	LP_COMMON_EVT_REJECT,

	/* Unknown response received */
	LP_COMMON_EVT_UNKNOWN,

	/* Instant collision detected */
	LP_COMMON_EVT_COLLISION,

	/* Ack received */
	LP_COMMON_EVT_ACK,
};

/* LLCP Remote Procedure Common FSM states */
enum {
	RP_COMMON_STATE_IDLE,
	RP_COMMON_STATE_WAIT_RX,
	RP_COMMON_STATE_WAIT_TX,
	RP_COMMON_STATE_WAIT_TX_ACK,
	RP_COMMON_STATE_WAIT_NTF,
};
/* LLCP Remote Procedure Common FSM events */
enum {
	/* Procedure run */
	RP_COMMON_EVT_RUN,

	/* Ack received */
	RP_COMMON_EVT_ACK,

	/* Request received */
	RP_COMMON_EVT_REQUEST,
};

/*
 * LLCP Local Procedure Common FSM
 */

static void lp_comm_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_pdu_encode_ping_req(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_pdu_encode_feature_req(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_MIN_USED_CHANS:
		llcp_pdu_encode_min_used_chans_ind(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
	case PROC_VERSION_EXCHANGE:
		llcp_pdu_encode_version_ind(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		break;
	case PROC_TERMINATE:
		llcp_pdu_encode_terminate_ind(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_pdu_encode_length_req(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		llcp_pdu_encode_cte_req(ctx, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CTE_RSP;
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Update procedure timeout. For TERMINATE supervision_timeout is used */
	ull_conn_prt_reload(conn, (ctx->proc != PROC_TERMINATE) ? conn->procedure_reload :
									conn->supervision_reload);
}

static void lp_comm_ntf_feature_exchange(struct ll_conn *conn, struct proc_ctx *ctx,
					 struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		llcp_ntf_encode_feature_rsp(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG:
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
		/*
		 * No notification on feature-request or periph-feature request
		 * TODO: probably handle as an unexpected call
		 */
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		llcp_ntf_encode_unknown_rsp(ctx, pdu);
		break;
	default:
		/* EGON: TODO: define behaviour for unexpected PDU */
		LL_ASSERT(0);
	}
}

static void lp_comm_ntf_version_ind(struct ll_conn *conn, struct proc_ctx *ctx,
				    struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		llcp_ntf_encode_version_ind(conn, pdu);
		break;
	default:
		/* EGON: TODO: define behaviour for unexpected PDU */
		LL_ASSERT(0);
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void lp_comm_ntf_length_change(struct ll_conn *conn, struct proc_ctx *ctx,
				      struct pdu_data *pdu)
{
	llcp_ntf_encode_length_change(conn, pdu);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
static void lp_comm_ntf_cte_req(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* TODO (ppryga): pack IQ samples and send them to host */
	/* TODO (ppryga): procedure may be re-triggered periodically by controller itself.
	 *		  Add periodicy handling code. It should be executed after receive
	 *		  notification about end of current procedure run.
	 */
	/* TODO (ppryga): Add handling of rejections in HCI: HCI_LE_CTE_Request_Failed. */
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_CTE_RSP:
		llcp_ntf_encode_cte_req(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_ntf_encode_reject_ext_ind(ctx, pdu);
		break;
	default:
		/* TODO (ppryga): Update when behavior for unexpected PDU is defined */
		LL_ASSERT(0);
	}
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

static void lp_comm_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;

	switch (ctx->proc) {
	case PROC_FEATURE_EXCHANGE:
		lp_comm_ntf_feature_exchange(conn, ctx, pdu);
		break;
	case PROC_VERSION_EXCHANGE:
		lp_comm_ntf_version_ind(conn, ctx, pdu);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		lp_comm_ntf_length_change(conn, ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		lp_comm_ntf_cte_req(conn, ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void lp_comm_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP ||
		    ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_PING_RSP) {
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		} else {
			/* Illegal response opcode */
			LL_ASSERT(0);
		}
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		if (!llcp_ntf_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_NTF;
		} else {
			lp_comm_ntf(conn, ctx);
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_MIN_USED_CHANS:
		llcp_lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
	case PROC_VERSION_EXCHANGE:
		if (!llcp_ntf_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_NTF;
		} else {
			lp_comm_ntf(conn, ctx);
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_TERMINATE:
		/* No notification */
		llcp_lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;

		/* Mark the connection for termination */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LOCALHOST_TERM_CONN;
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		if (ctx->response_opcode != PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) {
			/* Apply changes in data lengths/times */
			uint8_t dle_changed = ull_dle_update_eff(conn);

			if (dle_changed && !llcp_ntf_alloc_is_available()) {
				/* We need to generate NTF but no buffers avail so wait for one */
				ctx->state = LP_COMMON_STATE_WAIT_NTF;
			} else {
				if (dle_changed) {
					lp_comm_ntf(conn, ctx);
				}
				llcp_lr_complete(conn);
				ctx->state = LP_COMMON_STATE_IDLE;
			}
		} else {
			/* Peer does not accept DLU, so disable on current connection */
			feature_unmask_features(conn, LL_FEAT_BIT_DLE);

			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}

		if (!ull_cp_remote_dle_pending(conn)) {
			/* Resume data, but only if there is no remote procedure pending RSP
			 * in which case, the RSP tx-ACK will resume data
			 */
			llcp_tx_resume_data(conn);
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_CTE_RSP ||
		    (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND &&
		     ctx->reject_ext_ind.reject_opcode == PDU_DATA_LLCTRL_TYPE_CTE_REQ)) {
			lp_comm_ntf(conn, ctx);
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_send_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			conn->llcp.fex.sent = 1;
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_MIN_USED_CHANS:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of
		 * one LL_VERSION_IND PDU during a connection.
		 */
		if (!conn->llcp.vex.sent) {
			if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
				ctx->state = LP_COMMON_STATE_WAIT_TX;
			} else {
				lp_comm_tx(conn, ctx);
				conn->llcp.vex.sent = 1;
				ctx->state = LP_COMMON_STATE_WAIT_RX;
			}
		} else {
			ctx->response_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
			lp_comm_complete(conn, ctx, evt, param);
		}
		break;
	case PROC_TERMINATE:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		if (!ull_cp_remote_dle_pending(conn)) {
			if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
				ctx->state = LP_COMMON_STATE_WAIT_TX;
			} else {
				/* Pause data tx, to ensure we can later (on RSP rx-ack)
				 * update DLE without conflicting with out-going LL Data PDUs
				 * See BT Core 5.2 Vol6: B-4.5.10 & B-5.1.9
				 */
				llcp_tx_pause_data(conn);
				lp_comm_tx(conn, ctx);
				ctx->state = LP_COMMON_STATE_WAIT_RX;
			}
		} else {
			/* REQ was received from peer and RSP not yet sent
			 * lets piggy-back on RSP instead af sending REQ
			 * thus we can complete local req
			 */
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RUN:
		if (ctx->pause) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_send_req(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RUN:
		lp_comm_send_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_ACK:
		switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
		case PROC_MIN_USED_CHANS:
			ctx->tx_ack = NULL;
			lp_comm_complete(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
		case PROC_TERMINATE:
			ctx->tx_ack = NULL;
			lp_comm_complete(conn, ctx, evt, param);
			break;
		default:
			/* Ignore for other procedures */
			break;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
	/* TODO */
}

static void lp_comm_rx_decode(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->response_opcode = pdu->llctrl.opcode;

	switch (pdu->llctrl.opcode) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PDU_DATA_LLCTRL_TYPE_PING_RSP:
		/* ping_rsp has no data */
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		llcp_pdu_decode_feature_rsp(conn, pdu);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN)
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		/* No response expected */
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN */
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		llcp_pdu_decode_version_ind(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		llcp_pdu_decode_unknown_rsp(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		/* No response expected */
		LL_ASSERT(0);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		llcp_pdu_decode_length_rsp(conn, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	case PDU_DATA_LLCTRL_TYPE_CTE_RSP:
		/* CTE Response PDU had no data */
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_pdu_decode_reject_ext_ind(ctx, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_wait_rx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RESPONSE:
		lp_comm_rx_decode(conn, ctx, (struct pdu_data *)param);
		lp_comm_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	/* TODO */
	switch (evt) {
	case LP_COMMON_EVT_RUN:
		switch (ctx->proc) {
		case PROC_FEATURE_EXCHANGE:
		case PROC_VERSION_EXCHANGE:
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		case PROC_DATA_LENGTH_UPDATE:
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
			if (llcp_ntf_alloc_is_available()) {
				lp_comm_ntf(conn, ctx);
				llcp_lr_complete(conn);
				ctx->state = LP_COMMON_STATE_IDLE;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void lp_comm_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case LP_COMMON_STATE_IDLE:
		lp_comm_st_idle(conn, ctx, evt, param);
		break;
	case LP_COMMON_STATE_WAIT_TX:
		lp_comm_st_wait_tx(conn, ctx, evt, param);
		break;
	case LP_COMMON_STATE_WAIT_TX_ACK:
		lp_comm_st_wait_tx_ack(conn, ctx, evt, param);
		break;
	case LP_COMMON_STATE_WAIT_RX:
		lp_comm_st_wait_rx(conn, ctx, evt, param);
		break;
	case LP_COMMON_STATE_WAIT_NTF:
		lp_comm_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_lp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_ACK, tx->pdu);
}

void llcp_lp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_RESPONSE, rx->pdu);
}

void llcp_lp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_COMMON_STATE_IDLE;
}

void llcp_lp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Common FSM
 */
static void rp_comm_rx_decode(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ctx->response_opcode = pdu->llctrl.opcode;

	switch (pdu->llctrl.opcode) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PDU_DATA_LLCTRL_TYPE_PING_REQ:
		/* ping_req has no data */
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG:
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_CENTRAL */
		llcp_pdu_decode_feature_req(conn, pdu);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		llcp_pdu_decode_min_used_chans_ind(conn, pdu);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_CENTRAL */
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		llcp_pdu_decode_version_ind(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		llcp_pdu_decode_terminate_ind(ctx, pdu);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		llcp_pdu_decode_length_req(conn, pdu);
		/* On reception of REQ mark RSP open for local piggy-back
		 * Pause data tx, to ensure we can later (on RSP tx ack) update DLE without
		 * conflicting with out-going LL Data PDUs
		 * See BT Core 5.2 Vol6: B-4.5.10 & B-5.1.9
		 */
		llcp_tx_pause_data(conn);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PDU_DATA_LLCTRL_TYPE_CTE_REQ:
		llcp_pdu_decode_cte_req(conn, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

static void rp_comm_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		llcp_pdu_encode_ping_rsp(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_pdu_encode_feature_rsp(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
		break;
	case PROC_VERSION_EXCHANGE:
		llcp_pdu_encode_version_ind(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_pdu_encode_length_rsp(conn, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ: {
		uint8_t err_code = 0;

		if (conn->llcp.cte_rsp.is_enabled == 0) {
			err_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
		}
#if defined(CONFIG_BT_PHY_UPDATE)
		/* If the PHY update is not possible, then PHY1M is used.
		 * CTE is supported for PHY1M.
		 */
		if (conn->lll.phy_tx != PHY_CODED) {
			err_code = BT_HCI_ERR_INVALID_LL_PARAM;
		}
#endif /* CONFIG_BT_PHY_UPDATE */
		if (!(conn->llcp.cte_rsp.cte_types & BIT(conn->llcp.cte_req.cte_type)) &&
		    conn->llcp.cte_rsp.max_cte_len >= conn->llcp.cte_req.min_cte_len) {
			err_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
		}

		if (!err_code) {
			llcp_pdu_encode_cte_rsp(pdu);
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CTE_RSP;
		} else {
			llcp_pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_CTE_REQ, err_code);
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
		}
		break;
	}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void rp_comm_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_RUN:
		ctx->state = RP_COMMON_STATE_WAIT_RX;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void rp_comm_ntf_length_change(struct ll_conn *conn, struct proc_ctx *ctx,
				      struct pdu_data *pdu)
{
	llcp_ntf_encode_length_change(conn, pdu);
}

static void rp_comm_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	ARG_UNUSED(pdu);
	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;
	switch (ctx->proc) {
/* Note: the 'double' ifdef in case this switch case expands
 * in the future and the function is re-instated
 */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		rp_comm_ntf_length_change(conn, ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static void rp_comm_send_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		/* Always respond on remote ping */
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		/* Always respond on remote feature exchange */
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			conn->llcp.fex.sent = 1;
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of one
		 * LL_VERSION_IND PDU during a connection.
		 */
		if (!conn->llcp.vex.sent) {
			if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
				ctx->state = RP_COMMON_STATE_WAIT_TX;
			} else {
				rp_comm_tx(conn, ctx);
				conn->llcp.vex.sent = 1;
				llcp_rr_complete(conn);
				ctx->state = RP_COMMON_STATE_IDLE;
			}
		} else {
			/* Protocol Error.
			 *
			 * A procedure already sent a LL_VERSION_IND and received a LL_VERSION_IND.
			 */
			/* TODO */
			LL_ASSERT(0);
		}
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_CENTRAL)
	case PROC_MIN_USED_CHANS:
		/*
		 * Spec says (5.2, Vol.6, Part B, Section 5.1.11):
		 *     The procedure has completed when the Link Layer acknowledgment of the
		 *     LL_MIN_USED_CHANNELS_IND PDU is sent or received.
		 * In effect, for this procedure, this is equivalent to RX of PDU
		 */
		/* Inititate a chmap update, but only if acting as central, just in case ... */
		if (conn->lll.role == BT_HCI_ROLE_CENTRAL &&
		    ull_conn_lll_phy_active(conn, conn->llcp.muc.phys)) {
			uint8_t chmap[5];

			ull_chan_map_get((uint8_t *const)chmap);
			ull_cp_chan_map_update(conn, chmap);
			/* TODO - what to do on failure of ull_cp_chan_map_update() */
		}
		/* No response */
		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_CENTRAL */
	case PROC_TERMINATE:
		/* No response */
		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;

		/* Mark the connection for termination */
		conn->llcp_terminate.reason_final = ctx->data.term.error_code;
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			/* On RSP tx close the window for possible local req piggy-back */
			rp_comm_tx(conn, ctx);

			/* Wait for the peer to have ack'ed the RSP before updating DLE */
			ctx->state = RP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		if (ctx->pause || !llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void rp_comm_st_wait_rx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_REQUEST:
		rp_comm_rx_decode(conn, ctx, (struct pdu_data *)param);
		rp_comm_send_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_comm_st_wait_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RUN:
		rp_comm_send_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void rp_comm_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_ACK:
		switch (ctx->proc) {
		case PROC_DATA_LENGTH_UPDATE: {
			/* Apply changes in data lengths/times */
			uint8_t dle_changed = ull_dle_update_eff(conn);

			llcp_tx_resume_data(conn);

			if (dle_changed && !llcp_ntf_alloc_is_available()) {
				ctx->state = RP_COMMON_STATE_WAIT_NTF;
			} else {
				if (dle_changed) {
					rp_comm_ntf(conn, ctx);
				}
				llcp_rr_complete(conn);
				ctx->state = RP_COMMON_STATE_IDLE;
			}
			break;
		}
		default:
			/* Ignore other procedures */
			break;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_comm_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	if (llcp_ntf_alloc_is_available()) {
		rp_comm_ntf(conn, ctx);
		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;
	}
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static void rp_comm_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case RP_COMMON_STATE_IDLE:
		rp_comm_st_idle(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_RX:
		rp_comm_st_wait_rx(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_TX:
		rp_comm_st_wait_tx(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case RP_COMMON_STATE_WAIT_TX_ACK:
		rp_comm_st_wait_tx_ack(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_NTF:
		rp_comm_st_wait_ntf(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_REQUEST, rx->pdu);
}

void llcp_rp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_ACK, tx->pdu);
}

void llcp_rp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_COMMON_STATE_IDLE;
}

void llcp_rp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_RUN, param);
}
