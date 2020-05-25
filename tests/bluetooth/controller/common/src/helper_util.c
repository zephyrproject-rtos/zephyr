/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/types.h"
#include "ztest.h"
#include <stdlib.h>
#include "kconfig.h"

#include <bluetooth/hci.h>
#include <sys/slist.h>
#include <sys/util.h>

#include "hal/ccm.h"

#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"
#include "ll_feat.h"

#include "lll.h"
#include "lll_conn.h"

#include "ull_conn_types.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"

#include "helper_pdu.h"
#include "helper_util.h"

static u32_t event_active;
static u16_t lazy;
sys_slist_t ut_rx_q;
static sys_slist_t lt_tx_q;


#define PDU_DC_LL_HEADER_SIZE	(offsetof(struct pdu_data, lldata))
#define NODE_RX_HEADER_SIZE	(offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD	(NODE_RX_HEADER_SIZE)
#define PDU_DATA_SIZE		(PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)
#define PDU_RX_NODE_SIZE WB_UP(NODE_RX_STRUCT_OVERHEAD + PDU_DATA_SIZE)

helper_pdu_encode_func_t * const helper_pdu_encode[] = {
	helper_pdu_encode_version_ind,
	helper_pdu_encode_ping_req,
	helper_pdu_encode_ping_rsp,
	helper_pdu_encode_feature_req,
	helper_pdu_encode_slave_feature_req,
	helper_pdu_encode_feature_rsp,
	NULL,
	helper_pdu_encode_reject_ext_ind,
	helper_pdu_encode_enc_req,
	helper_pdu_encode_enc_rsp,
	helper_pdu_encode_start_enc_req,
	helper_pdu_encode_start_enc_rsp,
	helper_pdu_encode_phy_req,
	helper_pdu_encode_phy_rsp,
	helper_pdu_encode_phy_update_ind,
	helper_pdu_encode_unknown_rsp,
};

helper_pdu_verify_func_t *const helper_pdu_verify[] = {
	helper_pdu_verify_version_ind,
	helper_pdu_verify_ping_req,
	helper_pdu_verify_ping_rsp,
	helper_pdu_verify_feature_req,
	helper_pdu_verify_slave_feature_req,
	helper_pdu_verify_feature_rsp,
	helper_pdu_verify_reject_ind,
	helper_pdu_verify_reject_ext_ind,
	helper_pdu_verify_enc_req,
	helper_pdu_verify_enc_rsp,
	helper_pdu_verify_start_enc_req,
	helper_pdu_verify_start_enc_rsp,
	helper_pdu_verify_phy_req,
	helper_pdu_verify_phy_rsp,
	helper_pdu_verify_phy_update_ind,
	helper_pdu_verify_unknown_rsp,
};


helper_node_verify_func_t * const helper_node_verify[] = {
	helper_node_verify_phy_update,
};

/*
 * for debugging purpose only
 */
void test_print_conn(struct ull_cp_conn *conn)
{
	printf("------------------>\n");
	printf("Mock structure\n");
	printf("     Role: %d\n", conn->lll.role);
	printf("     event-count: %d\n", conn->lll.event_counter);
	printf("LLCP structure\n");
	printf("     Local state: %d\n", conn->llcp.local.state);
	printf("     Remote state: %d\n", conn->llcp.remote.state);
	printf("     Collision: %d\n", conn->llcp.remote.collision);
	printf("     Reject: %d\n", conn->llcp.remote.reject_opcode);
	printf("--------------------->\n");

}
void test_setup(struct ull_cp_conn *conn)
{
	memset(conn, 0, sizeof(struct ull_cp_conn));
		/* Initialize the upper test rx queue */
	sys_slist_init(&ut_rx_q);

	/* Initialize the lower tester tx queue */
	sys_slist_init(&lt_tx_q);

	/* Initialize the control procedure code */
	ull_cp_init();

	/* Initialize the ULL TX Q */
	ull_tx_q_init(&conn->tx_q);

	/* Initialize the connection object */
	ull_cp_conn_init(conn);

	event_active = 0;
	lazy = 0;
}

