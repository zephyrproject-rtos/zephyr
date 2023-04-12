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

#define EXPECTED_NUM_ROTATIONS 5

static bt_addr_le_t old_addr;
static int64_t old_time;
static int rpa_rotations;

static void test_address(bt_addr_le_t *addr)
{
	int64_t diff_ms, rpa_timeout_ms;

	/* Only save the address + time if this is the first scan */
	if (bt_addr_le_eq(&old_addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&old_addr, addr);
		old_time = k_uptime_get();
		return;
	}

	/* Compare old and new address */
	if (bt_addr_le_eq(&old_addr, addr)) {
		return;
	}

	printk("Old ");
	print_address(&old_addr);
	printk("New ");
	print_address(addr);

	rpa_rotations++;

	/* Ensure the RPA rotation occurs within +-10% of CONFIG_BT_RPA_TIMEOUT */
	diff_ms = k_uptime_get() - old_time;
	rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

	if (abs(diff_ms - rpa_timeout_ms) > (rpa_timeout_ms / 10)) {
		FAIL("RPA rotation did not occur within +-10% of CONFIG_BT_RPA_TIMEOUT\n");
	}

	bt_addr_le_copy(&old_addr, addr);
	old_time = k_uptime_get();

	if (rpa_rotations > EXPECTED_NUM_ROTATIONS) {
		PASS("PASS\n");
	}
}

static void cb_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *ad)
{
	test_address((bt_addr_le_t *)addr);
}

void start_scanning(void)
{
	struct bt_le_scan_param params;

	/* Start passive scanning */
	params.type = BT_LE_SCAN_TYPE_PASSIVE;
	params.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE;
	params.interval = BT_GAP_SCAN_FAST_INTERVAL;
	params.window = BT_GAP_SCAN_FAST_WINDOW;
	params.timeout = 0;

	int err = bt_le_scan_start(&params, cb_device_found);
	if (err) {
		FAIL("Failed to start scanning");
	}
}

void tester_procedure(void)
{
	/* Enable bluetooth */
	int err = bt_enable(NULL);

	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	start_scanning();

	/* The rest of the test is driven by the callback */
}
