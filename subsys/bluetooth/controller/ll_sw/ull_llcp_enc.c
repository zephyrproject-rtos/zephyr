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
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"

#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_llcp_features.h"
#include "ull_conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_enc
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CENTRAL)
/* LLCP Local Procedure Encryption FSM states */
enum {
	/* Start Procedure */
	LP_ENC_STATE_UNENCRYPTED,
	LP_ENC_STATE_WAIT_TX_ENC_REQ,
	LP_ENC_STATE_WAIT_RX_ENC_RSP,
	LP_ENC_STATE_WAIT_RX_START_ENC_REQ,
	LP_ENC_STATE_WAIT_TX_START_ENC_RSP,
	LP_ENC_STATE_WAIT_RX_START_ENC_RSP,
	LP_ENC_STATE_WAIT_NTF,
	/* Pause Procedure */
	LP_ENC_STATE_ENCRYPTED,
	LP_ENC_STATE_WAIT_TX_PAUSE_ENC_REQ,
	LP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP,
	LP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP,
};

/* LLCP Local Procedure Encryption FSM events */
enum {
	/* Procedure prepared */
	LP_ENC_EVT_RUN,

	/* Response received */
	LP_ENC_EVT_ENC_RSP,

	/* Request received */
	LP_ENC_EVT_START_ENC_REQ,

	/* Response received */
	LP_ENC_EVT_START_ENC_RSP,

	/* Reject response received */
	LP_ENC_EVT_REJECT,

	/* Unknown response received */
	LP_ENC_EVT_UNKNOWN,

	/* Response received */
	LP_ENC_EVT_PAUSE_ENC_RSP,
};
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
/* LLCP Remote Procedure Encryption FSM states */
enum {
	/* Start Procedure */
	RP_ENC_STATE_UNENCRYPTED,
	RP_ENC_STATE_WAIT_RX_ENC_REQ,
	RP_ENC_STATE_WAIT_TX_ENC_RSP,
	RP_ENC_STATE_WAIT_NTF_LTK_REQ,
	RP_ENC_STATE_WAIT_LTK_REPLY,
	RP_ENC_STATE_WAIT_TX_START_ENC_REQ,
	RP_ENC_STATE_WAIT_TX_REJECT_IND,
	RP_ENC_STATE_WAIT_RX_START_ENC_RSP,
	RP_ENC_STATE_WAIT_NTF,
	RP_ENC_STATE_WAIT_TX_START_ENC_RSP,
	/* Pause Procedure */
	RP_ENC_STATE_ENCRYPTED,
	RP_ENC_STATE_WAIT_RX_PAUSE_ENC_REQ,
	RP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP,
	RP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP,
};

/* LLCP Remote Procedure Encryption FSM events */
enum {
	/* Procedure prepared */
	RP_ENC_EVT_RUN,

	/* Request received */
	RP_ENC_EVT_ENC_REQ,

	/* Response received */
	RP_ENC_EVT_START_ENC_RSP,

	/* LTK request reply */
	RP_ENC_EVT_LTK_REQ_REPLY,

	/* LTK request negative reply */
	RP_ENC_EVT_LTK_REQ_NEG_REPLY,

	/* Reject response received */
	RP_ENC_EVT_REJECT,

	/* Unknown response received */
	RP_ENC_EVT_UNKNOWN,

	/* Request received */
	RP_ENC_EVT_PAUSE_ENC_REQ,

	/* Response received */
	RP_ENC_EVT_PAUSE_ENC_RSP,
};
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
/*
 * LLCP Local Procedure Encryption FSM
 */

