/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <ztest.h>
#include "kconfig.h"

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

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_df_types.h"
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

#include "ll_feat.h"

#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

struct ll_conn *conn_from_pool;

static void setup(void)
{
	ull_conn_init();

	conn_from_pool = ll_conn_acquire();
	zassert_not_null(conn_from_pool, "Could not allocate connection memory", NULL);

	test_setup(conn_from_pool);
}

/*
 * +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Feature Exchange Proc.     |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_FEATURE_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_FEATURE_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |     Feature Exchange Proc. |                   |
 *    |                   Complete |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
void test_hci_feature_exchange(void)
{
	uint64_t err;
	uint64_t set_feature = DEFAULT_FEATURE;
	uint64_t rsp_feature = ((LL_FEAT_BIT_MASK_VALID & FEAT_FILTER_OCTET0) | DEFAULT_FEATURE) &
			       LL_FEAT_BIT_MASK_VALID;

	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;
	struct pdu_data_llctrl_feature_rsp remote_feature_rsp;

	uint16_t conn_handle;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	sys_put_le64(set_feature, local_feature_req.features);
	sys_put_le64(rsp_feature, remote_feature_rsp.features);

	/* Initiate a Feature Exchange Procedure via HCI */
	err = ll_feature_req_send(conn_handle);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Error: %d", err);

	/* basically copied from unit-test for feature exchange */
	event_prepare(conn_from_pool);
	lt_rx(LL_FEATURE_REQ, conn_from_pool, &tx, &local_feature_req);
	lt_rx_q_is_empty(conn_from_pool);
	lt_tx(LL_FEATURE_RSP, conn_from_pool, &remote_feature_rsp);
	event_done(conn_from_pool);
	ut_rx_pdu(LL_FEATURE_RSP, &ntf, &remote_feature_rsp);
	ut_rx_q_is_empty();
	zassert_equal(conn_from_pool->lll.event_counter, 1, "Wrong event count %d\n",
		      conn_from_pool->lll.event_counter);
	ull_cp_release_tx(conn_from_pool, tx);
	ull_cp_release_ntf(ntf);

	ll_conn_release(conn_from_pool);
}

void test_hci_feature_exchange_wrong_handle(void)
{
	uint16_t conn_handle;
	uint64_t err;
	int ctx_counter;
	struct proc_ctx *ctx;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	err = ll_feature_req_send(conn_handle + 1);

	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Wrong reply for wrong handle\n");

	/* Use up all local procedure contexts */
	ctx_counter = 0;
	do {
		ctx = llcp_create_local_procedure(PROC_FEATURE_EXCHANGE);
		ctx_counter++;
	} while (ctx != NULL);

	err = ll_feature_req_send(conn_handle);
	zassert_equal(err, BT_HCI_ERR_CMD_DISALLOWED, "Wrong reply for no-resource condition\n");
}

void test_hci_version_ind(void)
{
	uint64_t err;
	uint16_t conn_handle;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_version_ind local_pdu = {
		.version_number = LL_VERSION_NUMBER,
		.company_id = CONFIG_BT_CTLR_COMPANY_ID,
		.sub_version_number = CONFIG_BT_CTLR_SUBVERSION_NUMBER,
	};

	struct pdu_data_llctrl_version_ind remote_pdu = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	err = ll_version_ind_send(conn_handle);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Error: %d", err);

	event_prepare(conn_from_pool);
	lt_rx(LL_VERSION_IND, conn_from_pool, &tx, &local_pdu);
	lt_rx_q_is_empty(conn_from_pool);
	lt_tx(LL_VERSION_IND, conn_from_pool, &remote_pdu);
	event_done(conn_from_pool);
	ut_rx_pdu(LL_VERSION_IND, &ntf, &remote_pdu);
	ut_rx_q_is_empty();
	zassert_equal(conn_from_pool->lll.event_counter, 1, "Wrong event count %d\n",
		      conn_from_pool->lll.event_counter);
	ull_cp_release_tx(conn_from_pool, tx);
	ull_cp_release_ntf(ntf);

	ll_conn_release(conn_from_pool);
}

void test_hci_version_ind_wrong_handle(void)
{
	uint16_t conn_handle;
	uint64_t err;
	int ctx_counter;
	struct proc_ctx *ctx;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	err = ll_version_ind_send(conn_handle + 1);

	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Wrong reply for wrong handle\n");

	ctx_counter = 0;
	do {
		ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);
		ctx_counter++;
	} while (ctx != NULL);

	err = ll_version_ind_send(conn_handle);
	zassert_equal(err, BT_HCI_ERR_CMD_DISALLOWED, "Wrong reply for no-resource condition\n");
}

