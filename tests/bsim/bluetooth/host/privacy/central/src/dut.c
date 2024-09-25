/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <babblekit/testcase.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

void start_scanning(void)
{
	int err;
	struct bt_le_scan_param param;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	/* Start active scanning */
	param.type = BT_LE_SCAN_TYPE_ACTIVE;
	param.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE;
	param.interval = BT_GAP_SCAN_FAST_INTERVAL;
	param.window = BT_GAP_SCAN_FAST_WINDOW;
	param.timeout = 0;
	param.interval_coded = 0;
	param.window_coded = 0;

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		FAIL("Failed to start scanning");
	}
}

void dut_procedure(void)
{
	start_scanning();

	/* Nothing to do */

	PASS("PASS\n");
}

void dut_procedure_connect_short_rpa_timeout(void)
{
	backchannel_init(1);

	const uint16_t rpa_timeout_s = 1;

	int err;
	bt_addr_le_t peer = {};
	struct bt_conn *conn = NULL;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Failed to enable bluetooth (err %d)");

	/* Central to use a short RPA timeout */
	err = bt_le_set_rpa_timeout(rpa_timeout_s);
	TEST_ASSERT(!err, "Failed to set RPA timeout (err %d)", err);

	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	/* Indicate to the peer device that we have found the advertiser. */
	backchannel_sync_send();

	/* Create a connection using that address */
	err = bt_testlib_connect(&peer, &conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	PASS("PASS\n");
}

void dut_procedure_connect_timeout(void)
{
	const uint16_t rpa_timeout_s = 1;

	int err;
	bt_addr_le_t peer = {.type = BT_ADDR_LE_RANDOM, .a = {.val = {1, 2, 3, 4, 5, 6}}};
	struct bt_conn *conn = NULL;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Failed to enable bluetooth (err %d)", err);

	/* Central to use a short RPA timeout */
	err = bt_le_set_rpa_timeout(rpa_timeout_s);
	TEST_ASSERT(!err, "Failed to set RPA timeout (err %d)", err);

	int64_t old_time = k_uptime_get();

	/* Create a connection using that address */
	err = bt_testlib_connect(&peer, &conn);
	TEST_ASSERT(err == BT_HCI_ERR_UNKNOWN_CONN_ID,
		    "Expected connection establishment to time out (err %d)", err);

	int64_t new_time = k_uptime_get();
	int64_t time_diff_ms = new_time - old_time;
	int64_t expected_conn_timeout_ms = CONFIG_BT_CREATE_CONN_TIMEOUT * 1000;
	int64_t diff_to_expected_ms = abs(time_diff_ms - expected_conn_timeout_ms);

	printk("Connection creation timed out after %d ms\n", time_diff_ms);
	TEST_ASSERT(diff_to_expected_ms < 0.1 * expected_conn_timeout_ms,
		    "Connection timeout not within 10 \%% of expected timeout");

	PASS("PASS\n");
}
