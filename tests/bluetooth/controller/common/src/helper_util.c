/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <stdlib.h>

#include <zephyr/bluetooth/hci.h>
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
#include "ll_feat.h"

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
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"
#include "ull_llcp.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn *emul_conn_pool[CONFIG_BT_MAX_CONN];

static uint32_t event_active[CONFIG_BT_MAX_CONN];
sys_slist_t ut_rx_q;
static sys_slist_t lt_tx_q;
static uint32_t no_of_ctx_buffers_at_test_setup;

#define PDU_DC_LL_HEADER_SIZE (offsetof(struct pdu_data, lldata))
#define NODE_RX_HEADER_SIZE (offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD (NODE_RX_HEADER_SIZE)
#define PDU_DATA_SIZE sizeof(struct pdu_data)
#define PDU_RX_NODE_SIZE WB_UP(NODE_RX_STRUCT_OVERHEAD + PDU_DATA_SIZE)

helper_pdu_encode_func_t *const helper_pdu_encode[] = {
	[LL_VERSION_IND] = helper_pdu_encode_version_ind,
	[LL_LE_PING_REQ] = helper_pdu_encode_ping_req,
	[LL_LE_PING_RSP] = helper_pdu_encode_ping_rsp,
	[LL_FEATURE_REQ] = helper_pdu_encode_feature_req,
	[LL_PERIPH_FEAT_XCHG] = helper_pdu_encode_peripheral_feature_req,
	[LL_FEATURE_RSP] = helper_pdu_encode_feature_rsp,
	[LL_MIN_USED_CHANS_IND] = helper_pdu_encode_min_used_chans_ind,
	[LL_REJECT_IND] = helper_pdu_encode_reject_ind,
	[LL_REJECT_EXT_IND] = helper_pdu_encode_reject_ext_ind,
	[LL_ENC_REQ] = helper_pdu_encode_enc_req,
	[LL_ENC_RSP] = helper_pdu_encode_enc_rsp,
	[LL_START_ENC_REQ] = helper_pdu_encode_start_enc_req,
	[LL_START_ENC_RSP] = helper_pdu_encode_start_enc_rsp,
	[LL_PAUSE_ENC_REQ] = helper_pdu_encode_pause_enc_req,
	[LL_PAUSE_ENC_RSP] = helper_pdu_encode_pause_enc_rsp,
	[LL_PHY_REQ] = helper_pdu_encode_phy_req,
	[LL_PHY_RSP] = helper_pdu_encode_phy_rsp,
	[LL_PHY_UPDATE_IND] = helper_pdu_encode_phy_update_ind,
	[LL_UNKNOWN_RSP] = helper_pdu_encode_unknown_rsp,
	[LL_CONNECTION_UPDATE_IND] = helper_pdu_encode_conn_update_ind,
	[LL_CONNECTION_PARAM_REQ] = helper_pdu_encode_conn_param_req,
	[LL_CONNECTION_PARAM_RSP] = helper_pdu_encode_conn_param_rsp,
	[LL_TERMINATE_IND] = helper_pdu_encode_terminate_ind,
	[LL_CHAN_MAP_UPDATE_IND] = helper_pdu_encode_channel_map_update_ind,
	[LL_LENGTH_REQ] = helper_pdu_encode_length_req,
	[LL_LENGTH_RSP] = helper_pdu_encode_length_rsp,
	[LL_CTE_REQ] = helper_pdu_encode_cte_req,
	[LL_CTE_RSP] = helper_pdu_encode_cte_rsp,
	[LL_CLOCK_ACCURACY_REQ] = helper_pdu_encode_sca_req,
	[LL_CLOCK_ACCURACY_RSP] = helper_pdu_encode_sca_rsp,
	[LL_CIS_REQ] = helper_pdu_encode_cis_req,
	[LL_CIS_RSP] = helper_pdu_encode_cis_rsp,
	[LL_CIS_IND] = helper_pdu_encode_cis_ind,
	[LL_CIS_TERMINATE_IND] = helper_pdu_encode_cis_terminate_ind,
	[LL_PERIODIC_SYNC_IND] = helper_pdu_encode_periodic_sync_ind,
	[LL_ZERO] = helper_pdu_encode_zero,
};

