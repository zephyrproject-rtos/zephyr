/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#define ULL_LLCP_UNITTEST

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
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

/* Default connection values */
#define INTVL_MIN 6U /* multiple of 1.25 ms (min 6, max 3200) */
#define INTVL_MAX 6U /* multiple of 1.25 ms (min 6, max 3200) */
#define LATENCY 1U
#define TIMEOUT 10U /* multiple of 10 ms (min 10, max 3200) */

/* Default conn_update_ind PDU */
struct pdu_data_llctrl_conn_update_ind conn_update_ind = { .win_size = 1U,
							   .win_offset = 0U,
							   .interval = INTVL_MAX,
							   .latency = LATENCY,
							   .timeout = TIMEOUT,
							   .instant = 6U };

/* Default conn_param_req PDU */
struct pdu_data_llctrl_conn_param_req conn_param_req = { .interval_min = INTVL_MIN,
							 .interval_max = INTVL_MAX,
							 .latency = LATENCY,
							 .timeout = TIMEOUT,
							 .preferred_periodicity = 0U,
							 .reference_conn_event_count = 0u,
							 .offset0 = 0x0000U,
							 .offset1 = 0xffffU,
							 .offset2 = 0xffffU,
							 .offset3 = 0xffffU,
							 .offset4 = 0xffffU,
							 .offset5 = 0xffffU };

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/* Default conn_param_rsp PDU */
struct pdu_data_llctrl_conn_param_rsp conn_param_rsp = { .interval_min = INTVL_MIN,
							 .interval_max = INTVL_MAX,
							 .latency = LATENCY,
							 .timeout = TIMEOUT,
							 .preferred_periodicity = 0U,
							 .reference_conn_event_count = 0u,
							 .offset0 = 0x0000U,
							 .offset1 = 0xffffU,
							 .offset2 = 0xffffU,
							 .offset3 = 0xffffU,
							 .offset4 = 0xffffU,
							 .offset5 = 0xffffU };

/* Invalid conn_param_req PDU */
struct pdu_data_llctrl_conn_param_req conn_param_req_invalid = { .interval_min = INTVL_MIN - 1,
							 .interval_max = INTVL_MAX + 1,
							 .latency = LATENCY,
							 .timeout = TIMEOUT - 1,
							 .preferred_periodicity = 0U,
							 .reference_conn_event_count = 0u,
							 .offset0 = 0x0000U,
							 .offset1 = 0xffffU,
							 .offset2 = 0xffffU,
							 .offset3 = 0xffffU,
							 .offset4 = 0xffffU,
							 .offset5 = 0xffffU };
/* Invalid conn_param_rsp PDU */
struct pdu_data_llctrl_conn_param_rsp conn_param_rsp_invalid = { .interval_min = INTVL_MIN - 1,
								 .interval_max = INTVL_MAX + 1,
								 .latency = LATENCY,
								 .timeout = TIMEOUT - 1,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x0000U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };
/* Different PDU contents for (B) */

/* Default conn_param_req PDU (B) */
struct pdu_data_llctrl_conn_param_req conn_param_req_B = {
	.interval_min = INTVL_MIN,
	.interval_max = INTVL_MAX,
	.latency = LATENCY + 1U, /* differentiate parameter */
	.timeout = TIMEOUT + 1U, /* differentiate parameter */
	.preferred_periodicity = 0U,
	.reference_conn_event_count = 0u,
	.offset0 = 0x0000U,
	.offset1 = 0xffffU,
	.offset2 = 0xffffU,
	.offset3 = 0xffffU,
	.offset4 = 0xffffU,
	.offset5 = 0xffffU
};

/* Default conn_param_rsp PDU (B) */
struct pdu_data_llctrl_conn_param_rsp conn_param_rsp_B = {
	.interval_min = INTVL_MIN,
	.interval_max = INTVL_MAX,
	.latency = LATENCY + 1U, /* differentiate parameter */
	.timeout = TIMEOUT + 1U, /* differentiate parameter */
	.preferred_periodicity = 0U,
	.reference_conn_event_count = 0u,
	.offset0 = 0x0000U,
	.offset1 = 0xffffU,
	.offset2 = 0xffffU,
	.offset3 = 0xffffU,
	.offset4 = 0xffffU,
	.offset5 = 0xffffU
};
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

/* Default conn_update_ind PDU (B) */
struct pdu_data_llctrl_conn_update_ind conn_update_ind_B = {
	.win_size = 1U,
	.win_offset = 0U,
	.interval = INTVL_MAX,
	.latency = LATENCY + 1U, /* differentiate parameter */
	.timeout = TIMEOUT + 1U, /* differentiate parameter */
	.instant = 6U
};

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
struct pdu_data_llctrl_conn_param_req *req_B = &conn_param_req_B;
struct pdu_data_llctrl_conn_param_rsp *rsp_B = &conn_param_rsp_B;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

struct pdu_data_llctrl_conn_update_ind *cu_ind_B = &conn_update_ind_B;

static struct ll_conn conn;


