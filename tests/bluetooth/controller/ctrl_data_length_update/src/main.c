/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
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

#include "ull_tx_queue.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"


#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);
}


/*
 * Locally triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *    | Data Length Update Proc.   |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_mas_loc(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = {27,328,251,2120};
	struct pdu_data_llctrl_length_rsp remote_length_rsp = {27,328,251,2120};

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 251, 2120 );
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf,  &remote_length_rsp);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}

void test_data_length_update_sla_loc(void)
{
	uint64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 27, 328, 251, 2120};
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 27, 328, 251, 2120};

	test_set_role(&conn, BT_HCI_ROLE_SLAVE);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 251, 2120 );
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf,  &remote_length_rsp);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1,
		      "Wrong event-count %d\n", conn.lll.event_counter);
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            |                              |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_mas_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req =  { 27, 328, 251, 2120};
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 27, 328, 251, 2120};

	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf,  &local_length_rsp);
	ut_rx_q_is_empty();
}

/*
 * Locally triggered Data Length Update procedure using remote REQ/RSP piggy back
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            |                              |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_mas_rem_2(void)
{
#if 0
	struct node_tx *tx;
	uint64_t err;
	struct pdu_data_llctrl_length_req remote_length_req =  { 27, 328, 251, 2120};
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 27, 328, 251, 2120};

	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_MASTER);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 251, 2120 );
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);


	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf,  &local_length_rsp);
	ut_rx_q_is_empty();
#endif
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            |                              |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |                            |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_sla_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req =  { 27, 328, 251, 2120};
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 27, 328, 251, 2120};

	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_SLAVE);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf,  &local_length_rsp);
	ut_rx_q_is_empty();
}

void test_main(void)
{
	ztest_test_suite(data_length_update_master,
			 ztest_unit_test_setup_teardown(test_data_length_update_mas_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_data_length_update_mas_rem, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_data_length_update_mas_rem_2, setup, unit_test_noop)
		);

	ztest_test_suite(data_length_update_slave,
			 ztest_unit_test_setup_teardown(test_data_length_update_sla_loc, setup, unit_test_noop),
			 ztest_unit_test_setup_teardown(test_data_length_update_sla_rem, setup, unit_test_noop)
		);

	ztest_run_test_suite(data_length_update_master);
	ztest_run_test_suite(data_length_update_slave);

}