helper_pdu_verify_func_t *const helper_pdu_verify[] = {
	[LL_VERSION_IND] = helper_pdu_verify_version_ind,
	[LL_LE_PING_REQ] = helper_pdu_verify_ping_req,
	[LL_LE_PING_RSP] = helper_pdu_verify_ping_rsp,
	[LL_FEATURE_REQ] = helper_pdu_verify_feature_req,
	[LL_PERIPH_FEAT_XCHG] = helper_pdu_verify_peripheral_feature_req,
	[LL_FEATURE_RSP] = helper_pdu_verify_feature_rsp,
	[LL_MIN_USED_CHANS_IND] = helper_pdu_verify_min_used_chans_ind,
	[LL_REJECT_IND] = helper_pdu_verify_reject_ind,
	[LL_REJECT_EXT_IND] = helper_pdu_verify_reject_ext_ind,
	[LL_ENC_REQ] = helper_pdu_verify_enc_req,
	[LL_ENC_RSP] = helper_pdu_verify_enc_rsp,
	[LL_START_ENC_REQ] = helper_pdu_verify_start_enc_req,
	[LL_START_ENC_RSP] = helper_pdu_verify_start_enc_rsp,
	[LL_PAUSE_ENC_REQ] = helper_pdu_verify_pause_enc_req,
	[LL_PAUSE_ENC_RSP] = helper_pdu_verify_pause_enc_rsp,
	[LL_PHY_REQ] = helper_pdu_verify_phy_req,
	[LL_PHY_RSP] = helper_pdu_verify_phy_rsp,
	[LL_PHY_UPDATE_IND] = helper_pdu_verify_phy_update_ind,
	[LL_UNKNOWN_RSP] = helper_pdu_verify_unknown_rsp,
	[LL_CONNECTION_UPDATE_IND] = helper_pdu_verify_conn_update_ind,
	[LL_CONNECTION_PARAM_REQ] = helper_pdu_verify_conn_param_req,
	[LL_CONNECTION_PARAM_RSP] = helper_pdu_verify_conn_param_rsp,
	[LL_TERMINATE_IND] = helper_pdu_verify_terminate_ind,
	[LL_CHAN_MAP_UPDATE_IND] = helper_pdu_verify_channel_map_update_ind,
	[LL_LENGTH_REQ] = helper_pdu_verify_length_req,
	[LL_LENGTH_RSP] = helper_pdu_verify_length_rsp,
	[LL_CTE_REQ] = helper_pdu_verify_cte_req,
	[LL_CTE_RSP] = helper_pdu_verify_cte_rsp,
	[LL_CLOCK_ACCURACY_REQ] = helper_pdu_verify_sca_req,
	[LL_CLOCK_ACCURACY_RSP] = helper_pdu_verify_sca_rsp,
	[LL_CIS_REQ] = helper_pdu_verify_cis_req,
	[LL_CIS_RSP] = helper_pdu_verify_cis_rsp,
	[LL_CIS_IND] = helper_pdu_verify_cis_ind,
	[LL_CIS_TERMINATE_IND] = helper_pdu_verify_cis_terminate_ind,
	[LL_PERIODIC_SYNC_IND] = helper_pdu_verify_periodic_sync_ind,
};

