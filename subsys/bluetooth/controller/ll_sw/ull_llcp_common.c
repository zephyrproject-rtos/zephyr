/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "ll_feat.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_iso_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"

#include "ull_conn_types.h"
#include "ull_chan_internal.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_internal.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"

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
	RP_COMMON_STATE_POSTPONE_TERMINATE,
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


static void lp_comm_ntf(struct ll_conn *conn, struct proc_ctx *ctx);
static void lp_comm_terminate_invalid_pdu(struct ll_conn *conn, struct proc_ctx *ctx);

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
/**
 * @brief Stop and tear down a connected ISO stream
 * This function may be called to tear down a CIS.
 *
 * @param cig_id         ID of specific ISO group
 * @param cis_id         ID of connected ISO stream to stop
 * @param reason         Termination reason
 */
static void llcp_cis_stop_by_id(uint8_t cig_id, uint8_t cis_id, uint8_t reason)
{
	struct ll_conn_iso_group *cig = ll_conn_iso_group_get_by_id(cig_id);

	if (cig) {
		struct ll_conn_iso_stream *cis;
		uint16_t cis_handle = UINT16_MAX;

		/* Look through CIS's of specified group */
		cis = ll_conn_iso_stream_get_by_group(cig, &cis_handle);
		while (cis && cis->cis_id != cis_id) {
			/* Get next CIS */
			cis = ll_conn_iso_stream_get_by_group(cig, &cis_handle);
		}
		if (cis && cis->lll.handle == cis_handle) {
			ull_conn_iso_cis_stop(cis, NULL, reason);
		}
	}
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */

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
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		llcp_pdu_encode_cis_terminate_ind(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
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
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		llcp_pdu_encode_clock_accuracy_req(ctx, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP;
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	if (ctx->proc != PROC_TERMINATE) {
		/* Use normal timeout value of 40s */
		llcp_lr_prt_restart(conn);
	} else {
		/* Use supervision timeout value
		 * NOTE: As the supervision timeout is at most 32s the normal procedure response
		 * timeout of 40s will never come into play for the ACL Termination procedure.
		 */
		const uint32_t conn_interval_us = conn->lll.interval * CONN_INT_UNIT_US;
		const uint16_t sto_reload = RADIO_CONN_EVENTS(
			(conn->supervision_timeout * 10U * 1000U),
			conn_interval_us);
		llcp_lr_prt_restart_with_value(conn, sto_reload);
	}
}

static void lp_comm_ntf_feature_exchange(struct ll_conn *conn, struct proc_ctx *ctx,
					 struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		llcp_ntf_encode_feature_rsp(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		llcp_ntf_encode_unknown_rsp(ctx, pdu);
		break;
	default:
		/* Unexpected PDU, should not get through, so ASSERT */
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
		/* Unexpected PDU, should not get through, so ASSERT */
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

static void lp_comm_complete_cte_req_finalize(struct ll_conn *conn)
{
	llcp_rr_set_paused_cmd(conn, PROC_NONE);
	llcp_lr_complete(conn);
}

static void lp_comm_ntf_cte_req(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	switch (ctx->response_opcode) {
	case PDU_DATA_LLCTRL_TYPE_CTE_RSP:
		/* Notify host that received LL_CTE_RSP does not have CTE */
		if (!ctx->data.cte_remote_rsp.has_cte) {
			llcp_ntf_encode_cte_req(pdu);
		}
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		llcp_ntf_encode_unknown_rsp(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_ntf_encode_reject_ext_ind(ctx, pdu);
		break;
	default:
		/* Unexpected PDU, should not get through, so ASSERT */
		LL_ASSERT(0);
	}
}

static void lp_comm_ntf_cte_req_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (llcp_ntf_alloc_is_available()) {
		lp_comm_ntf(conn, ctx);
		ull_cp_cte_req_set_disable(conn);
		ctx->state = LP_COMMON_STATE_IDLE;
	} else {
		ctx->state = LP_COMMON_STATE_WAIT_NTF;
	}
}

static void lp_comm_complete_cte_req(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (conn->llcp.cte_req.is_enabled) {
		if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_CTE_RSP) {
			if (ctx->data.cte_remote_rsp.has_cte) {
				if (conn->llcp.cte_req.req_interval != 0U) {
					conn->llcp.cte_req.req_expire =
						conn->llcp.cte_req.req_interval;
				} else {
					/* Disable the CTE request procedure when it is completed in
					 * case it was executed as non-periodic.
					 */
					conn->llcp.cte_req.is_enabled = 0U;
				}
				ctx->state = LP_COMMON_STATE_IDLE;
			} else {
				lp_comm_ntf_cte_req_tx(conn, ctx);
			}
		} else if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND &&
			   ctx->reject_ext_ind.reject_opcode == PDU_DATA_LLCTRL_TYPE_CTE_REQ) {
			lp_comm_ntf_cte_req_tx(conn, ctx);
		} else if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP &&
			   ctx->unknown_response.type == PDU_DATA_LLCTRL_TYPE_CTE_REQ) {
			/* CTE response is unsupported in peer, so disable locally for this
			 * connection
			 */
			feature_unmask_features(conn, LL_FEAT_BIT_CONNECTION_CTE_REQ);
			lp_comm_ntf_cte_req_tx(conn, ctx);
		} else if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNUSED) {
			/* This path is related with handling disable the CTE REQ when PHY
			 * has been changed to CODED PHY. BT 5.3 Core Vol 4 Part E 7.8.85
			 * says CTE REQ has to be automatically disabled as if it had been requested
			 * by Host. There is no notification send to Host.
			 */
			ull_cp_cte_req_set_disable(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		} else {
			/* Illegal response opcode, internally changes state to
			 * LP_COMMON_STATE_IDLE
			 */
			lp_comm_terminate_invalid_pdu(conn, ctx);
		}
	} else {
		/* The CTE_REQ was disabled by Host after the request was send.
		 * It does not matter if response has arrived, it should not be handled.
		 */
		ctx->state = LP_COMMON_STATE_IDLE;
	}

	if (ctx->state == LP_COMMON_STATE_IDLE) {
		lp_comm_complete_cte_req_finalize(conn);
	}
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
static void lp_comm_ntf_sca(struct node_rx_pdu *ntf, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	struct node_rx_sca *pdu_sca = (struct node_rx_sca *)pdu;

	ntf->hdr.type = NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE;
	pdu_sca->status = ctx->data.sca_update.error_code;
	pdu_sca->sca = ctx->data.sca_update.sca;
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

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
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		lp_comm_ntf_sca(ntf, ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		LL_ASSERT(0);
		break;
	}

	/* Enqueue notification towards LL */
	ll_rx_put_sched(ntf->hdr.link, ntf);
}

static void lp_comm_terminate_invalid_pdu(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* Invalid behaviour */
	/* Invalid PDU received so terminate connection */
	conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
	llcp_lr_complete(conn);
	ctx->state = LP_COMMON_STATE_IDLE;
}

static void lp_comm_ntf_complete_proxy(struct ll_conn *conn, struct proc_ctx *ctx,
				       const bool valid_pdu)
{
	if (valid_pdu) {
		if (!llcp_ntf_alloc_is_available()) {
			ctx->state = LP_COMMON_STATE_WAIT_NTF;
		} else {
			lp_comm_ntf(conn, ctx);
			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		}
	} else {
		/* Illegal response opcode */
		lp_comm_terminate_invalid_pdu(conn, ctx);
	}
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
			lp_comm_terminate_invalid_pdu(conn, ctx);
		}
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		lp_comm_ntf_complete_proxy(conn, ctx,
			(ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP ||
			 ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_FEATURE_RSP));
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_MIN_USED_CHANS:
		llcp_lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
	case PROC_VERSION_EXCHANGE:
		lp_comm_ntf_complete_proxy(conn, ctx,
			(ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_VERSION_IND));
		break;
	case PROC_TERMINATE:
		/* No notification */
		llcp_lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;

		/* Mark the connection for termination */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LOCALHOST_TERM_CONN;
		break;
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		/* No notification */
		llcp_lr_complete(conn);
		ctx->state = LP_COMMON_STATE_IDLE;
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_LENGTH_RSP) {
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
		} else if (ctx->response_opcode == PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP) {
			/* Peer does not accept DLU, so disable on current connection */
			feature_unmask_features(conn, LL_FEAT_BIT_DLE);

			llcp_lr_complete(conn);
			ctx->state = LP_COMMON_STATE_IDLE;
		} else {
			/* Illegal response opcode */
			lp_comm_terminate_invalid_pdu(conn, ctx);
			break;
		}

		if (!ull_cp_remote_dle_pending(conn)) {
			/* Resume data, but only if there is no remote procedure pending RSP
			 * in which case, the RSP tx-ACK will resume data
			 */
			llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);
		}
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PROC_CTE_REQ:
		lp_comm_complete_cte_req(conn, ctx);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		switch (ctx->response_opcode) {
		case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
			/* Peer does not support SCA update, so disable on current connection */
			feature_unmask_features(conn, LL_FEAT_BIT_SCA_UPDATE);
			ctx->data.sca_update.error_code = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			/* Fall through to complete procedure */
		case PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP:
#if defined(CONFIG_BT_PERIPHERAL)
			if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL &&
			    !ctx->data.sca_update.error_code &&
			    conn->periph.sca != ctx->data.sca_update.sca) {
				conn->periph.sca = ctx->data.sca_update.sca;
				ull_conn_update_peer_sca(conn);
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
				ull_peripheral_iso_update_peer_sca(conn);
#endif /* defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */
			}