void test_set_role(struct ull_cp_conn *conn, u8_t role)
{
	conn->lll.role = role;
}


void event_prepare(struct ull_cp_conn *conn)
{
	struct mocked_lll_conn *lll;

	/* Can only be called with no active event */
	zassert_equal(event_active, 0, "Called inside an active event");
	event_active = 1;

	/*** ULL Prepare ***/

	/* Handle any LL Control Procedures */
	ull_cp_run(conn);

	/*** LLL Prepare ***/
	lll = &conn->lll;

	/* Save the latency for use in event */
	lll->latency_prepare += lazy;

	/* Calc current event counter value */
	u16_t event_counter = lll->event_counter + lll->latency_prepare;

	/* Store the next event counter value */
	lll->event_counter = event_counter + 1;

	lll->latency_prepare = 0;

	/* Rest lazy */
	lazy = 0;
}

void event_done(struct ull_cp_conn *conn)
{
	struct node_rx_pdu *rx;

	/* Can only be called with active event */
	zassert_equal(event_active, 1, "Called outside an active event");
	event_active = 0;

	while ((rx = (struct node_rx_pdu *) sys_slist_get(&lt_tx_q))) {
		ull_cp_rx(conn, rx);
		free(rx);
	}
}

u16_t event_counter(struct ull_cp_conn *conn)
{
	/* TODO(thoh): Mocked lll_conn */
	struct mocked_lll_conn *lll;
	u16_t event_counter;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	/* If event_counter is called inside an event_prepare()/event_done() pair
	 * return the current event counter value (i.e. -1);
	 * otherwise return the next event counter value */
	if (event_active)
		event_counter--;

	return event_counter;
}


void lt_tx_real(const char *file, u32_t line, helper_pdu_opcode_t opcode, struct ull_cp_conn *conn, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *rx;

	rx = malloc(PDU_RX_NODE_SIZE);
	zassert_not_null(rx,  "Out of memory.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *) rx->pdu;
	zassert_not_null(helper_pdu_encode[opcode], "PDU encode function cannot be NULL\n");
	helper_pdu_encode[opcode](pdu, param);

	sys_slist_append(&lt_tx_q, (sys_snode_t *) rx);
}


void lt_rx_real(const char *file, u32_t line, helper_pdu_opcode_t opcode, struct ull_cp_conn *conn, struct node_tx **tx_ref, void *param)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	tx = ull_tx_q_dequeue(&conn->tx_q);

	zassert_not_null(tx, "Tx Q empty.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *)tx->pdu;
	if (helper_pdu_verify[opcode]) {
		helper_pdu_verify[opcode](file, line, pdu, param);
	}

	*tx_ref = tx;
}


void lt_rx_q_is_empty_real(const char *file, u32_t line, struct ull_cp_conn *conn)
{
	struct node_tx *tx;

	tx = ull_tx_q_dequeue(&conn->tx_q);
	zassert_is_null(tx, "Tx Q not empty.\nCalled at %s:%d\n", file, line);
}



void ut_rx_pdu_real(const char *file, u32_t line, helper_pdu_opcode_t opcode, struct node_rx_pdu **ntf_ref, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU, "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *) ntf->pdu;
	if (helper_pdu_verify[opcode]) {
		helper_pdu_verify[opcode](file, line, pdu, param);
	}

	*ntf_ref = ntf;
}


void ut_rx_node_real(const char *file, u32_t line, helper_node_opcode_t opcode, struct node_rx_pdu **ntf_ref, void *param)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_not_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU, "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	if (helper_node_verify[opcode]) {
		helper_node_verify[opcode](file, line, ntf, param);
	}

	*ntf_ref = ntf;
}


void ut_rx_q_is_empty_real(const char *file, u32_t line)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *) sys_slist_get(&ut_rx_q);
	zassert_is_null(ntf, "Ntf Q not empty.\nCalled at %s:%d\n", file, line);
}