void test_hci_apto(void)
{
	uint16_t conn_handle;
	uint64_t err;

	uint16_t apto;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	conn_from_pool->apto_reload = 100;
	conn_from_pool->lll.interval = 10;
	err = ll_apto_get(conn_handle, &apto);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	zassert_equal(apto, 125, "Apto is %d", apto);

	err = ll_apto_get(conn_handle + 1, &apto);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, NULL);

	err = ll_apto_set(conn_handle, 1000);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	zassert_equal(conn_from_pool->apto_reload, 800, "Apto reload is %d",
		      conn_from_pool->apto_reload);

	err = ll_apto_get(conn_handle + 1, 0x00);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, NULL);
}

void test_hci_phy(void)
{
	uint16_t conn_handle;
	uint64_t err;

	uint8_t phy_tx, phy_rx;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	err = ll_phy_req_send(conn_handle + 1, 0x00, 0x00, 0x00);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, NULL);
	conn_from_pool->llcp.fex.features_used = 0x00;
	conn_from_pool->llcp.fex.valid = 1;
	err = ll_phy_req_send(conn_handle, 0x03, 0xFF, 0x03);
	zassert_equal(err, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE, "Errorcode %d", err);

	conn_from_pool->llcp.fex.features_used = 0xFFFF;
	err = ll_phy_req_send(conn_handle, 0x03, 0xFF, 0x03);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
	err = ll_phy_get(conn_handle + 1, &phy_tx, &phy_rx);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, NULL);

	conn_from_pool->lll.phy_rx = 0x3;
	conn_from_pool->lll.phy_tx = 0x7;
	err = ll_phy_get(conn_handle, &phy_tx, &phy_rx);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	zassert_equal(phy_tx, 0x07, NULL);
	zassert_equal(phy_rx, 0x03, NULL);

	err = ll_phy_default_set(0x00, 0x00);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	phy_tx = ull_conn_default_phy_tx_get();
	phy_rx = ull_conn_default_phy_rx_get();
	zassert_equal(phy_tx, 0x00, NULL);
	zassert_equal(phy_rx, 0x00, NULL);
	err = ll_phy_default_set(0x01, 0x03);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);
	phy_tx = ull_conn_default_phy_tx_get();
	phy_rx = ull_conn_default_phy_rx_get();
	zassert_equal(phy_tx, 0x01, NULL);
	zassert_equal(phy_rx, 0x03, NULL);
}

void test_hci_dle(void)
{
	uint16_t conn_handle;
	uint64_t err;

	uint16_t tx_octets, tx_time;
	uint16_t max_tx_octets, max_tx_time;
	uint16_t max_rx_octets, max_rx_time;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	tx_octets = 251;
	tx_time = 2400;

	conn_from_pool->llcp.fex.features_used = 0x00;
	err = ll_length_req_send(conn_handle, tx_octets, tx_time);
	zassert_equal(err, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE, "Errorcode %d", err);
	conn_from_pool->llcp.fex.features_used = 0xFFFFFFFF;
	err = ll_length_req_send(conn_handle + 1, tx_octets, tx_time);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);

	ll_length_max_get(&max_tx_octets, &max_tx_time, &max_rx_octets, &max_rx_time);
	zassert_equal(max_tx_octets, LL_LENGTH_OCTETS_RX_MAX, NULL);
	zassert_equal(max_rx_octets, LL_LENGTH_OCTETS_RX_MAX, NULL);
	zassert_equal(max_tx_time, 17040, "Actual time is %d", max_tx_time);
	zassert_equal(max_rx_time, 17040, "Actual time is %d", max_rx_time);

	err = ll_length_default_set(0x00, 0x00);
	ll_length_default_get(&max_tx_octets, &max_tx_time);
	zassert_equal(err, 00, NULL);
	zassert_equal(max_tx_octets, 0x00, NULL);
	zassert_equal(max_tx_time, 0x00, NULL);
	err = ll_length_default_set(0x10, 0x3FF);
	ll_length_default_get(&max_tx_octets, &max_tx_time);
	zassert_equal(err, 00, NULL);
	zassert_equal(max_tx_octets, 0x10, NULL);
	zassert_equal(max_tx_time, 0x3FF, NULL);
	max_tx_octets = ull_conn_default_tx_octets_get();
	max_tx_time = ull_conn_default_tx_time_get();
	zassert_equal(max_tx_octets, 0x10, NULL);
	zassert_equal(max_tx_time, 0x3FF, NULL);
}

void test_hci_terminate(void)
{
	uint16_t conn_handle;
	uint64_t err;

	uint8_t reason;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	reason = 0x01;
	err = ll_terminate_ind_send(conn_handle + 1, reason);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);
	err = ll_terminate_ind_send(conn_handle, reason);
	zassert_equal(err, BT_HCI_ERR_INVALID_PARAM, "Errorcode %d", err);
	reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	err = ll_terminate_ind_send(conn_handle, reason);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

}