#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
bool ull_handle_cpr_anchor_point_move(struct ll_conn *conn, uint16_t *offsets, uint8_t *status)
{
	ztest_copy_return_data(status, 1);
	return ztest_get_return_value();
}
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */

static void test_unmask_feature_conn_param_req(struct ll_conn *conn)
{
	conn->llcp.fex.features_used &= ~BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ);
}

static bool test_get_feature_conn_param_req(struct ll_conn *conn)
{
	return (conn->llcp.fex.features_used & BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ));
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void conn_update_setup(void *data)
{
	test_setup(&conn);

	conn_param_req.reference_conn_event_count = -1;
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	conn_param_rsp.reference_conn_event_count = -1;
#endif

	/* Initialize lll conn parameters (different from new) */
	struct lll_conn *lll = &conn.lll;

	lll->interval = 0;
	lll->latency = 0;
	conn.supervision_timeout = 1U;
	lll->event_counter = 0;
}

static bool is_instant_reached(struct ll_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_loc, test_conn_update_central_loc_accept)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	conn_param_rsp.reference_conn_event_count = conn_param_req.reference_conn_event_count;
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, &conn_param_rsp);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}


/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 * Parallel CPRs attemtped and rejected/cached
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~  parallel remote CPR is attempted and rejected   ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_RSP |
 *    |                           |<--------------------------|
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~  parallel remote CPR is attempted and rejected   ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~    parallel local CPR is attempted and cached    ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~    parallel local CPR is now started    ~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 */
ZTEST(central_loc, test_conn_update_central_loc_accept_reject_2nd_cpr)
{
	struct ll_conn conn_2nd;
	struct ll_conn conn_3rd;
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL
	};
	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Initialize extra connections */
	test_setup_idx(&conn_2nd, 1);
	test_setup_idx(&conn_3rd, 2);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Role */
	test_set_role(&conn_2nd, BT_HCI_ROLE_PERIPHERAL);
	/* Role */
	test_set_role(&conn_3rd, BT_HCI_ROLE_PERIPHERAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Connect */
	ull_cp_state_set(&conn_2nd, ULL_CP_CONNECTED);

	/* Connect */
	ull_cp_state_set(&conn_3rd, ULL_CP_CONNECTED);
	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Now CPR is active on 'conn' so let 'conn_2nd' attempt to start a CPR */
	/* Prepare */
	event_prepare(&conn_2nd);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn_2nd);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn_2nd, &conn_param_req);

	/* Done */
	event_done(&conn_2nd);

	/* Prepare */
	event_prepare(&conn_2nd);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn_2nd, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn_2nd);

	/* Done */
	event_done(&conn_2nd);

	/* Release Tx */
	ull_cp_release_tx(&conn_2nd, tx);

	/* Rx */
	conn_param_rsp.reference_conn_event_count = conn_param_req.reference_conn_event_count;
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, &conn_param_rsp);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Now CPR is active on 'conn' so let 'conn_2nd' attempt to start a CPR again */
	/* Prepare */
	event_prepare(&conn_3rd);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn_3rd);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn_3rd, &conn_param_req);

	/* Done */
	event_done(&conn_3rd);

	/* Prepare */
	event_prepare(&conn_3rd);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn_3rd, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn_3rd);

	/* Done */
	event_done(&conn_3rd);

	/* Release Tx */
	ull_cp_release_tx(&conn_3rd, tx);

	/* Initiate a parallel Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn_3rd, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn_3rd);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn_3rd);

	/* Done */
	event_done(&conn_3rd);

	/* Prepare */
	event_prepare(&conn_3rd);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn_3rd);

	/* Done */
	event_done(&conn_3rd);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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

		/* Prepare on conn_3rd for parallel CPR */
		event_prepare(&conn_3rd);

		/* Tx Queue should have no LL Control PDU */
		lt_rx_q_is_empty(&conn_3rd);

		/* Done on conn_3rd for parallel CPR */
		event_done(&conn_3rd);
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

	/* Now the locally initiated CPR on conn_3rd should be allowed to run */
	/* Prepare */
	event_prepare(&conn_3rd);

	/* Tx Queue should have one LL Control PDU, indicating parallel CPR is now active */
	conn_param_req.reference_conn_event_count = event_counter(&conn_3rd);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn_3rd, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn_3rd);

	/* Done */
	event_done(&conn_3rd);

	/* Release Tx */
	ull_cp_release_tx(&conn_3rd, tx);

	/* Release Ntf */
	release_ntf(ntf);

	/* One less CTXs as the conn_3rd CPR is still 'running' */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-1,
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral
 * responds with invalid params
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(central_loc, test_conn_update_central_loc_invalid_param_rsp)
{
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
		.error_code = BT_HCI_ERR_INVALID_LL_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, &conn_param_rsp_invalid);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_PARAM_REQ   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_REJECT_IND           |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(central_loc, test_conn_update_central_loc_invalid_rsp)
{
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_loc, test_conn_update_central_loc_reject)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	struct node_rx_pu cu = { .status = BT_HCI_ERR_UNACCEPT_CONN_PARAM };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host is legacy.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_loc, test_conn_update_central_loc_remote_legacy)
{
	bool feature_bit_param_req;
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Check that feature Param Reg. is unmasked */
	feature_bit_param_req = test_get_feature_conn_param_req(&conn);
	zassert_equal(feature_bit_param_req, false, "Feature bit not unmasked");

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Controller do not
 * support Connection Parameters Request procedure, features not exchanged.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_loc, test_conn_update_central_loc_unsupp_wo_feat_exch)
{
	bool feature_bit_param_req;
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ
	};

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Check that feature Param Reg. is unmasked */
	feature_bit_param_req = test_get_feature_conn_param_req(&conn);
	zassert_equal(feature_bit_param_req, false, "Feature bit not unmasked");

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Controller do not
 * support Connection Parameters Request procedure, features exchanged.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
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
ZTEST(central_loc, test_conn_update_central_loc_unsupp_w_feat_exch)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Disable feature */
	test_unmask_feature_conn_param_req(&conn);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * (A)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * and
 *
 * (B)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Procedure collides and is rejected.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_loc, test_conn_update_central_loc_collision)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Emulate valid feature exchange */
	conn.llcp.fex.valid = 1;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* (A) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, req_B);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/**/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, &conn_param_rsp);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_rem, test_conn_update_central_rem_accept)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection with invalid parameters,
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(central_rem, test_conn_update_central_rem_invalid_req)
{
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_INVALID_LL_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_invalid);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_rem, test_conn_update_central_rem_reject)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_neg_reply(&conn, BT_HCI_ERR_UNACCEPT_CONN_PARAM);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * (A)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 *
 * and
 *
 * (B)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * NOTE:
 * Central-initiated Connection Parameters Request procedure is paused.
 * Peripheral-initiated Connection Parameters Request procedure is finished.
 * Central-initiated Connection Parameters Request procedure is resumed.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
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
ZTEST(central_rem, test_conn_update_central_rem_collision)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (B) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, req_B->interval_min, req_B->interval_max, req_B->latency,
				 req_B->timeout, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (A) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	/* (A) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	req_B->reference_conn_event_count = event_counter(&conn) - 1;
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, req_B);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_RSP, &conn, rsp_B);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	conn_update_ind_B.instant = event_counter(&conn) + 6U;
	lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind_B);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_loc, test_conn_update_periph_loc_accept)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_loc, test_conn_update_periph_loc_reject)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_UNACCEPT_CONN_PARAM };

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure. (A)
 * Peripheral requests change in LE connection parameters, central rejects due to
 * Central-initiated Connection Update procedure (B) overlapping.
 * Central rejects peripheral init and assumes 'own' connection update to complete
 *
 * +-----+                    +-------+                          +-----+
 * | UT  |                    | LL_P  |                          | LT  |
 * +-----+                    +-------+                          +-----+
 *    |                           |                                 |
 *    | LE Connection Update (A)  |                                 |
 *    |-------------------------->|                                 |
 *    |                           | LL_CONNECTION_PARAM_REQ         | (A)
 *    |                           |-------------------------------->|
 *    |                           |                                 |
 *    |                           |<--------------------------------|
 *    |                           |        LL_CONNECTION_UPDATE_IND | (B)
 *    |                           |                                 |
 *    |                           |              LL_REJECT_EXT_IND  | (A)
 *    |                           |<--------------------------------|
 *    |                           |                                 |
 *    |                           |                                 |
 *    |      LE Connection Update |                                 |
 *    |                  Complete |                                 | (A/B)
 *    |<--------------------------|                                 |
 *    |                           |                                 |
 */
