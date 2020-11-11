/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#define ULL_LLCP_UNITTEST

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

#include "helper_pdu.h"
#include "helper_util.h"

static struct ull_cp_conn conn;

static void setup(void)
{
	test_setup(&conn);
}

static bool is_instant_reached(struct ull_cp_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_RSP |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_mas_loc_accept(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |         LL_REJECT_EXT_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_mas_loc_reject(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Controller do not
 * support Connection Parameters Request procedure.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |            LL_UNKNOWN_RSP |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_mas_loc_unsupp_feat(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ
	};

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * (A)
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * and
 *
 * (B)
 * Slave-initiated Connection Parameters Request procedure.
 * Procedure collides and is rejected.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   | (A)
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ | (B)
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                <--------------------->                |
 *    |                < PROCEDURE COLLISION >                |
 *    |                <--------------------->                |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         | (B)
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_RSP | (A)
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  | (A)
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_mas_loc_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* (A) Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/**/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/**/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (A) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (A) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}


/*
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_mas_rem_accept(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);


	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Negative Reply            |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
void test_conn_update_mas_rem_reject(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_neg_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);
}

/* Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Controller do not
 * support Connection Parameters Request procedure.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           | LL_UNKNOWN_RSP            |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
void test_conn_update_mas_rem_unsupp_feat(void)
{
	/* TODO(thoh): Implement when Remote Request machine has feature
	 * checking */
}

/*
 * (A)
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host accepts.
 *
 * and
 *
 * (B)
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * NOTE:
 * Master-initiated Connection Parameters Request procedure is paused.
 * Slave-initiated Connection Parameters Request procedure is finished.
 * Master-initiated Connection Parameters Request procedure is resumed.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------| (A)
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           | (B)
 *    |                           |                           |
 *    |               <------------------------>              |
 *    |               < LOCAL PROCEDURE PAUSED >              |
 *    |               <------------------------>              |
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           | (A)
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           | (A)
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->| (A)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (A)
 *    |                           |                           |
 *    |              <------------------------->              |
 *    |              < LOCAL PROCEDURE RESUMED >              |
 *    |              <------------------------->              |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->| (B)
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_RSP |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->| (B)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (B)
 *    |                           |                           |
 */
void test_conn_update_mas_rem_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_MASTER);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (B) Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (A) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	/* (A) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (A) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (A) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (B) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (B) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (B) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_sla_loc_accept(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_conn_update_ind conn_update_ind = {0};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	conn_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |         LL_REJECT_EXT_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_sla_loc_reject(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Controller do not
 * support Connection Parameters Request procedure.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |            LL_UNKNOWN_RSP |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_sla_loc_unsupp_feat(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * (A)
 * Slave-initiated Connection Parameters Request procedure.
 * Procedure collides and is rejected.
 *
 * and
 *
 * (B)
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           | (A)
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->| (A)
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    |                <--------------------->                |
 *    |                < PROCEDURE COLLISION >                |
 *    |                <--------------------->                |
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           | (B)
 *    |                           |                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           | (B)
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->| (A)
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (A)
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (B)
 */
void test_conn_update_sla_loc_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu1 = {
		.status = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct node_rx_pu cu2 = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct pdu_data_llctrl_conn_update_ind conn_update_ind = {0};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* (A) Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/*******************/

	/* (B) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	/* (B) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu1);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx */
	conn_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (B) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (B) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (B) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu2);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_RSP   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
void test_conn_update_sla_rem_accept(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_conn_update_ind conn_update_ind = {0};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	conn_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Negative Reply            |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
void test_conn_update_sla_rem_reject(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_neg_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);
}

/*
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Controller do not
 * support Connection Parameters Request procedure.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           | LL_UNKNOWN_RSP            |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
void test_conn_update_sla_rem_unsupp_feat(void)
{
	/* TODO(thoh): Implement when Remote Request machine has feature
	 * checking */
}

/*
 * (A)
 * Master-initiated Connection Parameters Request procedure.
 * Master requests change in LE connection parameters, slave’s Host accepts.
 *
 * and
 *
 * (B)
 * Slave-initiated Connection Parameters Request procedure.
 * Slave requests change in LE connection parameters, master’s Host accepts.
 *
 * NOTE:
 * Slave-initiated Connection Parameters Request procedure is paused.
 * Master-initiated Connection Parameters Request procedure is finished.
 * Slave-initiated Connection Parameters Request procedure is resumed.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------| (A)
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           | (B)
 *    |                           |                           |
 *    |               <------------------------>              |
 *    |               < LOCAL PROCEDURE PAUSED >              |
 *    |               <------------------------>              |
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           | (A)
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           | (A)
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_RSP   |
 *    |                           |-------------------------->| (A)
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------| (A)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (A)
 *    |                           |                           |
 *    |              <------------------------->              |
 *    |              < LOCAL PROCEDURE RESUMED >              |
 *    |              <------------------------->              |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->| (B)
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (B)
 *    |                           |                           |
 */
void test_conn_update_sla_rem_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = {
		.status = BT_HCI_ERR_SUCCESS
	};

	struct pdu_data_llctrl_conn_update_ind conn_update_ind = {0};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_SLAVE);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (B) Initiate an Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, 0, 0, 0, 0);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (A) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/*******************/

	/* (A) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	conn_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (A) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (A) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	conn_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* (B) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (B) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (B) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
}


void test_main(void)
{

	ztest_test_suite(mas_loc,
			 ztest_unit_test_setup_teardown(test_conn_update_mas_loc_accept, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_loc_reject, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_loc_unsupp_feat, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_loc_collision, setup, unit_test_noop)
			);

	ztest_test_suite(mas_rem,
			 ztest_unit_test_setup_teardown(test_conn_update_mas_rem_accept, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_rem_reject, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_rem_unsupp_feat, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_mas_rem_collision, setup, unit_test_noop)
			);

	ztest_test_suite(sla_loc,
			 ztest_unit_test_setup_teardown(test_conn_update_sla_loc_accept, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_loc_reject, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_loc_unsupp_feat, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_loc_collision, setup, unit_test_noop)
			);

	ztest_test_suite(sla_rem,
			 ztest_unit_test_setup_teardown(test_conn_update_sla_rem_accept, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_rem_reject, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_rem_unsupp_feat, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_conn_update_sla_rem_collision, setup, unit_test_noop)
			);

	ztest_run_test_suite(mas_loc);
	ztest_run_test_suite(mas_rem);
	ztest_run_test_suite(sla_loc);
	ztest_run_test_suite(sla_rem);
}