helper_pdu_ntf_verify_func_t *const helper_pdu_ntf_verify[] = {
	[LL_VERSION_IND] = NULL,
	[LL_LE_PING_REQ] = NULL,
	[LL_LE_PING_RSP] = NULL,
	[LL_FEATURE_REQ] = NULL,
	[LL_PERIPH_FEAT_XCHG] = NULL,
	[LL_FEATURE_RSP] = NULL,
	[LL_MIN_USED_CHANS_IND] = NULL,
	[LL_REJECT_IND] = NULL,
	[LL_REJECT_EXT_IND] = NULL,
	[LL_ENC_REQ] = helper_pdu_ntf_verify_enc_req,
	[LL_ENC_RSP] = NULL,
	[LL_START_ENC_REQ] = NULL,
	[LL_START_ENC_RSP] = NULL,
	[LL_PHY_REQ] = NULL,
	[LL_PHY_RSP] = NULL,
	[LL_PHY_UPDATE_IND] = NULL,
	[LL_UNKNOWN_RSP] = NULL,
	[LL_CONNECTION_UPDATE_IND] = NULL,
	[LL_CONNECTION_PARAM_REQ] = NULL,
	[LL_CONNECTION_PARAM_RSP] = NULL,
	[LL_TERMINATE_IND] = NULL,
	[LL_CHAN_MAP_UPDATE_IND] = NULL,
	[LL_LENGTH_REQ] = NULL,
	[LL_LENGTH_RSP] = NULL,
	[LL_CTE_REQ] = NULL,
	[LL_CTE_RSP] = helper_pdu_ntf_verify_cte_rsp,
	[LL_CTE_RSP] = NULL,
	[LL_CLOCK_ACCURACY_REQ] = NULL,
	[LL_CLOCK_ACCURACY_RSP] = NULL,
	[LL_CIS_REQ] = NULL,
	[LL_CIS_RSP] = NULL,
	[LL_CIS_IND] = NULL,
	[LL_CIS_TERMINATE_IND] = NULL,
	[LL_PERIODIC_SYNC_IND] = NULL,
};

helper_node_encode_func_t *const helper_node_encode[] = {
	[LL_VERSION_IND] = NULL,
	[LL_LE_PING_REQ] = NULL,
	[LL_LE_PING_RSP] = NULL,
	[LL_FEATURE_REQ] = NULL,
	[LL_PERIPH_FEAT_XCHG] = NULL,
	[LL_FEATURE_RSP] = NULL,
	[LL_MIN_USED_CHANS_IND] = NULL,
	[LL_REJECT_IND] = NULL,
	[LL_REJECT_EXT_IND] = NULL,
	[LL_ENC_REQ] = NULL,
	[LL_ENC_RSP] = NULL,
	[LL_START_ENC_REQ] = NULL,
	[LL_START_ENC_RSP] = NULL,
	[LL_PHY_REQ] = NULL,
	[LL_PHY_RSP] = NULL,
	[LL_PHY_UPDATE_IND] = NULL,
	[LL_UNKNOWN_RSP] = NULL,
	[LL_CONNECTION_UPDATE_IND] = NULL,
	[LL_CONNECTION_PARAM_REQ] = NULL,
	[LL_CONNECTION_PARAM_RSP] = NULL,
	[LL_TERMINATE_IND] = NULL,
	[LL_CHAN_MAP_UPDATE_IND] = NULL,
	[LL_CTE_REQ] = NULL,
	[LL_CTE_RSP] = helper_node_encode_cte_rsp,
	[LL_CLOCK_ACCURACY_REQ] = NULL,
	[LL_CLOCK_ACCURACY_RSP] = NULL,
	[LL_CIS_REQ] = NULL,
	[LL_CIS_RSP] = NULL,
	[LL_CIS_IND] = NULL,
	[LL_CIS_TERMINATE_IND] = NULL,
	[LL_PERIODIC_SYNC_IND] = NULL,
};

helper_node_verify_func_t *const helper_node_verify[] = {
	[NODE_PHY_UPDATE] = helper_node_verify_phy_update,
	[NODE_CONN_UPDATE] = helper_node_verify_conn_update,
	[NODE_ENC_REFRESH] = helper_node_verify_enc_refresh,
	[NODE_CTE_RSP] = helper_node_verify_cte_rsp,
	[NODE_CIS_REQUEST] = helper_node_verify_cis_request,
	[NODE_CIS_ESTABLISHED] = helper_node_verify_cis_established,
	[NODE_PEER_SCA_UPDATE] = helper_node_verify_peer_sca_update,
};

/*
 * for debugging purpose only
 */
void test_print_conn(struct ll_conn *conn)
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

uint16_t test_ctx_buffers_cnt(void)
{
	return no_of_ctx_buffers_at_test_setup;
}

static uint8_t find_idx(struct ll_conn *conn)
{
	for (uint8_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (emul_conn_pool[i] == conn) {
			return i;
		}
	}
	zassert_true(0, "Invalid connection object");
	return 0xFF;
}

