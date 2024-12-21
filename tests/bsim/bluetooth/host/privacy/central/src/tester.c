/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/toolchain.h>

#include <babblekit/testcase.h>

DEFINE_FLAG(flag_new_address);
DEFINE_FLAG(flag_connected);

void scanned_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info)
{
	static bool init;
	static int64_t old_time;
	static bt_addr_le_t old_addr;
	bt_addr_le_t new_addr;

	if (bst_result == Passed) {
		return;
	}

	if (!init) {
		old_addr = *info->addr;
		old_time = k_uptime_get();
		init = true;
	}

	new_addr = *info->addr;

	/* Check if the scan request comes from a new address */
	if (bt_addr_le_cmp(&old_addr, &new_addr)) {
		int64_t new_time, diff, time_diff_ms, rpa_timeout_ms;

		printk("Scanned request from new ");
		print_address(info->addr);

		/* Ensure the RPA rotation occurs within +-10% of CONFIG_BT_RPA_TIMEOUT */
		new_time = k_uptime_get();
		time_diff_ms = new_time - old_time;
		rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

		if (time_diff_ms > rpa_timeout_ms) {
			diff = time_diff_ms - rpa_timeout_ms;
		} else {
			diff = rpa_timeout_ms - time_diff_ms;
		}

		if (diff > rpa_timeout_ms * 0.10) {
			FAIL("RPA rotation did not occur within +-10%% of CONFIG_BT_RPA_TIMEOUT");
		}
		old_time = new_time;

		SET_FLAG(flag_new_address);
	}

	old_addr = new_addr;
}

static void connected_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info)
{
	SET_FLAG(flag_connected);
}

static struct bt_le_ext_adv_cb adv_callbacks = {
	.scanned = scanned_cb,
	.connected = connected_cb
};

void start_advertising(void)
{
	int err;
	uint8_t mfg_data[] = {0xAB, 0xCD, 0xEF};
	const struct bt_data sd[] = {BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3)};
	struct bt_le_adv_param params;
	struct bt_le_ext_adv_start_param start_params;
	struct bt_le_ext_adv *adv;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	/* Create advertising set */
	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.secondary_max_skip = 0;
	params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_SCANNABLE |
			 BT_LE_ADV_OPT_NOTIFY_SCAN_REQ;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
	params.peer = NULL;

	err = bt_le_ext_adv_create(&params, &adv_callbacks, &adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
	}

	/* Set scan data */
	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, ARRAY_SIZE(sd));
	if (err) {
		FAIL("Failed to set advertising data (err %d)", err);
	}

	/* Start advertising */
	start_params.timeout = 0;
	start_params.num_events = 0;

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		FAIL("Failed to start advertising (err %d)\n", err);
	}
}

void tester_procedure(void)
{
	start_advertising();

	for (int i = 0; i < 5; i++) {
		WAIT_FOR_FLAG(flag_new_address);
		UNSET_FLAG(flag_new_address);
	}

	PASS("PASS\n");
}

void tester_procedure_periph_delayed_start_of_conn_adv(void)
{
	backchannel_init(0);

	int err;
	struct bt_le_adv_param params =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
				     BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	struct bt_data ad;
	struct bt_le_ext_adv *adv;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Failed to enable bluetooth (err %d)");

	/* Advertiser to use a long RPA timeout */
	err = bt_le_set_rpa_timeout(100);
	TEST_ASSERT(!err, "Failed to set RPA timeout (err %d)", err);

	err = bt_le_ext_adv_create(&params, &adv_callbacks, &adv);
	TEST_ASSERT(!err, "Failed to create advertising set (err %d)", err);

	ad.type = BT_DATA_NAME_COMPLETE;
	ad.data_len = strlen(CONFIG_BT_DEVICE_NAME);
	ad.data = (const uint8_t *)CONFIG_BT_DEVICE_NAME;

	err = bt_le_ext_adv_set_data(adv, &ad, 1, NULL, 0);
	TEST_ASSERT(!err, "Failed to set advertising data (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(!err, "Failed to start advertiser (err %d)", err);

	backchannel_sync_wait();

	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(!err, "Failed to stop advertiser (err %d)", err);

	/* Wait a few RPA cycles before restaring the advertiser to force RPA timeout
	 * on the DUT.
	 */
	k_sleep(K_SECONDS(7));

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(!err, "Failed to restart advertiser (err %d)", err);

	WAIT_FOR_FLAG(flag_connected);

	PASS("PASS\n");
}