static struct node_tx *llcp_lp_enc_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		llcp_pdu_encode_enc_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		llcp_pdu_encode_start_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		llcp_pdu_encode_pause_enc_req(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		llcp_pdu_encode_pause_enc_rsp(pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_lr_prt_restart(conn);

	return tx;
}

static void lp_enc_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;

	if (ctx->data.enc.error == BT_HCI_ERR_SUCCESS) {
		if (ctx->proc == PROC_ENCRYPTION_START) {
			/* Encryption Change Event */
			/* TODO(thoh): is this correct? */
			llcp_pdu_encode_start_enc_rsp(pdu);
		} else if (ctx->proc == PROC_ENCRYPTION_PAUSE) {
			/* Encryption Key Refresh Complete Event */
			ntf->hdr.type = NODE_RX_TYPE_ENC_REFRESH;
		} else {
			/* Should never happen */
			LL_ASSERT(0);
		}
	} else {
		llcp_pdu_encode_reject_ind(pdu, ctx->data.enc.error);
	}

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void lp_enc_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = LP_ENC_STATE_WAIT_NTF;
	} else {
		lp_enc_ntf(conn, ctx);
		llcp_lr_complete(conn);
		ctx->state = LP_ENC_STATE_UNENCRYPTED;
	}
}

static void lp_enc_store_m(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* Store SKDm */
	memcpy(&ctx->data.enc.skd[0], pdu->llctrl.enc_req.skdm, sizeof(pdu->llctrl.enc_req.skdm));
	/* Store IVm in the LLL CCM RX
	 * TODO(thoh): Should this be made into a ULL function, as it
	 * interacts with data outside of LLCP?
	 */
	memcpy(&conn->lll.ccm_rx.iv[0], pdu->llctrl.enc_req.ivm, sizeof(pdu->llctrl.enc_req.ivm));
}

static void lp_enc_send_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	struct node_tx *tx;

	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_ENC_STATE_WAIT_TX_ENC_REQ;
	} else {
		tx = llcp_lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_ENC_REQ);
		lp_enc_store_m(conn, ctx, (struct pdu_data *)tx->pdu);
		/* Wait for the LL_ENC_RSP */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_ENC_RSP;
		ctx->state = LP_ENC_STATE_WAIT_RX_ENC_RSP;

		/* Pause possibly ongoing remote procedure */
		llcp_rr_pause(conn);

	}
}

static void lp_enc_send_pause_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_ENC_STATE_WAIT_TX_PAUSE_ENC_REQ;
	} else {
		llcp_lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ);
		/* Wait for the LL_PAUSE_ENC_RSP */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
		ctx->state = LP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP;
	}
}

static void lp_enc_send_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP;
	} else {
		llcp_lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP);

		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		/* Continue with an encapsulated Start Procedure */
		ctx->state = LP_ENC_STATE_UNENCRYPTED;

		/* Tx Encryption disabled */
		conn->lll.enc_tx = 0U;

		/* Rx Decryption disabled */
		conn->lll.enc_rx = 0U;
	}
}

static void lp_enc_setup_lll(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* TODO(thoh): Move LLL/CCM manipulation to ULL? */

	/* Calculate the Session Key */
	ecb_encrypt(&ctx->data.enc.ltk[0], &ctx->data.enc.skd[0], NULL, &conn->lll.ccm_rx.key[0]);

	/* Copy the Session Key */
	memcpy(&conn->lll.ccm_tx.key[0], &conn->lll.ccm_rx.key[0], sizeof(conn->lll.ccm_tx.key));

	/* Copy the IV */
	memcpy(&conn->lll.ccm_tx.iv[0], &conn->lll.ccm_rx.iv[0], sizeof(conn->lll.ccm_tx.iv));

	/* Reset CCM counter */
	conn->lll.ccm_tx.counter = 0U;
	conn->lll.ccm_rx.counter = 0U;

	/* Set CCM direction:
	 *	periph to central = 0,
	 *	central to periph = 1
	 */
	conn->lll.ccm_tx.direction = 1U;
	conn->lll.ccm_rx.direction = 0U;
}

static void lp_enc_send_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_ENC_STATE_WAIT_TX_START_ENC_RSP;
	} else {
		lp_enc_setup_lll(conn, ctx);
		llcp_lp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP);

		/* Wait for LL_START_ENC_RSP */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
		ctx->state = LP_ENC_STATE_WAIT_RX_START_ENC_RSP;

		/* Tx Encryption enabled */
		conn->lll.enc_tx = 1U;

		/* Rx Decryption enabled */
		conn->lll.enc_rx = 1U;
	}
}