void test_setup(struct ll_conn *conn)
{
	ull_conn_init();

	/**/
	memset(conn, 0x00, sizeof(*conn));

	/* Initialize the upper test rx queue */
	sys_slist_init(&ut_rx_q);

	/* Initialize the lower tester tx queue */
	sys_slist_init(&lt_tx_q);

	/* Initialize the control procedure code */
	ull_cp_init();

	/* Initialize the ULL TX Q */
	ull_tx_q_init(&conn->tx_q);

	/* Initialize the connection object */
	ull_llcp_init(conn);

	ll_reset();
	conn->lll.event_counter = 0;
	conn->lll.interval = 6;
	conn->supervision_timeout = 600;
	event_active[0] = 0;

	memset(emul_conn_pool, 0x00, sizeof(emul_conn_pool));
	emul_conn_pool[0] = conn;

	no_of_ctx_buffers_at_test_setup = llcp_ctx_buffers_free();

}

void test_setup_idx(struct ll_conn *conn, uint8_t idx)
{
	if (idx == 0) {
		test_setup(conn);
		return;
	}

	memset(conn, 0x00, sizeof(*conn));

	/* Initialize the ULL TX Q */
	ull_tx_q_init(&conn->tx_q);

	/* Initialize the connection object */
	ull_llcp_init(conn);

	conn->lll.event_counter = 0;
	event_active[idx] = 0;
	emul_conn_pool[idx] = conn;
}

void test_set_role(struct ll_conn *conn, uint8_t role)
{
	conn->lll.role = role;
}

void event_prepare(struct ll_conn *conn)
{
	struct lll_conn *lll = &conn->lll;
	uint32_t *evt_active = &(event_active[find_idx(conn)]);

	/* Can only be called with no active event */
	zassert_equal(*evt_active, 0, "Called inside an active event");
	*evt_active = 1;

	/*** ULL Prepare ***/

	/* Event counter */
	conn->event_counter = lll->event_counter + lll->latency_prepare;

	/* Handle any LL Control Procedures */
	ull_cp_run(conn);

	/*** LLL Prepare ***/

	/* Save the latency for use in event */
	lll->latency_prepare += lll->latency;

	/* Calc current event counter value */
	uint16_t event_counter = lll->event_counter + lll->latency_prepare;

	/* Store the next event counter value */
	lll->event_counter = event_counter + 1;

	lll->latency_prepare = 0;
}

void event_tx_ack(struct ll_conn *conn, struct node_tx *tx)
{
	uint32_t *evt_active = &(event_active[find_idx(conn)]);
	/* Can only be called with active event */
	zassert_equal(*evt_active, 1, "Called outside an active event");

	ull_cp_tx_ack(conn, tx);
}

void event_done(struct ll_conn *conn)
{
	struct node_rx_pdu *rx;
	uint32_t *evt_active = &(event_active[find_idx(conn)]);

	/* Can only be called with active event */
	zassert_equal(*evt_active, 1, "Called outside an active event");
	*evt_active = 0;

	/* Notify all control procedures that wait with Host notifications for instant to be on
	 * air. This is done here because UT does not maintain actual connection events.
	 */
	ull_cp_tx_ntf(conn);

	while ((rx = (struct node_rx_pdu *)sys_slist_get(&lt_tx_q))) {

		/* Mark buffer for release */
		rx->hdr.type = NODE_RX_TYPE_RELEASE;

		ull_cp_rx(conn, NULL, rx);

		if (rx->hdr.type == NODE_RX_TYPE_RELEASE) {
			/* Only release if node was not hi-jacked by LLCP */
			ll_rx_release(rx);
		} else if (rx->hdr.type != NODE_RX_TYPE_RETAIN) {
			/* Otherwise put/sched to emulate ull_cp_rx return path */
			ll_rx_put_sched(rx->hdr.link, rx);
		}
	}
}

uint16_t event_counter(struct ll_conn *conn)
{
	uint16_t event_counter;
	uint32_t *evt_active = &(event_active[find_idx(conn)]);

	/* Calculate current event counter */
	event_counter = ull_conn_event_counter(conn);

	/* If event_counter is called inside an event_prepare()/event_done() pair
	 * return the current event counter value (i.e. -1);
	 * otherwise return the next event counter value
	 */
	if (*evt_active) {
		event_counter--;
	}

	return event_counter;
}