void test_hci_conn_update(void)
{
	uint16_t conn_handle;
	uint8_t err;

	uint8_t cmd, status;
	uint16_t interval_min, interval_max, latency, timeout;

	uint8_t unknown_cmds[3U] = { 1U, 3U, 255U };

	cmd = 0x00;
	status = 0x00;
	interval_min = 10U;
	interval_max = 100U;
	latency = 5U;
	timeout = 1000U;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	/* Unknown Connection ID */
	err = ll_conn_update(conn_handle + 1, cmd, status, interval_min, interval_max, latency,
			     timeout);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);

	/* Unknown commands */
	for (uint8_t i = 0U; i < sizeof(unknown_cmds); i++) {
		err = ll_conn_update(conn_handle, unknown_cmds[i], status, interval_min,
				     interval_max, latency, timeout);
		zassert_equal(err, BT_HCI_ERR_UNKNOWN_CMD, "Errorcode %d", err);
	}

	/* Connection Update or Connection Parameter Req. */
	conn_from_pool->llcp.fex.features_used |= BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ);
	err = ll_conn_update(conn_handle, cmd, status, interval_min, interval_max, latency,
			     timeout);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

	conn_from_pool->llcp.fex.features_used &= ~BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ);
	err = ll_conn_update(conn_handle, cmd, status, interval_min, interval_max, latency,
			     timeout);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

	/* Connection Parameter Req. Reply */
	cmd = 2U;
	conn_from_pool->llcp.fex.features_used |= BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ);
	err = ll_conn_update(conn_handle, cmd, status, interval_min, interval_max, latency,
			     timeout);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

	/* Connection Parameter Req. Neg. Reply */
	status = 0x01;
	conn_from_pool->llcp.fex.features_used |= BIT64(BT_LE_FEAT_BIT_CONN_PARAM_REQ);
	err = ll_conn_update(conn_handle, cmd, status, 0U, 0U, 0U, 0U);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
}

/* 'Define' out Central API tests because ull_central.c is mock'ed, so API is not supported */
#define ULL_CENTRAL_MOCKED

void test_hci_chmap(void)
{
#ifndef ULL_CENTRAL_MOCKED
	uint16_t conn_handle;
	uint64_t err;
	uint8_t chmap[5] = {0};
	uint8_t chmap_default[5] = { 0x12, 0x34, 0x56, 0x78, 0x9a };
	uint8_t chmap_test[5] = { 0x42, 0x00, 0x42, 0x00, 0x00 };

	err = ll_chm_update(chmap);
	zassert_equal(err, BT_HCI_ERR_INVALID_PARAM, "Errorcode %d", err);

	conn_handle = ll_conn_handle_get(conn_from_pool);
	memcpy(conn_from_pool->lll.data_chan_map, chmap_default,
	       sizeof(conn_from_pool->lll.data_chan_map));

	test_set_role(conn_from_pool, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	err = ll_chm_get(conn_handle + 1, chmap);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);

	err = ll_chm_get(conn_handle, chmap);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
	zassert_mem_equal(chmap, chmap_default, sizeof(chmap), "Channel map invalid");

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);

	err = ll_chm_get(conn_handle, chmap);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
	zassert_mem_equal(chmap, chmap_default, sizeof(chmap), "Channel map invalid");

	err = ll_chm_update(chmap_test);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

	err = ll_chm_get(conn_handle, chmap);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
	zassert_mem_equal(chmap, chmap_test, sizeof(chmap), "Channel map invalid");
#endif /* !defined(ULL_CENTRAL_MOCKED) */
}

void test_hci_rssi(void)
{
	uint16_t conn_handle;
	uint64_t err;

	uint8_t rssi;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	conn_from_pool->lll.rssi_latest = 0xcd;

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	err = ll_rssi_get(conn_handle + 1, &rssi);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);

	err = ll_rssi_get(conn_handle, &rssi);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
	zassert_equal(rssi, 0xcd, "RSSI %d", err);
}

void test_hci_enc(void)
{
#ifndef ULL_CENTRAL_MOCKED
	uint16_t conn_handle;
	uint64_t err;

	uint8_t rand_nr;
	uint8_t ediv;
	uint8_t error_code;
	uint8_t ltk[5];

	conn_handle = ll_conn_handle_get(conn_from_pool);

	test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

	error_code = 0;

	err = ll_enc_req_send(conn_handle + 1, &rand_nr, &ediv, &ltk[0]);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);
	err = ll_enc_req_send(conn_handle, &rand_nr, &ediv, &ltk[0]);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);

	test_set_role(conn_from_pool, BT_HCI_ROLE_PERIPHERAL);
	err = ll_start_enc_req_send(conn_handle + 1, error_code, &ltk[0]);
	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Errorcode %d", err);
	err = ll_start_enc_req_send(conn_handle, error_code, &ltk[0]);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Errorcode %d", err);
#endif /* !defined(ULL_CENTRAL_MOCKED) */
}

void test_main(void)
{
	ztest_test_suite(
		hci_interface,
		ztest_unit_test_setup_teardown(test_hci_feature_exchange, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_feature_exchange_wrong_handle, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_version_ind, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_version_ind_wrong_handle, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_apto, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_phy, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_dle, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_terminate, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_conn_update, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_chmap, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_enc, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_hci_rssi, setup, unit_test_noop)

	);

	ztest_run_test_suite(hci_interface);
}