static void lp_enc_st_unencrypted(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		/* Pause Tx data */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		lp_enc_send_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_tx_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
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

static void lp_enc_store_s(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* Store SKDs */
	memcpy(&ctx->data.enc.skd[8], pdu->llctrl.enc_rsp.skds, sizeof(pdu->llctrl.enc_rsp.skds));
	/* Store IVs in the LLL CCM RX
	 * TODO(thoh): Should this be made into a ULL function, as it
	 * interacts with data outside of LLCP?
	 */
	memcpy(&conn->lll.ccm_rx.iv[4], pdu->llctrl.enc_rsp.ivs, sizeof(pdu->llctrl.enc_rsp.ivs));
}

static inline uint8_t reject_error_code(struct pdu_data *pdu)
{
	if (pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_IND) {
		return pdu->llctrl.reject_ind.error_code;
#if defined(CONFIG_BT_CTLR_EXT_REJ_IND)
	} else if (pdu->llctrl.opcode == PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND) {
		return pdu->llctrl.reject_ext_ind.error_code;
#endif /* CONFIG_BT_CTLR_EXT_REJ_IND */
	} else {
		/* Called with an invalid PDU */
		LL_ASSERT(0);

		/* Keep compiler happy */
		return BT_HCI_ERR_UNSPECIFIED;
	}
}

static void lp_enc_st_wait_rx_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_ENC_EVT_ENC_RSP:
		/* Pause Rx data */
		ull_conn_pause_rx_data(conn);
		lp_enc_store_s(conn, ctx, pdu);

		/* After the Central has received the LL_ENC_RSP PDU,
		 * only PDUs related to this procedure are valid, and invalids should
		 * result in disconnect.
		 * to achieve this enable the greedy RX behaviour, such that
		 * all PDU's end up in this FSM.
		 */
		ctx->rx_greedy = 1U;

		/* Wait for LL_START_ENC_REQ */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_REQ;
		ctx->state = LP_ENC_STATE_WAIT_RX_START_ENC_REQ;
		break;
	case LP_ENC_EVT_REJECT:
		/* Encryption is not supported by the Link Layer of the Peripheral */

		/* Resume Tx data */
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);

		/* Store the error reason */
		ctx->data.enc.error = reject_error_code(pdu);

		/* Resume possibly paused remote procedure */
		llcp_rr_resume(conn);

		/* Complete the procedure */
		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_rx_start_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_ENC_EVT_START_ENC_REQ:
		lp_enc_send_start_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_EVT_REJECT:
		/* Resume Tx data */
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/* Resume Rx data */
		ull_conn_resume_rx_data(conn);

		/* Store the error reason */
		ctx->data.enc.error = reject_error_code(pdu);

		/* Resume possibly paused remote procedure */
		llcp_rr_resume(conn);

		/* Disable the greedy behaviour */
		ctx->rx_greedy = 0U;

		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_tx_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
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

