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
#include "ull_conn_llcp_internal.h"
#include "ull_llcp_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_common
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#include "helper_pdu.h"
#include "helper_util.h"

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

	/* Response recieved */
	LP_COMMON_EVT_RESPONSE,

	/* Reject response recieved */
	LP_COMMON_EVT_REJECT,

	/* Unknown response recieved */
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

	/* Request recieved */
	RP_COMMON_EVT_REQUEST,
};

/*
 * ULL -> LL Interface
 */

extern void ll_rx_enqueue(struct node_rx_pdu *rx);

/*
 * LLCP Local Procedure Common FSM
 */

static void lp_comm_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (ctx->proc) {
	case PROC_LE_PING:
		pdu_encode_ping_req(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PING_RSP;
		break;
	case PROC_FEATURE_EXCHANGE:
		pdu_encode_feature_req(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
		break;
	case PROC_MIN_USED_CHANS:
		pdu_encode_min_used_chans_ind(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
	case PROC_VERSION_EXCHANGE:
		pdu_encode_version_ind(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		break;
	case PROC_TERMINATE:
		pdu_encode_terminate_ind(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
	case PROC_DATA_LENGTH_UPDATE:
		pdu_encode_length_req(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_comm_ntf_feature_exchange(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		ntf_encode_feature_rsp(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
		/*
		 * No notification on feature-request or slave-feature request
		 * TODO: probably handle as an unexpected call
		 */
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		ntf_encode_unknown_rsp(ctx, pdu);
		break;
	default:
		/* EGON: TODO: define behaviour for unexpected PDU */
		LL_ASSERT(0);
	}
}

static void lp_comm_ntf_version_ind(struct ll_conn *conn,  struct proc_ctx *ctx, struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		ntf_encode_version_ind(conn, pdu);
		break;
	default:
		/* EGON: TODO: define behaviour for unexpected PDU */
		LL_ASSERT(0);
	}
}

static void lp_comm_ntf_length_change(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ntf_encode_length_change(conn, pdu);
}

static void lp_comm_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;
	switch (ctx->proc) {
	case PROC_FEATURE_EXCHANGE:
		lp_comm_ntf_feature_exchange(conn, ctx, pdu);
		break;
	case PROC_VERSION_EXCHANGE:
		lp_comm_ntf_version_ind(conn, ctx, pdu);
		break;
	case PROC_DATA_LENGTH_UPDATE:
		lp_comm_ntf_length_change(conn, ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void lp_comm_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP || ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_PING_RSP) {
			lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		else
		{
			/* Illegal response opcode */
			LL_ASSERT(0);
		}
		break;
	case PROC_FEATURE_EXCHANGE:
		if (!ntf_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_NTF;
		} else {
			lp_comm_ntf(conn, ctx);
			lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_MIN_USED_CHANS:
		/* Store the MUC parameters with conn on completion */
		conn->llcp.muc.min_used_chans = ctx->data.muc.min_used_chans;
		conn->llcp.muc.phys = ctx->data.muc.phys;
		lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;
		break;
	case PROC_VERSION_EXCHANGE:
		if (!ntf_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_NTF;
		} else {
			lp_comm_ntf(conn, ctx);
			lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_TERMINATE:
		/* No notification */
		lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;

		/* Mark the connection for termination */
		conn->terminate.reason = BT_HCI_ERR_LOCALHOST_TERM_CONN;
		break;
	case PROC_DATA_LENGTH_UPDATE:
		if (ctx->response_opcode != PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) {
			if (conn->llcp.dlu.dle_changed && !ntf_alloc_is_available()) {
				ctx->state = LP_COMMON_STATE_WAIT_NTF;
			} else {
				// Notify only if data length parameters changed
				if (conn->llcp.dlu.dle_changed) {
					lp_comm_ntf(conn, ctx);
				}
				lr_complete(conn);
				ctx->state = LP_COMMON_STATE_IDLE;
			}
		} else {
			// Peer does not accept DLU, so disable locally
			conn->llcp.dlu.disabled = 1;
		}
		tx_resume_data(conn);
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_send_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		if (!tx_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
	case PROC_FEATURE_EXCHANGE:
		if (!tx_alloc_is_available() || ctx->pause) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			conn->llcp.fex.sent = 1;
			ctx->state = LP_COMMON_STATE_WAIT_RX;
		}
		break;
	case PROC_MIN_USED_CHANS:
		if (!tx_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of one LL_VERSION_IND PDU during a connection. */
		if (!conn->llcp.vex.sent) {
			if (!tx_alloc_is_available()) {
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
		if (!tx_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
	case PROC_DATA_LENGTH_UPDATE:
		// Flush data path before starting the procedure
		if (!conn->llcp.dlu.rsp_open) {
			// Pause data tx, to ensure we can update DLE without conflicting with out-going data
			tx_pause_data(conn);
			if (!tx_alloc_is_available() || !ull_conn_tx_data_path_flushed(conn)) {
				ctx->state = LP_COMMON_STATE_WAIT_TX;
			} else {
				lp_comm_tx(conn, ctx);
				ctx->state = LP_COMMON_STATE_WAIT_RX;
			}
		} else {
			// REQ was received from peer and RSP not yet sent
			// so we will piggy-back on RSP and not send REQ
		}

		break;
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
		switch (ctx->proc) {
		case PROC_DATA_LENGTH_UPDATE:
			if (!ctx->pause && !tx_alloc_is_available() && !ull_conn_tx_data_path_flushed(conn)) {
				lp_comm_send_req(conn, ctx, evt, param);
			}
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
}

static void lp_comm_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_ACK:
		switch (ctx->proc) {
		case PROC_MIN_USED_CHANS:
			ctx->tx_ack = NULL;
			lp_comm_complete(conn, ctx, evt, param);
			break;
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
	case PDU_DATA_LLCTRL_TYPE_PING_RSP:
		/* ping_rsp has no data */
		break;
	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		pdu_decode_feature_rsp(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		pdu_decode_feature_req(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		/* No response expected */
		break;
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		pdu_decode_version_ind(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		pdu_decode_unknown_rsp(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		/* No response expected */
		LL_ASSERT(0);
		break;
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		pdu_decode_length_rsp(conn, pdu);
		// Apply the length update in LL
		ll_apply_remote_dlu(conn);
		// And resume data tx
		tx_resume_data(conn);
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
		lp_comm_rx_decode(conn, ctx, (struct pdu_data *) param);
		lp_comm_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
}

static void lp_comm_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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
void ull_cp_priv_lp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_ACK, tx->pdu);
}

void ull_cp_priv_lp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_RESPONSE, rx->pdu);
}

void ull_cp_priv_lp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_COMMON_STATE_IDLE;
}

void ull_cp_priv_lp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
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
	case PDU_DATA_LLCTRL_TYPE_PING_REQ:
		/* ping_req has no data */
		break;
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		pdu_decode_feature_req(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
		pdu_decode_min_used_chans_ind(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		pdu_decode_version_ind(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
		pdu_decode_terminate_ind(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		pdu_decode_length_req(conn, pdu);
		// On reception of REQ mark RSP open for input
		conn->llcp.dlu.rsp_open = 1;
		// And pause data tx to make sure we can change the DLE
		// parameters without conflicting with outgoing data
		tx_pause_data(conn);
		break;
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
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (ctx->proc) {
	case PROC_LE_PING:
		pdu_encode_ping_rsp(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
		break;
	case PROC_FEATURE_EXCHANGE:
		pdu_encode_feature_rsp(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_FEATURE_RSP;
		break;
	case PROC_VERSION_EXCHANGE:
		pdu_encode_version_ind(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
		break;
	case PROC_DATA_LENGTH_UPDATE:
		pdu_encode_length_rsp(conn, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_LENGTH_RSP;
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
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

static void rp_comm_ntf_length_change(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	ntf_encode_length_change(conn, pdu);
}

static void rp_comm_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	pdu = (struct pdu_data *) ntf->pdu;
	switch (ctx->proc) {
	case PROC_DATA_LENGTH_UPDATE:
		rp_comm_ntf_length_change(conn, ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void rp_comm_send_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		/* Always respond on remote ping */
		if (!tx_alloc_is_available()) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_FEATURE_EXCHANGE:
		/* Always respond on remote feature exchange */
		if (!tx_alloc_is_available()) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			conn->llcp.fex.sent = 1;
			rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of one LL_VERSION_IND PDU during a connection. */
		if (!conn->llcp.vex.sent) {
			if (!tx_alloc_is_available()) {
				ctx->state = RP_COMMON_STATE_WAIT_TX;
			} else {
				rp_comm_tx(conn, ctx);
				conn->llcp.vex.sent = 1;
				rr_complete(conn);
				ctx->state = RP_COMMON_STATE_IDLE;
			}
		} else {
			/* Protocol Error.
			 *
			 * A procedure already sent a LL_VERSION_IND and recieved a LL_VERSION_IND.
			 */
			/* TODO */
			LL_ASSERT(0);
		}
		break;
	case PROC_MIN_USED_CHANS:
		/* No response */
		ctx->state = RP_COMMON_STATE_IDLE;
		break;
	case PROC_TERMINATE:
		/* No response */
		rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;

		/* Mark the connection for termination */
		conn->terminate.reason = ctx->data.term.error_code;
		break;
	case PROC_DATA_LENGTH_UPDATE:
		if (!tx_alloc_is_available() || !ull_conn_tx_data_path_flushed(conn)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {

			// On RSP tx close the window for possible local req piggy-back
			conn->llcp.dlu.rsp_open = 0;
			rp_comm_tx(conn, ctx);

			// Wait for the peer to have received the RSP before updating DLE
			ctx->state = RP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void rp_comm_st_wait_rx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_REQUEST:
		rp_comm_rx_decode(conn, ctx, (struct pdu_data *) param);
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
		switch (ctx->proc) {
		case PROC_DATA_LENGTH_UPDATE:
			if (!ctx->pause && !tx_alloc_is_available() && !ull_conn_tx_data_path_flushed(conn)) {
				rp_comm_send_rsp(conn, ctx, evt, param);
			}
			break;
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

static void rp_comm_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_ACK:
		switch (ctx->proc) {
		case PROC_DATA_LENGTH_UPDATE:
			// Apply the length update in LL
			ll_apply_remote_dlu(conn);
			// And resume data tx
			tx_resume_data(conn);

			// Notify only if data length parameters changed
			if (conn->llcp.dlu.dle_changed) {
				if (!ntf_alloc_is_available()) {
					ctx->state = RP_COMMON_STATE_WAIT_NTF;
					break;
				} else {
					rp_comm_ntf(conn, ctx);
				}
			}
			rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
			break;
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

static void rp_comm_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
}

static void rp_comm_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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
	case RP_COMMON_STATE_WAIT_TX_ACK:
		rp_comm_st_wait_tx_ack(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_NTF:
		rp_comm_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_comm_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_REQUEST, rx->pdu);
}

void ull_cp_priv_rp_comm_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_ACK, tx->pdu);
}

void ull_cp_priv_rp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_COMMON_STATE_IDLE;
}

void ull_cp_priv_rp_comm_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_RUN, param);
}
