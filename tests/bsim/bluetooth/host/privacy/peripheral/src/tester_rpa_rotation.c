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
	int64_t		old_time;
	int		rpa_rotations;
	bool            addr_set;
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

static void validate_rpa_addr_generated_for_adv_sets(void)
{
	for (int i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		if (!adv_set_data[i].addr_set) {
			return;
		}
	}
	if (bt_addr_le_eq(&adv_set_data[0].old_addr, &adv_set_data[1].old_addr)) {
		/* With RPA sharing mode disabled, the first two adv sets should have
		 * a different address even though they use the same Bluetooth ID.
		 */
		if (!IS_ENABLED(CONFIG_BT_RPA_SHARING)) {
			FAIL("RPA same for adv sets with same id and RPA sharing disabled\n");
		}
	} else {
		/* In the RPA sharing mode, the first two adv sets should have
		 * the same address as they use the same Bluetooth ID.
		 */
		if (IS_ENABLED(CONFIG_BT_RPA_SHARING)) {
			FAIL("RPA not same for adv sets with same id and RPA sharing enabled\n");
		}
	}
	if (bt_addr_le_eq(&adv_set_data[0].old_addr, &adv_set_data[3].old_addr)) {
		FAIL("RPA same for adv sets with different id's\n");
	}
	if (bt_addr_le_eq(&adv_set_data[1].old_addr, &adv_set_data[3].old_addr)) {
		FAIL("RPA same for adv sets with different id's\n");
	}
	adv_set_data[0].addr_set	= false;
	adv_set_data[1].addr_set	= false;
	adv_set_data[2].addr_set	= false;
}

static void test_address(bt_addr_le_t *addr)
{
	int64_t diff_ms, rpa_timeout_ms;

	if (!BT_ADDR_IS_RPA(&addr->a)) {
		FAIL("Bluetooth address is not RPA\n");
	}

	/* Only save the address + time if this is the first scan */
	if (bt_addr_le_eq(&adv_set_data[adv_index].old_addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&adv_set_data[adv_index].old_addr, addr);
		adv_set_data[adv_index].old_time = k_uptime_get();
		return;
	}

	/* Compare old and new address */
	if (bt_addr_le_eq(&adv_set_data[adv_index].old_addr, addr)) {
		return;
	}
	adv_set_data[adv_index].addr_set = true;
	printk("Ad set %d Old ", adv_index);
	print_address(&adv_set_data[adv_index].old_addr);
	printk("Ad set %d New ", adv_index);
	print_address(addr);

	adv_set_data[adv_index].rpa_rotations++;

	/* Ensure the RPA rotation occurs within +-10% of CONFIG_BT_RPA_TIMEOUT */
	diff_ms = k_uptime_get() - adv_set_data[adv_index].old_time;
	rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

	if (abs(diff_ms - rpa_timeout_ms) > (rpa_timeout_ms / 10)) {
		FAIL("RPA rotation did not occur within +-10%% of CONFIG_BT_RPA_TIMEOUT\n");
	}

	bt_addr_le_copy(&adv_set_data[adv_index].old_addr, addr);
	adv_set_data[adv_index].old_time = k_uptime_get();
	validate_rpa_addr_generated_for_adv_sets();

	if (adv_set_data[adv_index].rpa_rotations > EXPECTED_NUM_ROTATIONS) {
		PASS("PASS\n");
	}
}

static void cb_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *ad)
{
	bt_data_parse(ad, data_cb, NULL);
	test_address((bt_addr_le_t *)addr);
}

void start_scanning(void)
{
	/* Start passive scanning */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = 0x0040,
		.window     = 0x0020,
	};

	int err = bt_le_scan_start(&scan_param, cb_device_found);

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

	err = settings_load();
	if (err) {
		FAIL("Failed to enable settings (err %d\n)", err);
	}

	start_scanning();

	/* The rest of the test is driven by the callback */
}