static struct node_rx_pdu *rx_malloc_store;

void lt_tx_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		struct ll_conn *conn, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *rx;

	rx = malloc(PDU_RX_NODE_SIZE);
	zassert_not_null(rx, "Out of memory.\nCalled at %s:%d\n", file, line);

	/* Remember RX node to allow for correct release */
	rx_malloc_store = rx;

	/* Encode node_rx_pdu if required by particular procedure */
	if (helper_node_encode[opcode]) {
		helper_node_encode[opcode](rx, param);
	}

	pdu = (struct pdu_data *)rx->pdu;
	zassert_not_null(helper_pdu_encode[opcode], "PDU encode function cannot be NULL\n");
	helper_pdu_encode[opcode](pdu, param);

	sys_slist_append(&lt_tx_q, (sys_snode_t *)rx);
}

void lt_tx_real_no_encode(const char *file, uint32_t line, struct pdu_data *pdu,
			  struct ll_conn *conn, void *param)
{
	struct node_rx_pdu *rx;

	rx = malloc(PDU_RX_NODE_SIZE);
	zassert_not_null(rx, "Out of memory.\nCalled at %s:%d\n", file, line);

	/* Remember RX node to allow for correct release */
	rx_malloc_store = rx;

	memcpy((struct pdu_data *)rx->pdu, pdu, sizeof(struct pdu_data));
	sys_slist_append(&lt_tx_q, (sys_snode_t *)rx);
}

void release_ntf(struct node_rx_pdu *ntf)
{
	if (ntf == rx_malloc_store) {
		free(ntf);
		return;
	}

	ntf->hdr.next = NULL;
	ll_rx_mem_release((void **)&ntf);
}

void lt_rx_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		struct ll_conn *conn, struct node_tx **tx_ref, void *param)
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

void lt_rx_q_is_empty_real(const char *file, uint32_t line, struct ll_conn *conn)
{
	struct node_tx *tx;

	tx = ull_tx_q_dequeue(&conn->tx_q);
	zassert_is_null(tx, "Tx Q not empty.\nCalled at %s:%d\n", file, line);
}

void ut_rx_pdu_real(const char *file, uint32_t line, enum helper_pdu_opcode opcode,
		    struct node_rx_pdu **ntf_ref, void *param)
{
	struct pdu_data *pdu;
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *)sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU,
		      "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	pdu = (struct pdu_data *)ntf->pdu;
	if (helper_pdu_ntf_verify[opcode]) {
		helper_pdu_ntf_verify[opcode](file, line, pdu, param);
	} else if (helper_pdu_verify[opcode]) {
		helper_pdu_verify[opcode](file, line, pdu, param);
	}

	*ntf_ref = ntf;
}

void ut_rx_node_real(const char *file, uint32_t line, enum helper_node_opcode opcode,
		     struct node_rx_pdu **ntf_ref, void *param)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *)sys_slist_get(&ut_rx_q);
	zassert_not_null(ntf, "Ntf Q empty.\nCalled at %s:%d\n", file, line);

	zassert_not_equal(ntf->hdr.type, NODE_RX_TYPE_DC_PDU,
			  "Ntf node is of the wrong type.\nCalled at %s:%d\n", file, line);

	if (helper_node_verify[opcode]) {
		helper_node_verify[opcode](file, line, ntf, param);
	}

	*ntf_ref = ntf;
}

void ut_rx_q_is_empty_real(const char *file, uint32_t line)
{
	struct node_rx_pdu *ntf;

	ntf = (struct node_rx_pdu *)sys_slist_get(&ut_rx_q);
	zassert_is_null(ntf, "Ntf Q not empty.\nCalled at %s:%d\n", file, line);
}

void encode_pdu(enum helper_pdu_opcode opcode, struct pdu_data *pdu, void *param)
{
	zassert_not_null(helper_pdu_encode[opcode], "PDU encode function cannot be NULL\n");
	helper_pdu_encode[opcode](pdu, param);
}
