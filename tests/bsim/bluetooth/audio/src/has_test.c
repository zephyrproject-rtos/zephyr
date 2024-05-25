/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/has.h>

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(has_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

const uint8_t test_preset_index_1 = 0x01;
const uint8_t test_preset_index_3 = 0x03;
const uint8_t test_preset_index_5 = 0x05;
const char *test_preset_name_1 = "test_preset_name_1";
const char *test_preset_name_3 = "test_preset_name_3";
const char *test_preset_name_5 = "test_preset_name_5";
const enum bt_has_properties test_preset_properties = BT_HAS_PROP_AVAILABLE;

static int preset_select(uint8_t index, bool sync)
{
	return 0;
}

static const struct bt_has_preset_ops preset_ops = {
	.select = preset_select,
};

static void test_common(void)
{
	struct bt_has_features_param has_param = {0};
	struct bt_has_preset_register_param preset_param;

	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Advertising successfully started");

	has_param.type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
	has_param.preset_sync_support = true;

	err = bt_has_register(&has_param);
	if (err) {
		FAIL("HAS register failed (err %d)\n", err);
		return;
	}

	has_param.type = BT_HAS_HEARING_AID_TYPE_MONAURAL;
	has_param.preset_sync_support = false;

	err = bt_has_features_set(&has_param);
	if (err) {
		FAIL("HAS register failed (err %d)\n", err);
		return;
	}

	preset_param.index = test_preset_index_5;
	preset_param.properties = test_preset_properties;
	preset_param.name = test_preset_name_5;
	preset_param.ops = &preset_ops,

	err = bt_has_preset_register(&preset_param);
	if (err) {
		FAIL("Preset register failed (err %d)\n", err);
		return;
	}

	preset_param.index = test_preset_index_1;
	preset_param.properties = test_preset_properties;
	preset_param.name = test_preset_name_1;

	err = bt_has_preset_register(&preset_param);
	if (err) {
		FAIL("Preset register failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Presets registered");

	PASS("HAS passed\n");
}

static void test_main(void)
{
	test_common();

	PASS("HAS passed\n");
}

static void test_offline_behavior(void)
{
	struct bt_has_preset_register_param preset_param;
	struct bt_has_features_param has_param = {0};
	int err;

	test_common();

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	preset_param.index = test_preset_index_3;
	preset_param.properties = test_preset_properties;
	preset_param.name = test_preset_name_3;
	preset_param.ops = &preset_ops,

	err = bt_has_preset_register(&preset_param);
	if (err) {
		FAIL("Preset register failed (err %d)\n", err);
		return;
	}

	has_param.type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
	has_param.preset_sync_support = true;

	err = bt_has_features_set(&has_param);
	if (err) {
		FAIL("Features set failed (err %d)\n", err);
		return;
	}

	err = bt_has_preset_active_set(test_preset_index_3);
	if (err) {
		FAIL("Preset activation failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	PASS("HAS passed\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "has_offline_behavior",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_offline_behavior,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	if (IS_ENABLED(CONFIG_BT_HAS)) {
		return bst_add_tests(tests, test_has);
	} else {
		return tests;
	}
}
