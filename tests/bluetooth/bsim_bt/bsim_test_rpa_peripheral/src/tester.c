/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/conn.h"
#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>

static volatile int64_t old_time, new_time;
static bt_addr_le_t old_addr;
static bt_addr_le_t *new_addr;

static void cb_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *ad)
{
	static bool init;

	if (!init) {
		old_addr = *addr;
		old_time = k_uptime_get();
		init = true;
	}

	new_addr = (bt_addr_le_t *)addr;
	new_time = k_uptime_get();
}

void start_scanning(void)
{
	int err;
	struct bt_le_scan_param params;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	/* Start passive scanning */
	params.type = BT_LE_SCAN_TYPE_PASSIVE;
	params.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE;
	params.interval = BT_GAP_SCAN_FAST_INTERVAL;
	params.window = BT_GAP_SCAN_FAST_WINDOW;

	err = bt_le_scan_start(&params, cb_device_found);
	if (err) {
		FAIL("Failed to start scanning");
	}
}

void tester_procedure(void)
{
	int err;

	/* Setup synchronization channel */
	backchannel_init(DUT_PERIPHERAL_ID);

	start_scanning();

	/* Wait for the first address rotation */
	backchannel_sync_wait();

	for (uint16_t i = 0; i < 5; i++) {
		int64_t diff, time_diff_ms, rpa_timeout_ms;

		backchannel_sync_wait();

		/* Compare old and new address */
		err = bt_addr_le_cmp(&old_addr, new_addr);
		if (err == 0) {
			FAIL("RPA did not rotate", err);
		}

		/* Ensure the RPA rotation occurs within +-10% of CONFIG_BT_RPA_TIMEOUT */
		time_diff_ms = new_time - old_time;
		rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

		if (time_diff_ms > rpa_timeout_ms) {
			diff = time_diff_ms - rpa_timeout_ms;
		} else {
			diff = rpa_timeout_ms - time_diff_ms;
		}

		if (diff > (rpa_timeout_ms / 10)) {
			FAIL("RPA rotation did not occur within +-10%% of CONFIG_BT_RPA_TIMEOUT");
		}

		printk("Old ");
		print_address(&old_addr);
		printk("New ");
		print_address(new_addr);

		old_addr = *new_addr;
		old_time = new_time;
	}

	PASS("PASS\n");
}
