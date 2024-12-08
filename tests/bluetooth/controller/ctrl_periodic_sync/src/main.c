/*
 * Copyright (c) 2024 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
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
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_sync_internal.h"
#include "ull_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;
static struct ll_sync_set *sync;
static struct ll_adv_sync_set *adv_sync;

static void periodic_sync_setup(void *data)
{
	test_setup(&conn);
}

/* Custom fakes for this test suite */

/* uint32_t mayfly_enqueue(uint8_t caller_id, uint8_t callee_id,
 *							uint8_t chain, struct mayfly *m);
 */
FAKE_VALUE_FUNC(uint32_t, mayfly_enqueue, uint8_t, uint8_t, uint8_t, struct mayfly *);

uint32_t mayfly_enqueue_custom_fake(uint8_t caller_id, uint8_t callee_id,
								uint8_t chain, struct mayfly *m)
{
	/* Only proceed if it is the right mayfly enqueue used for getting the offset */
	if (m->param == &conn && chain == 1U) {
		/* Mock that mayfly has run and ull_lp_past_offset_calc_reply()
		 * is called with the found past offset
		 */
		ull_lp_past_offset_calc_reply(&conn, 0, 0, 0);
	}

	return 0;
}

/* void ull_sync_transfer_received(struct ll_conn *conn, uint16_t service_data,
 *				struct pdu_adv_sync_info *si, uint16_t conn_event_count,
 *				uint16_t last_pa_event_counter, uint8_t sid,
 *				uint8_t addr_type, uint8_t sca, uint8_t phy,
 *				uint8_t *adv_addr, uint16_t sync_conn_event_count,
 *				uint8_t addr_resolved);
 */
FAKE_VOID_FUNC(ull_sync_transfer_received, struct ll_conn *, uint16_t,
					struct pdu_adv_sync_info *,
					uint16_t,
					uint16_t,
					uint8_t,
					uint8_t,
					uint8_t,
					uint8_t,
					uint8_t *,
					uint16_t,
					uint8_t);

void ull_sync_transfer_received_custom_fake(struct ll_conn *conn, uint16_t service_data,
				struct pdu_adv_sync_info *si, uint16_t conn_event_count,
				uint16_t last_pa_event_counter, uint8_t sid,
				uint8_t addr_type, uint8_t sca, uint8_t phy,
				uint8_t *adv_addr, uint16_t sync_conn_event_count,
				uint8_t addr_resolved)
{
}
/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Start                      |                     |
 *    | Periodic Adv. Sync Transfer|                     |
 *    | Proc.                      |                     |
 *    |--------------------------->|                     |
 *    |                            |                     |
 *    |                            | LL_PERIODIC_SYNC_IND|
 *    |                            |------------------>  |
 *    |                            |             'll_ack'|
 *    |                            |                     |
 *    |Periodic Adv. Sync Transfer |                     |
 *    |	Proc. Complete             |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct proc_ctx *ctx;
	uint16_t service_data = 0;

	adv_sync = NULL;
	sync = ull_sync_is_enabled_get(0);

	struct pdu_data_llctrl_periodic_sync_ind local_periodic_sync_ind = {
		.id = 0x00,
		.conn_event_count = 0x00,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x00,
		.sca = 0x00,
		.phy = 0x00,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0
	};

	/* Reset and setup mayfly_enqueue_custom_fake */
	RESET_FAKE(mayfly_enqueue);
	mayfly_enqueue_fake .custom_fake = mayfly_enqueue_custom_fake;

	/* Initialise sync_info */
	memset(&local_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	/* Initiate a Periodic Adv. Sync Transfer Procedure */
	err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Connection event done with successful rx from peer */
	ctx = llcp_lr_peek_proc(&conn, PROC_PERIODIC_SYNC);
	if (ctx) {
		ctx->data.periodic_sync.conn_start_to_actual_us = 0;
		ctx->data.periodic_sync.conn_evt_trx = 1;
		llcp_lp_past_conn_evt_done(&conn, ctx);
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PERIODIC_SYNC_IND, &conn, &tx, &local_periodic_sync_ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d and ctx_buffers_cnt is %d", llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt());
}

ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_loc_2)
{
	uint8_t err;
	uint16_t service_data = 0;

	sync = ull_sync_is_enabled_get(0);
	adv_sync = NULL;

	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ull_llcp_init(&conn);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);

	for (int i = 0U; i < CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM; i++) {
		zassert_equal(err, BT_HCI_ERR_SUCCESS);
		err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);
	}

	zassert_not_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	zassert_equal(llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt() - CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM,
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+ +-------+                  +-----+
 * | UT  | | LL_A  |                  | LT  |
 * +-----+ +-------+                  +-----+
 *    |        |                         |
 *    |        |    LL_PERIODIC_SYNC_IND |
 *    |        |<----------------------- |
 *    |        |                         |
 *    |        |                         |
 *    |        |                         |
 *    |        |                         |
 */
ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_rem)
{
	struct pdu_data_llctrl_periodic_sync_ind remote_periodic_sync_ind = {
		.id = 0x01,
		.conn_event_count = 0x00,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x01,
		.sca = 0x00,
		.phy = 0x01,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0
	};

	/* Reset and setup fake functions */
	RESET_FAKE(ull_sync_transfer_received);
	ull_sync_transfer_received_fake.custom_fake = ull_sync_transfer_received_custom_fake;

	RESET_FAKE(mayfly_enqueue);
	mayfly_enqueue_fake .custom_fake = mayfly_enqueue_custom_fake;

	/* Initialise sync_info */
	memset(&remote_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PERIODIC_SYNC_IND, &conn, &remote_periodic_sync_ind);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d and ctx_buffers_cnt is %d", llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt());


	/* Verify that ull_sync_transfer_received was called,
	 * hence the phy invalidation mechanism works
	 */
	zassert_equal(ull_sync_transfer_received_fake.call_count, 1,
			"ull_sync_transfer_received_fake.call_count is %d and expected: %d",
			ull_sync_transfer_received_fake.call_count, 1);
}