ZTEST(periph_loc, test_conn_update_periph_loc_reject_central_overlap)
{
	uint8_t err;
	uint16_t instant;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_pu cu2 = { .status = BT_HCI_ERR_SUCCESS };
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	cu_ind_B->instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, cu_ind_B);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Controller do not
 * support Connection Parameters Request procedure, features not exchanged.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_loc, test_conn_update_periph_loc_unsupp_feat_wo_feat_exch)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE };

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Controller do not
 * support Connection Parameters Request procedure, features exchanged.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
ZTEST(periph_loc, test_conn_update_periph_loc_unsupp_feat_w_feat_exch)
{
	uint8_t err;

	/* Disable feature */
	test_unmask_feature_conn_param_req(&conn);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * (A)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Procedure collides and is rejected.
 *
 * and
 *
 * (B)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
 *    |                           |<--------------------------| (A)
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
ZTEST(periph_loc, test_conn_update_periph_loc_collision)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu1 = { .status = BT_HCI_ERR_LL_PROC_COLLISION };

	struct node_rx_pu cu2 = { .status = BT_HCI_ERR_SUCCESS };

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* (A) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	conn_param_req.reference_conn_event_count = event_counter(&conn);
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, req_B);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);


	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/*******************/

	/* (B) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, req_B);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	/* (B) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	rsp_B->reference_conn_event_count = req_B->reference_conn_event_count;
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, rsp_B);
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu1);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx */
	cu_ind_B->instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, cu_ind_B);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_rem, test_conn_update_periph_rem_accept)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}