static void lp_enc_st_wait_rx_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case LP_ENC_EVT_START_ENC_RSP:
		/* Resume Tx data */
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/* Resume Rx data */
		ull_conn_resume_rx_data(conn);
		ctx->data.enc.error = BT_HCI_ERR_SUCCESS;

		/* Resume possibly paused remote procedure */
		llcp_rr_resume(conn);

		/* Disable the greedy behaviour */
		ctx->rx_greedy = 0U;

		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_state_encrypted(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		/* Pause Tx data */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		lp_enc_send_pause_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_state_wait_tx_pause_enc_req(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_send_pause_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_state_wait_rx_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_PAUSE_ENC_RSP:
		/*
		 * Pause Rx data; will be resumed when the encapsulated
		 * Start Procedure is done.
		 */
		ull_conn_pause_rx_data(conn);
		lp_enc_send_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_state_wait_tx_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case LP_ENC_EVT_RUN:
		lp_enc_send_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_enc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	/* Start Procedure */
	case LP_ENC_STATE_UNENCRYPTED:
		lp_enc_st_unencrypted(conn, ctx, evt, param);
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
	/* Pause Procedure */
	case LP_ENC_STATE_ENCRYPTED:
		lp_enc_state_encrypted(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_TX_PAUSE_ENC_REQ:
		lp_enc_state_wait_tx_pause_enc_req(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP:
		lp_enc_state_wait_rx_pause_enc_rsp(conn, ctx, evt, param);
		break;
	case LP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP:
		lp_enc_state_wait_tx_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_lp_enc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

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
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_REJECT, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_PAUSE_ENC_RSP, pdu);
		break;
	default:
		/* Unknown opcode */

		/*
		 * BLUETOOTH CORE SPECIFICATION Version 5.3
		 * Vol 6, Part B, 5.1.3.1 Encryption Start procedure
		 *
		 * [...]
		 *
		 * If, at any time during the encryption start procedure after the Peripheral has
		 * received the LL_ENC_REQ PDU or the Central has received the
		 * LL_ENC_RSP PDU, the Link Layer of the Central or the Peripheral receives an
		 * unexpected Data Physical Channel PDU from the peer Link Layer, it shall
		 * immediately exit the Connection state, and shall transition to the Standby state.
		 * The Host shall be notified that the link has been disconnected with the error
		 * code Connection Terminated Due to MIC Failure (0x3D).
		 */

		conn->llcp_terminate.reason_final = BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
	}
}

void llcp_lp_enc_init_proc(struct proc_ctx *ctx)
{
	switch (ctx->proc) {
	case PROC_ENCRYPTION_START:
		ctx->state = LP_ENC_STATE_UNENCRYPTED;
		break;
	case PROC_ENCRYPTION_PAUSE:
		ctx->state = LP_ENC_STATE_ENCRYPTED;
		break;
	default:
		LL_ASSERT(0);
	}
}

void llcp_lp_enc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_enc_execute_fsm(conn, ctx, LP_ENC_EVT_RUN, param);
}

#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
/*
 * LLCP Remote Procedure Encryption FSM
 */

static struct node_tx *llcp_rp_enc_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
		llcp_pdu_encode_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
		llcp_pdu_encode_start_enc_req(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		llcp_pdu_encode_start_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		llcp_pdu_encode_pause_enc_rsp(pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		if (conn->llcp.fex.valid && feature_ext_rej_ind(conn)) {
			llcp_pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_ENC_REQ,
						       BT_HCI_ERR_PIN_OR_KEY_MISSING);
		} else {
			llcp_pdu_encode_reject_ind(pdu, BT_HCI_ERR_PIN_OR_KEY_MISSING);
		}
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_rr_prt_restart(conn);

	return tx;
}

static void rp_enc_ntf_ltk(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;

	llcp_ntf_encode_enc_req(ctx, pdu);

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void rp_enc_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = llcp_ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;

	if (ctx->proc == PROC_ENCRYPTION_START) {
		/* Encryption Change Event */
		/* TODO(thoh): is this correct? */
		llcp_pdu_encode_start_enc_rsp(pdu);
	} else if (ctx->proc == PROC_ENCRYPTION_PAUSE) {
		/* Encryption Key Refresh Complete Event */
		ntf->hdr.type = NODE_RX_TYPE_ENC_REFRESH;
	} else {
		/* Should never happen */
		LL_ASSERT(0);
	}

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void rp_enc_send_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param);

static void rp_enc_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_NTF;
	} else {
		rp_enc_ntf(conn, ctx);
		rp_enc_send_start_enc_rsp(conn, ctx, evt, param);
	}
}

static void rp_enc_send_ltk_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = RP_ENC_STATE_WAIT_NTF_LTK_REQ;
	} else {
		rp_enc_ntf_ltk(conn, ctx);
		ctx->state = RP_ENC_STATE_WAIT_LTK_REPLY;
	}
}

static void rp_enc_store_s(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* Store SKDs */
	memcpy(&ctx->data.enc.skds, pdu->llctrl.enc_rsp.skds, sizeof(pdu->llctrl.enc_rsp.skds));
	/* Store IVs in the LLL CCM RX
	 * TODO(thoh): Should this be made into a ULL function, as it
	 * interacts with data outside of LLCP?
	 */
	memcpy(&conn->lll.ccm_rx.iv[4], pdu->llctrl.enc_rsp.ivs, sizeof(pdu->llctrl.enc_rsp.ivs));
}

