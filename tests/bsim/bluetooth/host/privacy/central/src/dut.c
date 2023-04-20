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