#define RADIO_CONN_EVENTS(x, y) ((uint16_t)DIV_ROUND_UP(x, y))

/*
 * Central-initiated Connection Parameters Request procedure - only anchor point move.
 * Central requests change in anchor point only on LE connection, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |    (only apm)             |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |    Defered APM disabled   |                           |
 *    |    '<---------'           |                           |
 *    |    So accepted right away |                           |
 *    |    '--------->'           |                           |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_RSP   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_apm_accept_right_away)
{
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
	struct node_tx *tx;
	uint16_t instant;
	uint8_t error = 0U;
	/* Default conn_param_req PDU */
	struct pdu_data_llctrl_conn_param_req conn_param_req_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x0008U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };

	/* Default conn_param_rsp PDU */
	struct pdu_data_llctrl_conn_param_rsp conn_param_rsp_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x008U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };

	/* Prepare mocked call to ull_handle_cpr_anchor_point_move */
	/* No APM deferance, accept with error == 0 */
	ztest_returns_value(ull_handle_cpr_anchor_point_move, false);
	ztest_return_data(ull_handle_cpr_anchor_point_move, status, &error);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	conn.lll.interval = conn_param_req_apm.interval_max;
	conn.lll.latency = conn_param_req_apm.latency;
	conn.supervision_timeout = TIMEOUT;

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_apm);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp_apm);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
#endif
}

/*
 * Central-initiated Connection Parameters Request procedure - only anchor point move.
 * Central requests change in anchor point only on LE connection, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |    (only apm)             |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |    Defered APM disabled   |                           |
 *    |    '<---------'           |                           |
 *    |    So accepted right away |                           |
 *    |    but with error         |                           |
 *    |    '--------->'           |                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_apm_reject_right_away)
{
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
	struct node_tx *tx;
	/* Default conn_param_req PDU */
	struct pdu_data_llctrl_conn_param_req conn_param_req_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x0008U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL + 1
	};
	uint8_t error = reject_ext_ind.error_code;

	/* Prepare mocked call to ull_handle_cpr_anchor_point_move */
	/* No APM deferance, reject with some error code */
	ztest_returns_value(ull_handle_cpr_anchor_point_move, false);
	ztest_return_data(ull_handle_cpr_anchor_point_move, status, &error);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	conn.lll.interval = conn_param_req_apm.interval_max;
	conn.lll.latency = conn_param_req_apm.latency;
	conn.supervision_timeout = TIMEOUT;

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_apm);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
#endif
}

/*
 * Central-initiated Connection Parameters Request procedure - only anchor point move.
 * Central requests change in anchor point only on LE connection, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |    (only apm)             |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |    Defered APM            |                           |
 *    |    '<---------'           |                           |
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |    Defered accept         |                           |
 *    |    '--------->'           |                           |
 *    |                           |                           |
 *    |                           | LL_CONNECTION_PARAM_RSP   |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_apm_accept_defered)
{
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
	uint16_t offsets[6] = {
		0x0008U,
		0xffffU,
		0xffffU,
		0xffffU,
		0xffffU,
		0xffffU
	};
	struct node_tx *tx;
	uint16_t instant;
	uint8_t error = 0U;
	/* Default conn_param_req PDU */
	struct pdu_data_llctrl_conn_param_req conn_param_req_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x0004U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };

	/* Default conn_param_rsp PDU */
	struct pdu_data_llctrl_conn_param_rsp conn_param_rsp_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x008U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };

	/* Prepare mocked call to ull_handle_cpr_anchor_point_move */
	/* Defer APM */
	ztest_returns_value(ull_handle_cpr_anchor_point_move, true);
	ztest_return_data(ull_handle_cpr_anchor_point_move, status, &error);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	conn.lll.interval = conn_param_req_apm.interval_max;
	conn.lll.latency = conn_param_req_apm.latency;
	conn.supervision_timeout = TIMEOUT;

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_apm);

	/* Done */
	event_done(&conn);

	/* Run a few events */
	for (int i = 0; i < 10; i++) {

		/* Prepare */
		event_prepare(&conn);

		zassert_equal(true, ull_cp_remote_cpr_apm_awaiting_reply(&conn), NULL);

		/* There should be no host notification */
		ut_rx_q_is_empty();

		/* Done */
		event_done(&conn);
	}

	ull_cp_remote_cpr_apm_reply(&conn, offsets);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp_apm);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
#endif
}

