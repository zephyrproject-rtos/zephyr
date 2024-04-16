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

#include "common/bt_str.h"

#define ADV_SET_INDEX_ONE   0x00
#define ADV_SET_INDEX_TWO   0x01
#define ADV_SET_INDEX_THREE 0x02

static struct bt_le_ext_adv *adv_set[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static const struct bt_data ad_id[] = {
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_ONE),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_TWO),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, ADV_SET_INDEX_THREE),
};

bool cb_rpa_expired(struct bt_le_ext_adv *adv)
{
	/* Return true to rotate the current RPA */
	int err;
	struct bt_le_ext_adv_info info;

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		return false;
	}
	printk("advertiser[%d] RPA %s\n", info.id, bt_addr_le_str(info.addr));
	return true;
}

static void create_adv(struct bt_le_ext_adv **adv, int id)
{
	int err;
	struct bt_le_adv_param params;
	static struct bt_le_ext_adv_cb cb_adv;

	cb_adv.rpa_expired = cb_rpa_expired;
	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_EXT_ADV;
	params.id = id;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;

	err = bt_le_ext_adv_create(&params, &cb_adv, adv);
	if (err) {
		FAIL("Failed to create advertiser (%d)\n", err);
	}
}

void start_advertising(void)
{
	int err;
	int id_a;
	int id_b;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	id_a = bt_id_create(NULL, NULL);
	if (id_a < 0) {
		FAIL("bt_id_create id_a failed (err %d)\n", id_a);
	}

	id_b = bt_id_create(NULL, NULL);
	if (id_b < 0) {
		FAIL("bt_id_create id_b failed (err %d)\n", id_b);
	}

	for (int i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {

		if (i != ADV_SET_INDEX_THREE) {
			/* Create advertising set 1 and 2 with same id */
			create_adv(&adv_set[i], id_a);
		} else {
			/* Create advertising set 3 with different id */
			create_adv(&adv_set[i], id_b);
		}

		/* Set extended advertising data */
		err = bt_le_ext_adv_set_data(adv_set[i], &ad_id[i], 1, NULL, 0);
		if (err) {
			FAIL("Failed to set advertising data for set %d (err %d)\n", i, err);
		}

		err = bt_le_ext_adv_start(adv_set[i], BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			FAIL("Failed to start advertising (err %d)\n", err);
		}
	}
}

void dut_procedure(void)
{
	start_advertising();

	/* Nothing to do */
	PASS("PASS\n");
}