static void rp_enc_send_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	struct node_tx *tx;

	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_ENC_STATE_WAIT_TX_ENC_RSP;
	} else {
		tx = llcp_rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_ENC_RSP);
		rp_enc_store_s(conn, ctx, (struct pdu_data *)tx->pdu);
		rp_enc_send_ltk_ntf(conn, ctx, evt, param);
	}
}

static void rp_enc_setup_lll(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* TODO(thoh): Move LLL/CCM manipulation to ULL? */

	/* Calculate the Session Key */
	ecb_encrypt(&ctx->data.enc.ltk[0], &ctx->data.enc.skd[0], NULL, &conn->lll.ccm_rx.key[0]);

	/* Copy the Session Key */
	memcpy(&conn->lll.ccm_tx.key[0], &conn->lll.ccm_rx.key[0], sizeof(conn->lll.ccm_tx.key));

	/* Copy the IV */
	memcpy(&conn->lll.ccm_tx.iv[0], &conn->lll.ccm_rx.iv[0], sizeof(conn->lll.ccm_tx.iv));

	/* Reset CCM counter */
	conn->lll.ccm_tx.counter = 0U;
	conn->lll.ccm_rx.counter = 0U;

	/* Set CCM direction:
	 *	periph to central = 0,
	 *	central to periph = 1
	 */
	conn->lll.ccm_tx.direction = 0U;
	conn->lll.ccm_rx.direction = 1U;
}

static void rp_enc_send_start_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_ENC_STATE_WAIT_TX_START_ENC_REQ;
	} else {
		rp_enc_setup_lll(conn, ctx);
		llcp_rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_REQ);
		/* Wait for the LL_START_ENC_RSP */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_START_ENC_RSP;
		ctx->state = RP_ENC_STATE_WAIT_RX_START_ENC_RSP;

		/* Rx Decryption enabled */
		conn->lll.enc_rx = 1U;
	}
}

static void rp_enc_send_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_ENC_STATE_WAIT_TX_REJECT_IND;
	} else {
		llcp_rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_IND);
		llcp_rr_complete(conn);
		ctx->state = RP_ENC_STATE_UNENCRYPTED;

		/* Resume Tx data */
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/* Resume Rx data */
		ull_conn_resume_rx_data(conn);
		/* Resume possibly paused local procedure */
		llcp_lr_resume(conn);
	}
}

static void rp_enc_send_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_ENC_STATE_WAIT_TX_START_ENC_RSP;
	} else {
		llcp_rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_START_ENC_RSP);
		llcp_rr_complete(conn);
		ctx->state = RP_ENC_STATE_UNENCRYPTED;

		/* Resume Tx data */
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/* Resume Rx data */
		ull_conn_resume_rx_data(conn);

		/* Resume possibly paused local procedure */
		llcp_lr_resume(conn);

		/* Tx Encryption enabled */
		conn->lll.enc_tx = 1U;
	}
}

static void rp_enc_send_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (!llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP;
	} else {
		llcp_rp_enc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP);
		/* Wait for the LL_PAUSE_ENC_RSP */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP;
		ctx->state = RP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP;

		/* Rx Decryption disabled */
		conn->lll.enc_rx = 0U;
	}
}