/*
 * Central-initiated Connection Parameters Request procedure - only anchor point move.
 * Central requests change in anchor point only on LE connection, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |    (only apm)             |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |    Defered APM            |                           |
 *    |    '<---------'           |                           |
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |    Defered accept         |                           |
 *    |    but with error         |                           |
 *    |    '--------->'           |                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_apm_reject_defered)
{
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
	struct node_tx *tx;
	uint8_t error = 0U;
	/* Default conn_param_req PDU */
	struct pdu_data_llctrl_conn_param_req conn_param_req_apm = { .interval_min = INTVL_MIN,
								 .interval_max = INTVL_MAX,
								 .latency = LATENCY,
								 .timeout = TIMEOUT,
								 .preferred_periodicity = 0U,
								 .reference_conn_event_count = 0u,
								 .offset0 = 0x0008U,
								 .offset1 = 0xffffU,
								 .offset2 = 0xffffU,
								 .offset3 = 0xffffU,
								 .offset4 = 0xffffU,
								 .offset5 = 0xffffU };
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL
	};

	/* Prepare mocked call to ull_handle_cpr_anchor_point_move */
	/* Defer APM */
	ztest_returns_value(ull_handle_cpr_anchor_point_move, true);
	ztest_return_data(ull_handle_cpr_anchor_point_move, status, &error);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	conn.lll.interval = conn_param_req_apm.interval_max;
	conn.lll.latency = conn_param_req_apm.latency;
	conn.supervision_timeout = TIMEOUT;

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_apm);

	/* Done */
	event_done(&conn);

	/* Run a few events */
	for (int i = 0; i < 10; i++) {

		/* Prepare */
		event_prepare(&conn);

		zassert_equal(true, ull_cp_remote_cpr_apm_awaiting_reply(&conn), NULL);

		/* There should be no host notification */
		ut_rx_q_is_empty();

		/* Done */
		event_done(&conn);
	}

	ull_cp_remote_cpr_apm_neg_reply(&conn, BT_HCI_ERR_UNSUPP_LL_PARAM_VAL);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
}

