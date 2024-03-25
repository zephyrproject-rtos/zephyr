/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>

#define EXPECTED_NUM_ROTATIONS 5

struct adv_set_data_t {
	bt_addr_le_t	old_addr;
	uint8_t         rpa_rotations;
	int64_t			old_time;
};

static	uint8_t	adv_index;
static	struct	adv_set_data_t	adv_set_data[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static bool data_cb(struct bt_data *data, void *user_data)
{
	switch (data->type) {
	case BT_DATA_MANUFACTURER_DATA:
		adv_index = data->data[0];
		return false;
	default:
		return true;
	}
}

static void test_address(bt_addr_le_t *addr)
{
	int64_t diff_ms;
	static int64_t rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

	if (!BT_ADDR_IS_RPA(&addr->a)) {
		FAIL("Bluetooth address is not RPA\n");
	}

	/* Only save the address if this is the first scan */
	if (bt_addr_le_eq(&adv_set_data[adv_index].old_addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&adv_set_data[adv_index].old_addr, addr);
		adv_set_data[adv_index].old_time = 0;
		return;
	}

	diff_ms = k_uptime_get() - adv_set_data[adv_index].old_time;

	if (diff_ms < rpa_timeout_ms) {
		return;
	}

	printk("Ad set %d Old ", adv_index);
	print_address(&adv_set_data[adv_index].old_addr);
	printk("Ad set %d New ", adv_index);
	print_address(addr);

	/*	For the first 2 rpa rotations, either of the first 2 adv sets returns false.
	 *	Hence first 2 adv sets continue with old rpa in first 2 rpa rotations.
	 *	For the next 2 rpa rotations, either of the last 2 adv sets returns false.
	 *	Hence last 2 adv sets continue with old rpa in next 2 rpa rotations.
	 */
	if ((adv_set_data[adv_index].rpa_rotations % CONFIG_BT_EXT_ADV_MAX_ADV_SET)  < 2) {

		if (adv_index < 2) {
			if (!bt_addr_le_eq(addr, &adv_set_data[adv_index].old_addr)) {
				FAIL("Adv sets should continue with old rpa\n");
			}
		} else {
			if (bt_addr_le_eq(addr, &adv_set_data[adv_index].old_addr)) {
				FAIL("New RPA should have been generated\n");
			}
		}
	} else {
		if (adv_index < 2) {
			if (bt_addr_le_eq(addr, &adv_set_data[adv_index].old_addr)) {
				FAIL("New RPA should have been generated\n");
			}
		} else {
			if (!bt_addr_le_eq(addr, &adv_set_data[adv_index].old_addr)) {
				FAIL("Adv sets should continue with old rpa\n");
			}
		}
	}

	adv_set_data[adv_index].rpa_rotations++;
	if (adv_set_data[adv_index].rpa_rotations > EXPECTED_NUM_ROTATIONS) {
		PASS("PASS\n");
	}

	adv_set_data[adv_index].old_time = k_uptime_get();
	bt_addr_le_copy(&adv_set_data[adv_index].old_addr, addr);
}

static void cb_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *ad)
{
	bt_data_parse(ad, data_cb, NULL);
	test_address((bt_addr_le_t *)addr);
}

void start_rpa_scanning(void)
{
	/* Start passive scanning */
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = 0x0040,
		.window     = 0x0020,
	};

	int err = bt_le_scan_start(&scan_param, cb_device_found);

	if (err) {
		FAIL("Failed to start scanning");
	}
}

void tester_verify_rpa_procedure(void)
{
	/* Enable bluetooth */
	int err = bt_enable(NULL);

	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	err = settings_load();
	if (err) {
		FAIL("Failed to enable settings (err %d\n)", err);
	}

	start_rpa_scanning();
	/* The rest of the test is driven by the callback */
}
