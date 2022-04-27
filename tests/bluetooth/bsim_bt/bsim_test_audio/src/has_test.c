/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_HAS
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/has.h>

#include "common.h"

extern enum bst_result_t bst_result;

const uint8_t test_preset_index_1 = 0x01;
const uint8_t test_preset_index_5 = 0x05;
const char *test_preset_name_1 = "test_preset_name_1";
const char *test_preset_name_5 = "test_preset_name_5";
const enum bt_has_properties test_preset_properties = BT_HAS_PROP_AVAILABLE;

static int preset_select(uint8_t index, bool sync)
{
	return 0;
}

static const struct bt_has_preset_ops preset_ops = {
	.select = preset_select,
};

static void test_main(void)
{
	struct bt_has_preset_register_param param;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	param.index = test_preset_index_5;
	param.properties = test_preset_properties;
	param.name = test_preset_name_5;
	param.ops = &preset_ops,

	err = bt_has_preset_register(&param);
	if (err) {
		FAIL("Preset register failed (err %d)\n", err);
		return;
	}

	param.index = test_preset_index_1;
	param.properties = test_preset_properties;
	param.name = test_preset_name_1;

	err = bt_has_preset_register(&param);
	if (err) {
		FAIL("Preset register failed (err %d)\n", err);
		return;
	}

	printk("Presets registered\n");

	PASS("HAS passed\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_has);
}
#else
struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_HAS */