/*
 * (A)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Procedure collides and is rejected.
 *
 * and
 *
 * (B)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~    parallel remote CPRs attempted and rejected   ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
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
 *    |                           |<--------------------------| (A)
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (A)
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~    parallel local CPR is attempted and cached    ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           | (B)
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~    parallel local CPR is now started    ~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 */
ZTEST(periph_loc, test_conn_update_periph_loc_collision_reject_2nd_cpr)
{
	struct ll_conn conn_2nd;
	struct ll_conn conn_3rd;
	uint8_t err;
	struct node_tx *tx, *tx1;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu1 = { .status = BT_HCI_ERR_LL_PROC_COLLISION };
	struct node_rx_pu cu2 = { .status = BT_HCI_ERR_SUCCESS };
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind parallel_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL
	};

	/* Initialize extra connections */
	test_setup_idx(&conn_2nd, 1);
	test_setup_idx(&conn_3rd, 2);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	/* Role */
	test_set_role(&conn_2nd, BT_HCI_ROLE_PERIPHERAL);
	/* Role */
	test_set_role(&conn_3rd, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Connect */
	ull_cp_state_set(&conn_2nd, ULL_CP_CONNECTED);
	/* Connect */
	ull_cp_state_set(&conn_3rd, ULL_CP_CONNECTED);

	/* (A) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx1, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, req_B);

	/* Done */
	event_done(&conn);

	{
		zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-2,
		       "Free CTX buffers %d", llcp_ctx_buffers_free());
		/* Parallel CPR from central */
		/* Now CPR is active on 'conn' so let 'conn_2nd' attempt to start a CPR */
		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Rx */
		lt_tx(LL_CONNECTION_PARAM_REQ, &conn_2nd, &conn_param_req);

		/* Done */
		event_done(&conn_2nd);

		/* Tx Queue should have one LL Control PDU */
		lt_rx(LL_REJECT_EXT_IND, &conn_2nd, &tx, &parallel_reject_ext_ind);
		lt_rx_q_is_empty(&conn_2nd);

		/* Release Tx */
		ull_cp_release_tx(&conn_2nd, tx);

		/* There should be no 'extra' procedure on acount of the parallel CPR */
		zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-2,
		       "Free CTX buffers %d", llcp_ctx_buffers_free());
	}

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	{
		/* Parallel CPR from peripheral */
		/* Now CPR is active on 'conn' so let 'conn_3rd' attempt to start a CPR */
		/* Prepare */
		event_prepare(&conn_3rd);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn_3rd);

		/* Rx */
		lt_tx(LL_CONNECTION_PARAM_REQ, &conn_3rd, &conn_param_req);

		/* Done */
		event_done(&conn_3rd);

		/* Tx Queue should have one LL Control PDU */
		lt_rx(LL_REJECT_EXT_IND, &conn_3rd, &tx, &parallel_reject_ext_ind);
		lt_rx_q_is_empty(&conn_3rd);

		/* Release Tx */
		ull_cp_release_tx(&conn_3rd, tx);

		/* There should be no 'extra' procedure on acount of the parallel CPR */
		zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-2,
		       "Free CTX buffers %d", llcp_ctx_buffers_free());
	}

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);


	/* Release Tx */
	ull_cp_release_tx(&conn, tx1);

	/*******************/

	/* (B) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, req_B);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	/* (B) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	rsp_B->reference_conn_event_count = req_B->reference_conn_event_count;
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, rsp_B);
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu1);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	{
		/* Initiate a parallel local Connection Parameter Request Procedure */
		err = ull_cp_conn_update(&conn_2nd, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
		zassert_equal(err, BT_HCI_ERR_SUCCESS);

		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have no LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);

		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have no LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);
	}

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx */
	cu_ind_B->instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, cu_ind_B);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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

	{
		/* Now the locally initiated CPR on conn_3rd should be allowed to run */
		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have one LL Control PDU, indicating parallel CPR is now active */
		conn_param_req.reference_conn_event_count = event_counter(&conn_2nd);
		lt_rx(LL_CONNECTION_PARAM_REQ, &conn_2nd, &tx, &conn_param_req);
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);

		/* Release Tx */
		ull_cp_release_tx(&conn_2nd, tx);
	}

	/* Release Ntf */
	release_ntf(ntf);

	/* One less CTXs as the conn_2nd CPR is still 'running' */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-1,
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~  parallel remote CPR is attempted and rejected   ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           |
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~    parallel local CPR is attempted and cached    ~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~    parallel local CPR is now started    ~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_accept_reject_2nd_cpr)
{
	uint8_t err;
	struct ll_conn conn_2nd;
	struct ll_conn conn_3rd;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;
	struct pdu_data_llctrl_reject_ext_ind parallel_reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL
	};

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Initialize extra connections */
	test_setup_idx(&conn_2nd, 1);
	test_setup_idx(&conn_3rd, 2);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	/* Role */
	test_set_role(&conn_2nd, BT_HCI_ROLE_PERIPHERAL);
	/* Role */
	test_set_role(&conn_3rd, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Connect */
	ull_cp_state_set(&conn_2nd, ULL_CP_CONNECTED);
	/* Connect */
	ull_cp_state_set(&conn_3rd, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	{
		/* Parallel CPR from central */
		/* Now CPR is active on 'conn' so let 'conn_2nd' attempt to start a CPR */
		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Rx */
		lt_tx(LL_CONNECTION_PARAM_REQ, &conn_2nd, &conn_param_req);

		/* Done */
		event_done(&conn_2nd);

		/* Tx Queue should have one LL Control PDU */
		lt_rx(LL_REJECT_EXT_IND, &conn_2nd, &tx, &parallel_reject_ext_ind);
		lt_rx_q_is_empty(&conn_2nd);

		/* Release Tx */
		ull_cp_release_tx(&conn_2nd, tx);

		/* There should be no 'extra' procedure on acount of the parallel CPR */
		zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-1,
		       "Free CTX buffers %d", llcp_ctx_buffers_free());
	}

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	{
		/* Parallel CPR from peripheral */
		/* Now CPR is active on 'conn' so let 'conn_3rd' attempt to start a CPR */
		/* Prepare */
		event_prepare(&conn_3rd);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn_3rd);

		/* Rx */
		lt_tx(LL_CONNECTION_PARAM_REQ, &conn_3rd, &conn_param_req);

		/* Done */
		event_done(&conn_3rd);

		/* Tx Queue should have one LL Control PDU */
		lt_rx(LL_REJECT_EXT_IND, &conn_3rd, &tx, &parallel_reject_ext_ind);
		lt_rx_q_is_empty(&conn_3rd);

		/* Release Tx */
		ull_cp_release_tx(&conn_3rd, tx);

		/* There should be no 'extra' procedure on acount of the parallel CPR */
		zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-1,
		       "Free CTX buffers %d", llcp_ctx_buffers_free());
	}

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	{
		/* Initiate a parallel local Connection Parameter Request Procedure */
		err = ull_cp_conn_update(&conn_2nd, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
		zassert_equal(err, BT_HCI_ERR_SUCCESS);

		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have no LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);

		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have no LL Control PDU */
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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

	{
		/* Now the locally initiated CPR on conn_3rd should be allowed to run */
		/* Prepare */
		event_prepare(&conn_2nd);

		/* Tx Queue should have one LL Control PDU, indicating parallel CPR is now active */
		conn_param_req.reference_conn_event_count = event_counter(&conn_2nd);
		lt_rx(LL_CONNECTION_PARAM_REQ, &conn_2nd, &tx, &conn_param_req);
		lt_rx_q_is_empty(&conn_2nd);

		/* Done */
		event_done(&conn_2nd);

		/* Release Tx */
		ull_cp_release_tx(&conn_2nd, tx);
	}

	/* Release Ntf */
	release_ntf(ntf);

	/* One less CTXs as the conn_2nd CPR is still 'running' */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt()-1,
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection with invalid parameters,
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_invalid_req)
{
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_INVALID_LL_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req_invalid);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
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
 *    |                           |       LL_<INVALID_IND>    |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem, test_conn_update_periph_rem_invalid_ind)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_reject_ind reject_ind = {
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_rem, test_conn_update_periph_rem_reject)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_UNACCEPT_CONN_PARAM
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	ull_cp_conn_param_req_neg_reply(&conn, BT_HCI_ERR_UNACCEPT_CONN_PARAM);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * (A)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * and
 *
 * (B)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 *
 * NOTE:
 * Peripheral-initiated Connection Parameters Request procedure is paused.
 * Central-initiated Connection Parameters Request procedure is finished.
 * Peripheral-initiated Connection Parameters Request procedure is resumed.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