static void rp_enc_state_unencrypted(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
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

static void rp_enc_store_m(struct ll_conn *conn, struct proc_ctx *ctx, struct pdu_data *pdu)
{
	/* Store Rand */
	memcpy(ctx->data.enc.rand, pdu->llctrl.enc_req.rand, sizeof(ctx->data.enc.rand));

	/* Store EDIV */
	ctx->data.enc.ediv[0] = pdu->llctrl.enc_req.ediv[0];
	ctx->data.enc.ediv[1] = pdu->llctrl.enc_req.ediv[1];

	/* Store SKDm */
	memcpy(&ctx->data.enc.skdm, pdu->llctrl.enc_req.skdm, sizeof(ctx->data.enc.skdm));

	/* Store IVm in the LLL CCM RX
	 * TODO(thoh): Should this be made into a ULL function, as it
	 * interacts with data outside of LLCP?
	 */
	memcpy(&conn->lll.ccm_rx.iv[0], pdu->llctrl.enc_req.ivm, sizeof(pdu->llctrl.enc_req.ivm));
}

static void rp_enc_state_wait_rx_enc_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case RP_ENC_EVT_ENC_REQ:
		/* Pause Tx data */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/* Pause Rx data */
		ull_conn_pause_rx_data(conn);

		/* Pause possibly paused local procedure */
		llcp_lr_pause(conn);

		rp_enc_store_m(conn, ctx, param);
		rp_enc_send_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ntf_ltk_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					  void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_ltk_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ltk_reply(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
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

static void rp_enc_state_wait_tx_start_enc_req(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_start_enc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_rx_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_START_ENC_RSP:
		rp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_start_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_start_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_encrypted(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		ctx->state = RP_ENC_STATE_WAIT_RX_PAUSE_ENC_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_rx_pause_enc_req(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_PAUSE_ENC_REQ:
		/* Pause Tx data */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_ENCRYPTION);
		/*
		 * Pause Rx data; will be resumed when the encapsulated
		 * Start Procedure is done.
		 */
		ull_conn_pause_rx_data(conn);
		rp_enc_send_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_tx_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_RUN:
		rp_enc_send_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_state_wait_rx_pause_enc_rsp(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_ENC_EVT_PAUSE_ENC_RSP:
		/* Continue with an encapsulated Start Procedure */
		/* Wait for the LL_ENC_REQ */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ;
		ctx->state = RP_ENC_STATE_WAIT_RX_ENC_REQ;

		/* Tx Encryption disabled */
		conn->lll.enc_tx = 0U;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_enc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	/* Start Procedure */
	case RP_ENC_STATE_UNENCRYPTED:
		rp_enc_state_unencrypted(conn, ctx, evt, param);
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
	/* Pause Procedure */
	case RP_ENC_STATE_ENCRYPTED:
		rp_enc_state_encrypted(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_RX_PAUSE_ENC_REQ:
		rp_enc_state_wait_rx_pause_enc_req(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_TX_PAUSE_ENC_RSP:
		rp_enc_state_wait_tx_pause_enc_rsp(conn, ctx, evt, param);
		break;
	case RP_ENC_STATE_WAIT_RX_PAUSE_ENC_RSP:
		rp_enc_state_wait_rx_pause_enc_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_enc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_ENC_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_START_ENC_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_PAUSE_ENC_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
		rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_PAUSE_ENC_RSP, pdu);
		break;
	default:
		/* Unknown opcode */

		/*
		 * BLUETOOTH CORE SPECIFICATION Version 5.3
		 * Vol 6, Part B, 5.1.3.1 Encryption Start procedure
		 *
		 * [...]
		 *
		 * If, at any time during the encryption start procedure after the Peripheral has
		 * received the LL_ENC_REQ PDU or the Central has received the
		 * LL_ENC_RSP PDU, the Link Layer of the Central or the Peripheral receives an
		 * unexpected Data Physical Channel PDU from the peer Link Layer, it shall
		 * immediately exit the Connection state, and shall transition to the Standby state.
		 * The Host shall be notified that the link has been disconnected with the error
		 * code Connection Terminated Due to MIC Failure (0x3D).
		 */

		conn->llcp_terminate.reason_final = BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
	}
}

void llcp_rp_enc_init_proc(struct proc_ctx *ctx)
{
	switch (ctx->proc) {
	case PROC_ENCRYPTION_START:
		ctx->state = RP_ENC_STATE_UNENCRYPTED;
		break;
	case PROC_ENCRYPTION_PAUSE:
		ctx->state = RP_ENC_STATE_ENCRYPTED;
		break;
	default:
		LL_ASSERT(0);
	}
}

void llcp_rp_enc_ltk_req_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_LTK_REQ_REPLY, NULL);
}

void llcp_rp_enc_ltk_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_LTK_REQ_NEG_REPLY, NULL);
}

bool llcp_rp_enc_ltk_req_reply_allowed(struct ll_conn *conn, struct proc_ctx *ctx)
{
	return (ctx->state == RP_ENC_STATE_WAIT_LTK_REPLY);
}

void llcp_rp_enc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_enc_execute_fsm(conn, ctx, RP_ENC_EVT_RUN, param);
}
#endif /* CONFIG_BT_PERIPHERAL */