#endif /* CONFIG_BT_PERIPHERAL */
			if (!llcp_ntf_alloc_is_available()) {
				ctx->state = LP_COMMON_STATE_WAIT_NTF;
			} else {
				lp_comm_ntf(conn, ctx);
				llcp_lr_complete(conn);
				ctx->state = LP_COMMON_STATE_IDLE;
			}
			break;
		default:
			/* Illegal response opcode */
			lp_comm_terminate_invalid_pdu(conn, ctx);
		}
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
static bool lp_cis_terminated(struct ll_conn *conn)
{
	return conn->llcp.cis.terminate_ack;
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */

static bool lp_comm_tx_proxy(struct ll_conn *conn, struct proc_ctx *ctx, const bool extra_cond)
{
	if (extra_cond || llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_COMMON_STATE_WAIT_TX;
	} else {
		lp_comm_tx(conn, ctx);

		/* Select correct state, depending on TX ack handling 'request' */
		ctx->state = ctx->tx_ack ? LP_COMMON_STATE_WAIT_TX_ACK : LP_COMMON_STATE_WAIT_RX;
		return true;
	}
	return false;
}

static void lp_comm_send_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		lp_comm_tx_proxy(conn, ctx, false);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		lp_comm_tx_proxy(conn, ctx, false);
		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case PROC_MIN_USED_CHANS:
		lp_comm_tx_proxy(conn, ctx, false);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of
		 * one LL_VERSION_IND PDU during a connection.
		 */
		if (!conn->llcp.vex.sent) {
			if (lp_comm_tx_proxy(conn, ctx, false)) {
				conn->llcp.vex.sent = 1;
			}
		} else {
			ctx->response_opcode = PDU_DATA_LLCTRL_TYPE_VERSION_IND;
			lp_comm_complete(conn, ctx, evt, param);
		}
		break;
	case PROC_TERMINATE:
		if (!llcp_tx_alloc_peek(conn, ctx)) {
			ctx->state = LP_COMMON_STATE_WAIT_TX;
		} else {
			lp_comm_tx(conn, ctx);
			ctx->data.term.error_code = BT_HCI_ERR_LOCALHOST_TERM_CONN;
			ctx->state = LP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		lp_comm_tx_proxy(conn, ctx, !lp_cis_terminated(conn));
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		if (!ull_cp_remote_dle_pending(conn)) {
			if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
				ctx->state = LP_COMMON_STATE_WAIT_TX;
			} else {
				/* Pause data tx, to ensure we can later (on RSP rx-ack)
				 * update DLE without conflicting with out-going LL Data PDUs
				 * See BT Core 5.2 Vol6: B-4.5.10 & B-5.1.9
				 */
				llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);
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
		if (conn->llcp.cte_req.is_enabled &&
#if defined(CONFIG_BT_CTLR_PHY)
		    conn->lll.phy_rx != PHY_CODED) {
#else
		    1) {
#endif /* CONFIG_BT_CTLR_PHY */
			lp_comm_tx_proxy(conn, ctx,
					 (llcp_rr_get_paused_cmd(conn) == PROC_CTE_REQ));
		} else {
			/* The PHY was changed to CODED when the request was waiting in a local
			 * request queue.
			 *
			 * Use of pair: proc PROC_CTE_REQ and rx_opcode PDU_DATA_LLCTRL_TYPE_UNUSED
			 * to complete the procedure before sending a request to peer.
			 * This is a special complete execution path to disable the procedure
			 * due to change of RX PHY to CODED.
			 */
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
			ctx->state = LP_COMMON_STATE_IDLE;
			llcp_lr_complete(conn);
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		lp_comm_tx_proxy(conn, ctx, false);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_COMMON_EVT_RUN:
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
		if (ctx->proc == PROC_CIS_TERMINATE) {
			/* We're getting going on a CIS Terminate */
			/* So we should start by requesting Terminate for the CIS in question */

			/* Clear terminate ack flag, used to signal CIS Terminated */
			conn->llcp.cis.terminate_ack = 0U;
			llcp_cis_stop_by_id(ctx->data.cis_term.cig_id, ctx->data.cis_term.cis_id,
					    ctx->data.cis_term.error_code);
		}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
		if (llcp_lr_ispaused(conn)) {
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

static void lp_comm_st_wait_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
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
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
		case PROC_CIS_TERMINATE:
			ctx->tx_ack = NULL;
			lp_comm_complete(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
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
#if defined(CONFIG_BT_CTLR_DATA_LENGTH) && defined(CONFIG_BT_CTLR_PHY)
		/* If Coded PHY is now supported we must update local max tx/rx times to reflect */
		if (feature_phy_coded(conn)) {
			ull_dle_max_time_get(conn, &conn->lll.dle.local.max_rx_time,
					     &conn->lll.dle.local.max_tx_time);
		}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH && CONFIG_BT_CTLR_PHY */
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
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PDU_DATA_LLCTRL_TYPE_CTE_RSP:
		llcp_pdu_decode_cte_rsp(ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_RSP:
		llcp_pdu_decode_clock_accuracy_rsp(ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_pdu_decode_reject_ext_ind(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* Empty on purpose, as we don't care about the PDU content, we'll disconnect */
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

static void lp_comm_st_wait_rx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
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
	switch (evt) {
	case LP_COMMON_EVT_RUN:
		switch (ctx->proc) {
		case PROC_FEATURE_EXCHANGE:
		case PROC_VERSION_EXCHANGE:
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		case PROC_DATA_LENGTH_UPDATE:
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
		case PROC_SCA_UPDATE:
#endif /* CONFIG_BT_CTLR_SCA_UPDATE) */
			if (llcp_ntf_alloc_is_available()) {
				lp_comm_ntf(conn, ctx);
				llcp_lr_complete(conn);
				ctx->state = LP_COMMON_STATE_IDLE;
			}
			break;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
		case PROC_CTE_REQ:
			if (llcp_ntf_alloc_is_available()) {
				lp_comm_ntf(conn, ctx);
				ctx->state = LP_COMMON_STATE_IDLE;
				lp_comm_complete_cte_req_finalize(conn);
			}
			break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
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

static void rp_comm_terminate(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_rr_complete(conn);
	ctx->state = RP_COMMON_STATE_IDLE;

	/* Mark the connection for termination */
	conn->llcp_terminate.reason_final = ctx->data.term.error_code;
}

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
static void rp_comm_stop_cis(struct proc_ctx *ctx)
{
	llcp_cis_stop_by_id(ctx->data.cis_term.cig_id, ctx->data.cis_term.cis_id,
			    ctx->data.cis_term.error_code);
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */

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
#if defined(CONFIG_BT_PERIPHERAL) || \
	(defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_CENTRAL))
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG) && defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG:
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_CENTRAL */
		llcp_pdu_decode_feature_req(conn, pdu);
#if defined(CONFIG_BT_CTLR_DATA_LENGTH) && defined(CONFIG_BT_CTLR_PHY)
		/* If Coded PHY is now supported we must update local max tx/rx times to reflect */
		if (feature_phy_coded(conn)) {
			ull_dle_max_time_get(conn, &conn->lll.dle.local.max_rx_time,
					     &conn->lll.dle.local.max_tx_time);
		}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH && CONFIG_BT_CTLR_PHY */
		break;
#endif /* CONFIG_BT_PERIPHERAL || (CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG && CONFIG_BT_CENTRAL) */
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
		/* Make sure no data is tx'ed after RX of terminate ind */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_TERMINATE);
		break;
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PDU_DATA_LLCTRL_TYPE_CIS_TERMINATE_IND:
		llcp_pdu_decode_cis_terminate_ind(ctx, pdu);
		/* Terminate CIS */
		rp_comm_stop_cis(ctx);
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
		llcp_pdu_decode_length_req(conn, pdu);
		/* On reception of REQ mark RSP open for local piggy-back
		 * Pause data tx, to ensure we can later (on RSP tx ack) update TX DLE without
		 * conflicting with out-going LL Data PDUs
		 * See BT Core 5.2 Vol6: B-4.5.10 & B-5.1.9
		 */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);
		ctx->data.dle.ntf_dle = ull_dle_update_eff_rx(conn);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	case PDU_DATA_LLCTRL_TYPE_CTE_REQ:
		llcp_pdu_decode_cte_req(ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PDU_DATA_LLCTRL_TYPE_CLOCK_ACCURACY_REQ:
		llcp_pdu_decode_clock_accuracy_req(ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
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
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		llcp_pdu_encode_feature_rsp(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
	case PROC_VERSION_EXCHANGE:
		llcp_pdu_encode_version_ind(pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		llcp_pdu_encode_length_rsp(conn, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	case PROC_CTE_REQ: {
		uint8_t err_code = 0;

		if (conn->llcp.cte_rsp.is_enabled == 0) {
			err_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
		}

#if defined(CONFIG_BT_PHY_UPDATE)
		/* If the PHY update is not possible, then PHY1M is used.
		 * CTE is supported for PHY1M.
		 */
		if (conn->lll.phy_tx == PHY_CODED) {
			err_code = BT_HCI_ERR_INVALID_LL_PARAM;
		}
#endif /* CONFIG_BT_PHY_UPDATE */
		if (!(conn->llcp.cte_rsp.cte_types & BIT(ctx->data.cte_remote_req.cte_type)) ||
		    conn->llcp.cte_rsp.max_cte_len < ctx->data.cte_remote_req.min_cte_len) {
			err_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
		}

		if (!err_code) {
			llcp_pdu_encode_cte_rsp(ctx, pdu);
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		} else {
			llcp_pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_CTE_REQ, err_code);
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		}

		ctx->tx_ack = tx;

		break;
	}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		llcp_pdu_encode_clock_accuracy_rsp(ctx, pdu);
		ctx->tx_ack = tx;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
	default:
		/* Unknown procedure */
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_rr_prt_restart(conn);
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
	ll_rx_put_sched(ntf->hdr.link, ntf);
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static bool rp_comm_tx_proxy(struct ll_conn *conn, struct proc_ctx *ctx, const bool complete)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_COMMON_STATE_WAIT_TX;
		return false;
	}

	rp_comm_tx(conn, ctx);
	ctx->state = RP_COMMON_STATE_WAIT_TX_ACK;
	if (complete) {
		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;
	}

	return true;
}

static void rp_comm_send_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_LE_PING)
	case PROC_LE_PING:
		/* Always respond on remote ping */
		rp_comm_tx_proxy(conn, ctx, true);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */
	case PROC_FEATURE_EXCHANGE:
		/* Always respond on remote feature exchange */
		rp_comm_tx_proxy(conn, ctx, true);
		break;
	case PROC_VERSION_EXCHANGE:
		/* The Link Layer shall only queue for transmission a maximum of one
		 * LL_VERSION_IND PDU during a connection.
		 * If the Link Layer receives an LL_VERSION_IND PDU and has already sent an
		 * LL_VERSION_IND PDU then the Link Layer shall not send another
		 * LL_VERSION_IND PDU to the peer device.
		 */
		if (!conn->llcp.vex.sent) {
			if (rp_comm_tx_proxy(conn, ctx, true)) {
				conn->llcp.vex.sent = 1;
			}
		} else {
			/* Invalid behaviour
			 * A procedure already sent a LL_VERSION_IND and received a LL_VERSION_IND.
			 * Ignore and complete the procedure.
			 */
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}

		break;
#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_CENTRAL)
	case PROC_MIN_USED_CHANS:
		/*
		 * Spec says (5.2, Vol.6, Part B, Section 5.1.11):
		 *     The procedure has completed when the Link Layer acknowledgment of the
		 *     LL_MIN_USED_CHANNELS_IND PDU is sent or received.
		 * In effect, for this procedure, this is equivalent to RX of PDU
		 *
		 * Also:
		 *     If the Link Layer receives an LL_MIN_USED_CHANNELS_IND PDU, it should ensure
		 *     that, whenever the Peripheral-to-Central PHY is one of those specified,
		 *     the connection uses at least the number of channels given in the
		 *     MinUsedChannels field of the PDU.
		 *
		 * The 'should' is here interpreted as 'permission' to do nothing
		 *
		 * Future improvement could implement logic to support this
		 */

		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_CENTRAL */
	case PROC_TERMINATE:
#if defined(CONFIG_BT_CENTRAL)
		if (conn->lll.role == BT_HCI_ROLE_CENTRAL) {
			/* No response, but postpone terminate until next event
			 * to ensure acking the reception of TERMINATE_IND
			 */
			ctx->state = RP_COMMON_STATE_POSTPONE_TERMINATE;
			break;
		}
#endif
#if defined(CONFIG_BT_PERIPHERAL)
		/* Terminate right away */
		rp_comm_terminate(conn, ctx);
#endif
		break;
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) || defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case PROC_CIS_TERMINATE:
		/* No response */
		llcp_rr_complete(conn);
		ctx->state = RP_COMMON_STATE_IDLE;

		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO || CONFIG_BT_CTLR_PERIPHERAL_ISO */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PROC_DATA_LENGTH_UPDATE:
		rp_comm_tx_proxy(conn, ctx, false);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	case PROC_CTE_REQ:
		if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx) ||
		    (llcp_rr_get_paused_cmd(conn) == PROC_CTE_REQ)) {
			ctx->state = RP_COMMON_STATE_WAIT_TX;
		} else {
			llcp_rr_set_paused_cmd(conn, PROC_PHY_UPDATE);
			rp_comm_tx(conn, ctx);
			ctx->state = RP_COMMON_STATE_WAIT_TX_ACK;
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case PROC_SCA_UPDATE:
		/* Always respond to remote SCA */
		rp_comm_tx_proxy(conn, ctx, false);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
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

static void rp_comm_st_postpone_terminate(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_RUN:
		LL_ASSERT(ctx->proc == PROC_TERMINATE);

		/* Note: now we terminate, mimicking legacy LLCP behaviour
		 * A check should be added to ensure that the ack of the terminate_ind was
		 * indeed tx'ed and not scheduled out/postponed by LLL
		 */
		rp_comm_terminate(conn, ctx);

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

static void rp_comm_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case RP_COMMON_EVT_ACK:
		switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		case PROC_DATA_LENGTH_UPDATE: {
			/* Apply changes in data lengths/times */
			uint8_t dle_changed = ull_dle_update_eff_tx(conn);

			dle_changed |= ctx->data.dle.ntf_dle;
			llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_DATA_LENGTH);

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
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
		case PROC_CTE_REQ: {
			/* add PHY update pause = false here */
			ctx->tx_ack = NULL;
			llcp_rr_set_paused_cmd(conn, PROC_NONE);
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
		case PROC_SCA_UPDATE: {
			ctx->tx_ack = NULL;
#if defined(CONFIG_BT_PERIPHERAL)
			if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL) {
				conn->periph.sca = ctx->data.sca_update.sca;
				ull_conn_update_peer_sca(conn);
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
				ull_peripheral_iso_update_peer_sca(conn);
#endif /* defined(CONFIG_BT_CTLR_PERIPHERAL_ISO) */
			}
#endif /* CONFIG_BT_PERIPHERAL */
			llcp_rr_complete(conn);
			ctx->state = RP_COMMON_STATE_IDLE;
		}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
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

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
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
	case RP_COMMON_STATE_POSTPONE_TERMINATE:
		rp_comm_st_postpone_terminate(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_TX:
		rp_comm_st_wait_tx(conn, ctx, evt, param);
		break;
	case RP_COMMON_STATE_WAIT_TX_ACK:
		rp_comm_st_wait_tx_ack(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
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