ZTEST(periph_rem, test_conn_update_periph_rem_collision)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* (B) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, req_B->interval_min, req_B->interval_max, req_B->latency,
				 req_B->timeout, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/*******************/
	/* (A) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/

	/* (A) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);
	conn_param_rsp.reference_conn_event_count = conn_param_req.reference_conn_event_count;

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	req_B->reference_conn_event_count = event_counter(&conn) - 1;
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, req_B);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	cu_ind_B->instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, cu_ind_B);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * (A)
 * Central-initiated Connection Parameters Request procedure.
 * Central requests change in LE connection parameters, peripheral’s Host accepts.
 *
 * and
 *
 * (B)
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 *
 * NOTE:
 * Peripheral-initiated Connection Parameters Request procedure is paused.
 * Central-initiated Connection Parameters Request procedure is finished.
 * Peripheral-initiated Connection Parameters Request procedure is resumed.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           | (B)
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |<--------------------------| (A)
 *    |                           |   LL_CONNECTION_PARAM_REQ |
 *    |                           |-------------------------->| (B)
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 *    |      LE Remote Connection |                           |
 *    |         Parameter Request |                           |
 *    |<--------------------------|                           | (A)
 *    | LE Remote Connection      |                           |
 *    | Parameter Request         |                           |
 *    | Reply                     |                           |
 *    |-------------------------->|                           | (A)
 *    |                           |   LL_REJECT_EXT_IND       |
 *    |                           |<--------------------------| (B)
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |      Complete (collision) |                           |
 *    |<--------------------------|                           | (B)
 *    |                           | LL_CONNECTION_PARAM_RSP   |
 *    |                           |-------------------------->| (A)
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           | (B)
 *    |                           |                           |
 *    |               <------------------------>              |
 *    |               < LOCAL PROCEDURE PAUSED >              |
 *    |               <------------------------>              |
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
ZTEST(periph_rem, test_conn_update_periph_rem_late_collision)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	uint16_t instant;
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};
	struct node_rx_pu cu1 = { .status = BT_HCI_ERR_LL_PROC_COLLISION };
	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*******************/

	/* (B) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, req_B->interval_min, req_B->interval_max, req_B->latency,
				 req_B->timeout, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/*******************/

	/* (A) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should have one LL Control PDU */
	req_B->reference_conn_event_count = event_counter(&conn) - 1;
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, req_B);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/*******************/
	/* (A) There should be one host notification */
	ut_rx_pdu(LL_CONNECTION_PARAM_REQ, &ntf, &conn_param_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/*******************/
	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* (A) */
	ull_cp_conn_param_req_reply(&conn);

	/*******************/

	/* Prepare */
	event_prepare(&conn);
	conn_param_rsp.reference_conn_event_count = conn_param_req.reference_conn_event_count;

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_RSP, &conn, &tx, &conn_param_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu1);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* (B) Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, req_B->interval_min, req_B->interval_max, req_B->latency,
				 req_B->timeout, NULL);

	/* Prepare */
	event_prepare(&conn);
	/* Done */
	event_done(&conn);


	/* (A) Rx */
	conn_update_ind.instant = event_counter(&conn) + 6U;
	instant = conn_update_ind.instant;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);
	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

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
	req_B->reference_conn_event_count = event_counter(&conn) - 1;
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, req_B);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	cu_ind_B->instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, cu_ind_B);

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
	release_ntf(ntf);
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}
#else /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

/*
 * Parameter Request Procedure not supported.
 * Central-initiated Connection Update procedure.
 * Central requests update of LE connection.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           | LL_CONNECTION_UPDATE_IND  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE Connection Update |                           |
 *    |                  Complete |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 *    | (If conn. parameters are  |                           |
 *    |  unchanged, host should   |                           |
 *    |  not receive a ntf.)      |                           |
 *    |                           |                           |
 */
