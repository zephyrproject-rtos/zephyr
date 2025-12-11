/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/settings/settings.h>

#include "babblekit/testcase.h"

#include "common/bt_str.h"

#define ID_1 1
#define ID_2 2

#define ADV_SET_INDEX_1		0x00
#define ADV_SET_INDEX_2		0x01
#define ADV_SET_INDEX_3		0x02
#define ADV_SET_INDEX_4		0x03

static struct bt_le_ext_adv *adv_set[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

	static const struct bt_data ad_id[] = {
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_1),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_2),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_3),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_4),
};

bool rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	/*	Return true to rotate the current RPA.
	 *	Return false to continue with old RPA.
	 */
	int	err;
	struct	bt_le_ext_adv_info info;
	static int rpa_count = -1;
	static int64_t	old_time;
	static int64_t	rpa_timeout_ms;
	int64_t	diff_ms;

	diff_ms = k_uptime_get() - old_time;
	rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

	if (diff_ms >= rpa_timeout_ms) {
		rpa_count++;
		old_time = k_uptime_get();
	}

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		return false;
	}
	printk("%s advertiser[%d] RPA %s\n", __func__, info.id, bt_addr_le_str(info.addr));

	/* Every rpa rotation one of the adv set returns false based on adv index */
	if (rpa_count == bt_le_ext_adv_get_index(adv)) {
		printk("adv index %d returns false\n", bt_le_ext_adv_get_index(adv));
		if (rpa_count == CONFIG_BT_EXT_ADV_MAX_ADV_SET - 1) {
			/* Reset RPA counter */
			rpa_count = -1;
		}
		return false;
	}
	return true;
}

static void create_adv(struct bt_le_ext_adv **adv, int id)
{
	int err;
	struct bt_le_adv_param params;
	static struct bt_le_ext_adv_cb cb_adv;

	cb_adv.rpa_expired = rpa_expired_cb;
	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_EXT_ADV;
	params.id = id;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;

	err = bt_le_ext_adv_create(&params, &cb_adv, adv);
	if (err) {
		TEST_FAIL("Failed to create advertiser (%d)", err);
	}
}

void start_rpa_advertising(void)
{
	int err;
	size_t bt_id_count;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Failed to enable bluetooth (err %d)", err);
	}

	err = settings_load();
	if (err) {
		TEST_FAIL("Failed to enable settings (err %d)", err);
	}

	bt_id_get(NULL, &bt_id_count);

	if (bt_id_count == 1) {
		int id_a;
		int id_b;

		printk("No extra identity found in settings, creating new ones...\n");

		id_a = bt_id_create(NULL, NULL);
		if (id_a != ID_1) {
			TEST_FAIL("bt_id_create id_a failed (err %d)", id_a);
		}

		id_b = bt_id_create(NULL, NULL);
		if (id_b != ID_2) {
			TEST_FAIL("bt_id_create id_b failed (err %d)", id_b);
		}
	} else {
		printk("Extra identities loaded from settings\n");
	}

	bt_id_get(NULL, &bt_id_count);
	if (bt_id_count != CONFIG_BT_ID_MAX) {
		TEST_FAIL("bt_id_get returned incorrect number of identities %u", bt_id_count);
	}

	for (int i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		/* Create first 2 advertising sets with one id and last 2 advertising sets with
		 * different id.
		 */
		if (i < 2) {
			create_adv(&adv_set[i], ID_1);
		} else {
			create_adv(&adv_set[i], ID_2);
		}
		/* Set extended advertising data */
		err = bt_le_ext_adv_set_data(adv_set[i], &ad_id[i], 1, NULL, 0);
		if (err) {
			TEST_FAIL("Failed to set advertising data for set %d (err %d)", i, err);
		}

		err = bt_le_ext_adv_start(adv_set[i], BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			TEST_FAIL("Failed to start advertising (err %d)", err);
		}
	}
}

void dut_rpa_expired_procedure(void)
{
	start_rpa_advertising();

	/* Nothing to do */
	TEST_PASS("PASS");
}
