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
	RP_COMMON_STATE_WAIT_NTF,
};
/* LLCP Remote Procedure Common FSM events */
enum {
	/* Procedure run */
	RP_COMMON_EVT_RUN,

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

static void lp_comm_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx)
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
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_comm_ntf_feature_exchange(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
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

static void lp_comm_ntf_version_ind(struct ull_cp_conn *conn,  struct proc_ctx *ctx, struct pdu_data *pdu)
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

static void lp_comm_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx)
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
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_enqueue(ntf);
}

static void lp_comm_complete(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
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
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_send_req(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		if (!tx_alloc_is_available() || ctx->pause) {
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
		if (!tx_alloc_is_available() || ctx->pause) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of one LL_VERSION_IND PDU during a connection. */
		if (!conn->llcp.vex.sent) {
			if (!tx_alloc_is_available() || ctx->pause) {
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
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
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

static void lp_comm_st_wait_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
}

static void lp_comm_st_wait_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_ACK:
		switch (ctx->proc) {
		case PROC_MIN_USED_CHANS:
			ctx->tx_ack = NULL;
			lp_comm_complete(conn, ctx, evt, param);
			break;
		default:
			/* Ignore for other procedures */
			break;
		}
	default:
		/* Ignore other evts */
		break;
	}
	/* TODO */
}

static void lp_comm_rx_decode(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
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
	default:

		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_wait_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RESPONSE:
		lp_comm_rx_decode(conn, ctx, (struct pdu_data *) param);
		lp_comm_complete(conn, ctx, evt, param);
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_comm_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
}

static void lp_comm_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
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
void ull_cp_priv_lp_comm_tx_ack(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_ACK, tx->pdu);
}

void ull_cp_priv_lp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_RESPONSE, rx->pdu);
}

void ull_cp_priv_lp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_COMMON_STATE_IDLE;
}

void ull_cp_priv_lp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_comm_execute_fsm(conn, ctx, LP_COMMON_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Common FSM
 */

static void rp_comm_rx_decode(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
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
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

static void rp_comm_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx)
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
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_comm_st_idle(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
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

static void rp_comm_send_rsp(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (ctx->proc) {
	case PROC_LE_PING:
		/* Always respond on remote ping */
		if (!tx_alloc_is_available() || ctx->pause) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_tx(conn, ctx);
			rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
		break;
	case PROC_FEATURE_EXCHANGE:
		/* Always respond on remote feature exchange */
		if (!tx_alloc_is_available() || ctx->pause) {
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
			if (!tx_alloc_is_available() || ctx->pause) {
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
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void rp_comm_st_wait_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_REQUEST:
		rp_comm_rx_decode(conn, ctx, (struct pdu_data *) param);
		if (ctx->pause) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			rp_comm_send_rsp(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_comm_st_wait_tx(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
}

static void rp_comm_st_wait_ntf(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
{
	/* TODO */
}

static void rp_comm_execute_fsm(struct ull_cp_conn *conn, struct proc_ctx *ctx, u8_t evt, void *param)
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
	case RP_COMMON_STATE_WAIT_NTF:
		rp_comm_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_comm_rx(struct ull_cp_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_REQUEST, rx->pdu);
}

void ull_cp_priv_rp_comm_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_COMMON_STATE_IDLE;
}

void ull_cp_priv_rp_comm_run(struct ull_cp_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_comm_execute_fsm(conn, ctx, RP_COMMON_EVT_RUN, param);
}