ZTEST(central_loc_no_param_req, test_conn_update_central_loc_accept_no_param_req)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	/* Test with and without parameter change  */
	uint8_t parameters_changed = 1U;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	do {
		/* Initiate a Connection Update Procedure */
		err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
		zassert_equal(err, BT_HCI_ERR_SUCCESS);

		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should have one LL Control PDU */
		conn_update_ind.instant = event_counter(&conn) + 6U;
		lt_rx(LL_CONNECTION_UPDATE_IND, &conn, &tx, &conn_update_ind);
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* Release Tx */
		ull_cp_release_tx(&conn, tx);

		/* Save Instant */
		pdu = (struct pdu_data *)tx->pdu;
		instant = sys_le16_to_cpu(pdu->llctrl.conn_update_ind.instant);

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

		if (parameters_changed == 0U) {
			/* There should NOT be a host notification */
			ut_rx_q_is_empty();
		} else {
			/* There should be one host notification */
			ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
			ut_rx_q_is_empty();

			/* Release Ntf */
			release_ntf(ntf);
		}
	} while (parameters_changed-- > 0U);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Parameter Request Procedure not supported.
 * Peripheral-initiated Connection Update/Connection Parameter Request procedure
 * Central receives Connection Update parameters.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |           LL_UNKNOWN_RSP  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_PARAM_REQ  |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |           LL_UNKNOWN_RSP  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(central_rem_no_param_req, test_conn_update_central_rem_unknown_no_param_req)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_UNKNOWN_RSP, &conn, &tx, &unknown_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should NOT be a host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

	/* Check UNKNOWN_RSP on Connection Parameter Request */
	unknown_rsp.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_UNKNOWN_RSP, &conn, &tx, &unknown_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should NOT be a host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/*
 * Parameter Request Procedure not supported.
 * Peripheral-initiated Connection Update/Connection Parameter Request procedure
 * Central receives Connection Update parameters.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_M  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_PARAM_REQ  |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |           LL_UNKNOWN_RSP  |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem_no_param_req, test_conn_update_periph_rem_unknown_no_param_req)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, &conn_param_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_UNKNOWN_RSP, &conn, &tx, &unknown_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should NOT be a host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());

}

/*
 * Parameter Request Procedure not supported.
 * Central-initiated Connection Update procedure.
 * Peripheral receives Connection Update parameters.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
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
 *    | (If conn. parameters are  |                           |
 *    |  unchanged, host should   |                           |
 *    |  not receive a ntf.)      |                           |
 *    |                           |                           |
 */
ZTEST(periph_rem_no_param_req, test_conn_update_periph_rem_accept_no_param_req)
{
	struct node_rx_pdu *ntf;
	uint16_t instant;

	/* Test with and without parameter change  */
	uint8_t parameters_changed = 1U;

	struct node_rx_pu cu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	do {
		/* Prepare */
		event_prepare(&conn);

		/* Rx */
		conn_update_ind.instant = event_counter(&conn) + 6U;
		instant = conn_update_ind.instant;
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

		if (parameters_changed == 0U) {
			/* There should NOT be a host notification */
			ut_rx_q_is_empty();
		} else {
			/* There should be one host notification */
			ut_rx_node(NODE_CONN_UPDATE, &ntf, &cu);
			ut_rx_q_is_empty();

			/* Release Ntf */
			release_ntf(ntf);
		}
	} while (parameters_changed-- > 0U);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Parameter Request Procedure not supported.
 * Peripheral-initiated Connection Update procedure (not allowed).
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE Connection Update      |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |      ERR CMD Disallowed   |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 */
ZTEST(periph_loc_no_param_req, test_conn_update_periph_loc_disallowed_no_param_req)
{
	uint8_t err;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Update Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_CMD_DISALLOWED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}
#endif

/*
 * Central-initiated Connection Update procedure.
 * Peripheral receives invalid Connection Update parameters.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |  LL_CONNECTION_UPDATE_IND |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 */
ZTEST(periph_rem_invalid, test_conn_update_periph_rem_invalid_param)
{
	uint16_t interval;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	interval = conn_update_ind.interval;
	conn_update_ind.interval = 0U;
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_INVALID_LL_PARAM,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* Restore interval for other tests */
	conn_update_ind.interval = interval;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
/*
 * Peripheral-initiated Connection Parameters Request procedure.
 * Peripheral requests change in LE connection parameters, central’s Host accepts.
 * Peripheral receives invalid Connection Update parameters.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_P  |                    | LT  |
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
 *    ~~~~~~~~~~~~~~~~~~ TERMINATE CONNECTION ~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 */
ZTEST(periph_rem_invalid, test_conn_param_req_periph_rem_invalid_param)
{
	struct node_tx *tx;
	uint16_t interval;
	uint8_t err;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate a Connection Parameter Request Procedure */
	err = ull_cp_conn_update(&conn, INTVL_MIN, INTVL_MAX, LATENCY, TIMEOUT, NULL);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);
	conn_param_req.reference_conn_event_count = event_counter(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CONNECTION_PARAM_REQ, &conn, &tx, &conn_param_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	interval = conn_update_ind.interval;
	conn_update_ind.interval = 0U;
	conn_update_ind.instant = event_counter(&conn) + 6U;
	lt_tx(LL_CONNECTION_UPDATE_IND, &conn, &conn_update_ind);

	/* Done */
	event_done(&conn);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_INVALID_LL_PARAM,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Clear termination flag for subsequent test cycle */
	conn.llcp_terminate.reason_final = 0;

	/* Restore interval for other tests */
	conn_update_ind.interval = interval;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
ZTEST_SUITE(central_loc, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(central_rem, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(periph_loc, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(periph_rem, NULL, NULL, conn_update_setup, NULL, NULL);
#else
ZTEST_SUITE(central_loc_no_param_req, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(central_rem_no_param_req, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(periph_loc_no_param_req, NULL, NULL, conn_update_setup, NULL, NULL);
ZTEST_SUITE(periph_rem_no_param_req, NULL, NULL, conn_update_setup, NULL, NULL);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

ZTEST_SUITE(periph_rem_invalid, NULL, NULL, conn_update_setup, NULL, NULL);
