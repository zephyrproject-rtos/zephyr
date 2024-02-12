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
#include <zephyr/settings/settings.h>

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

bool rpa_expired_cb_returns_true(struct bt_le_ext_adv *adv)
{
	/* Return true to rotate the current RPA */
	int err;
	struct bt_le_ext_adv_info info;

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		return false;
	}
	printk("%s advertiser[%d] RPA %s\n", __func__, info.id, bt_addr_le_str(info.addr));

	return true;
}

bool rpa_expired_cb_returns_false(struct bt_le_ext_adv *adv)
{
	/* Return false not to rotate the current RPA */
	int err;
	struct bt_le_ext_adv_info info;

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		return false;
	}
	printk("%s advertiser[%d] RPA %s\n", __func__, info.id, bt_addr_le_str(info.addr));

	return false;
}

static void create_adv(struct bt_le_ext_adv **adv, int id, bool expired_return)
{
	int err;
	struct bt_le_adv_param params;
	static struct bt_le_ext_adv_cb cb_adv[] = {
		{.rpa_expired = rpa_expired_cb_returns_true},
		{.rpa_expired = rpa_expired_cb_returns_false}
	};

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_EXT_ADV;
	params.id = id;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;

	err = bt_le_ext_adv_create(&params, expired_return ? &cb_adv[0] : &cb_adv[1], adv);
	if (err) {
		FAIL("Failed to create advertiser (%d)\n", err);
	}
}

void start_rpa_advertising(void)
{
	int err;
	size_t bt_id_count;

	/* Enable bluetooth */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Failed to enable bluetooth (err %d\n)", err);
	}

	err = settings_load();
	if (err) {
		FAIL("Failed to enable settings (err %d\n)", err);
	}

	bt_id_get(NULL, &bt_id_count);

	if (bt_id_count == 1) {
		int id_a;
		int id_b;

		printk("No extra identity found in settings, creating new ones...\n");

		id_a = bt_id_create(NULL, NULL);
		if (id_a != ID_1) {
			FAIL("bt_id_create id_a failed (err %d)\n", id_a);
		}

		id_b = bt_id_create(NULL, NULL);
		if (id_b != ID_2) {
			FAIL("bt_id_create id_b failed (err %d)\n", id_b);
		}
	} else {
		printk("Extra identities loaded from settings\n");
	}

	bt_id_get(NULL, &bt_id_count);
	if (bt_id_count != CONFIG_BT_ID_MAX) {
		FAIL("bt_id_get returned incorrect number of identities %u\n", bt_id_count);
	}

	for (int i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		/* Create first 2 advertising sets with one id and for both sets,rpa_expied_cb
		 * returns true.
		 * Create remaining 2 sets with different id and last adv set's rpa_expired cb
		 * returns false.
		 *
		 * So for first two adv sets with same id new rpa's will be generated every
		 * rotation and for last two adv sets with same id, rpa will continue with
		 * only one rpa through out the rotations since one of the adv set returned
		 * false.
		 */
		switch (i)  {
		case ADV_SET_INDEX_1:
		case ADV_SET_INDEX_2:
			create_adv(&adv_set[i], ID_1, true);
			break;
		case ADV_SET_INDEX_3:
			create_adv(&adv_set[i], ID_2, true);
			break;
		case ADV_SET_INDEX_4:
			create_adv(&adv_set[i], ID_2, false);
			break;
		default:
			printk("Shouldn't be here\n");
			break;
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

void dut_rpa_expired_procedure(void)
{
	start_rpa_advertising();

	/* Nothing to do */
	PASS("PASS\n");
}