/* +-----+                     +-------+                  +-----+
 * | UT  |                     | LL_A  |                  | LT  |
 * +-----+                     +-------+                  +-----+
 *    |                            |                         |
 *    |                            |    LL_PERIODIC_SYNC_IND |
 *    |                            |<------------------------|
 *    |                            |                         |
 *    |                            |                         |
 *    |                            |                         |
 *    |                            |                         |
 *    | Start                      |                         |
 *    | Periodic Adv. Sync Transfer|                         |
 *    | Proc.                      |                         |
 *    |--------------------------->|                         |
 *    |                            |                         |
 *    |                            |                         |
 *    |                            |   LL_PERIODIC_SYNC_IND  |
 *    |                            |------------------------>|
 *    |                            |                 'll_ack'|
 *    |Periodic Adv. Sync Transfer |                         |
 *    |             Proc. Complete |                         |
 *    |<---------------------------|                         |
 *    |                            |                         |
 */
ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_rem_2)
{
	uint8_t err;
	struct node_tx *tx;
	uint16_t service_data = 0;
	struct proc_ctx *ctx;

	sync = ull_sync_is_enabled_get(0);
	adv_sync = NULL;

	struct pdu_data_llctrl_periodic_sync_ind local_periodic_sync_ind = {
		.id = 0x00,
		.conn_event_count = 0x01,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x00,
		.sca = 0x00,
		.phy = 0x00,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0x01
	};

	struct pdu_data_llctrl_periodic_sync_ind remote_periodic_sync_ind = {
		.id = 0x01,
		.conn_event_count = 0x00,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x01,
		.sca = 0x00,
		.phy = 0x01,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0
	};

	/* Reset and setup fake functions */
	RESET_FAKE(ull_sync_transfer_received);
	ull_sync_transfer_received_fake.custom_fake = ull_sync_transfer_received_custom_fake;

	RESET_FAKE(mayfly_enqueue);
	mayfly_enqueue_fake .custom_fake = mayfly_enqueue_custom_fake;

	/* Initialise sync_info */
	memset(&local_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));
	memset(&remote_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PERIODIC_SYNC_IND, &conn, &remote_periodic_sync_ind);

	/* Done */
	event_done(&conn);

	/* Initiate a Periodic Adv. Sync Transfer Procedure */
	err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Connection event done with successful rx from peer */
	ctx = llcp_lr_peek_proc(&conn, PROC_PERIODIC_SYNC);
	if (ctx) {
		ctx->data.periodic_sync.conn_start_to_actual_us = 0;
		ctx->data.periodic_sync.conn_evt_trx = 1;
		llcp_lp_past_conn_evt_done(&conn, ctx);
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PERIODIC_SYNC_IND, &conn, &tx, &local_periodic_sync_ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d and ctx_buffers_cnt is %d", llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt());

	/* Verify that ull_sync_transfer_received was called,
	 * hence the phy invalidation mechanism works
	 */
	zassert_equal(ull_sync_transfer_received_fake.call_count, 1,
		      "ull_sync_transfer_received_fake.call_count is %d and expected: %d",
		      ull_sync_transfer_received_fake.call_count, 1);
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Start                      |                     |
 *    | Periodic Adv. Sync Transfer|                     |
 *    | Proc.                      |                     |
 *    |--------------------------->|                     |
 *    |                            |                     |
 *    |                            | LL_PERIODIC_SYNC_IND|
 *    |                            |------------------>  |
 *    |                            |             'll_ack'|
 *    |                            |                     |
 *    |                            |                     |
 *    |                            |                     |
 *    |Periodic Adv. Sync Transfer |                     |
 *    |Proc. Complete              |                     |
 *    |<---------------------------|                     |
 *    | Start                      |                     |
 *    | Periodic Adv. Sync Transfer|                     |
 *    | Proc.                      |                     |
 *    |--------------------------->|                     |
 *    |                            |                     |
 *    |                            |                     |
 *    |                            | LL_PERIODIC_SYNC_IND|
 *    |                            |------------------>  |
 *    |Periodic Adv. Sync Transfer |             'll_ack'|
 *    |             Proc. Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_loc_twice)
{
	uint8_t err;
	struct node_tx *tx;
	uint16_t service_data = 0;
	struct proc_ctx *ctx;

	sync = ull_sync_is_enabled_get(0);
	adv_sync = NULL;

	struct pdu_data_llctrl_periodic_sync_ind local_periodic_sync_ind = {
		.id = 0x00,
		.conn_event_count = 0x00,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x00,
		.sca = 0x00,
		.phy = 0x00,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0
	};

	/* Reset and setup mayfly_enqueue_custom_fake */
	RESET_FAKE(mayfly_enqueue);
	mayfly_enqueue_fake .custom_fake = mayfly_enqueue_custom_fake;

	/* Initialise sync_info */
	memset(&local_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	/* Initiate a periodic_sync Procedure */
	err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Connection event done with successful rx from peer */
	ctx = llcp_lr_peek_proc(&conn, PROC_PERIODIC_SYNC);
	if (ctx) {
		ctx->data.periodic_sync.conn_start_to_actual_us = 0;
		ctx->data.periodic_sync.conn_evt_trx = 1;
		llcp_lp_past_conn_evt_done(&conn, ctx);
	}

	/* Initiate a periodic_sync Procedure */
	err = ull_cp_periodic_sync(&conn, sync, adv_sync, service_data);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PERIODIC_SYNC_IND, &conn, &tx, &local_periodic_sync_ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Increase connection event count */
	local_periodic_sync_ind.conn_event_count++;
	local_periodic_sync_ind.sync_conn_event_count++;

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	/* Connection event done with successful rx from peer */
	ctx = llcp_lr_peek_proc(&conn, PROC_PERIODIC_SYNC);
	if (ctx) {
		ctx->data.periodic_sync.conn_start_to_actual_us = 0;
		ctx->data.periodic_sync.conn_evt_trx = 1;
		llcp_lp_past_conn_evt_done(&conn, ctx);
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PERIODIC_SYNC_IND, &conn, &tx, &local_periodic_sync_ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	/* Second attempt to run the periodic_sync  completes immediately in idle state.
	 * The context is released just after that.
	 */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d and ctx_buffers_cnt is %d", llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt());
}

/*
 * +-----+ +-------+                  +-----+
 * | UT  | | LL_A  |                  | LT  |
 * +-----+ +-------+                  +-----+
 *    |        |                         |
 *    |        |    LL_PERIODIC_SYNC_IND |
 *    |        |        (Invalid PHY)    |
 *    |        |<----------------------- |
 *    |        |                         |
 *    |        |                         |
 *    |        |                         |
 *    |        |                         |
 */
ZTEST(periodic_sync_transfer, test_periodic_sync_transfer_invalid_phy)
{
	struct pdu_data_llctrl_periodic_sync_ind remote_periodic_sync_ind = {
		.id = 0x01,
		.conn_event_count = 0x00,
		.last_pa_event_counter = 0x00,
		.sid = 0x00,
		.addr_type = 0x01,
		.sca = 0x00,
		.phy = 0x03,
		.adv_addr = { 0, 0, 0, 0, 0, 0},
		.sync_conn_event_count = 0
	};

	/* Reset and setup ull_sync_transfer_received fake */
	RESET_FAKE(ull_sync_transfer_received);
	ull_sync_transfer_received_fake.custom_fake = ull_sync_transfer_received_custom_fake;

	RESET_FAKE(mayfly_enqueue);
	mayfly_enqueue_fake .custom_fake = mayfly_enqueue_custom_fake;

	/* Initialise sync_info */
	memset(&remote_periodic_sync_ind.sync_info, 0, sizeof(struct pdu_adv_sync_info));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_PAST_RECV);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PERIODIC_SYNC_IND, &conn, &remote_periodic_sync_ind);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d and ctx_buffers_cnt is %d", llcp_ctx_buffers_free(),
		      test_ctx_buffers_cnt());

	/* Verify that ull_sync_transfer_received was not called,
	 * hence the phy invalidation mechanism works
	 */
	zassert_equal(ull_sync_transfer_received_fake.call_count, 0,
		      "ull_sync_transfer_received_fake.call_count is %d and expected: %d",
		      ull_sync_transfer_received_fake.call_count, 0);
}

ZTEST_SUITE(periodic_sync_transfer, NULL, NULL, periodic_sync_setup, NULL, NULL);
