/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_HAS_CLIENT
#include <zephyr/bluetooth/audio/has.h>

#include "common.h"

extern enum bst_result_t bst_result;

extern const char *test_preset_name_1;
extern const char *test_preset_name_5;
extern const uint8_t test_preset_index_1;
extern const uint8_t test_preset_index_5;
extern const enum bt_has_properties test_preset_properties;

CREATE_FLAG(g_service_discovered);
CREATE_FLAG(g_preset_switched);
CREATE_FLAG(g_preset_1_found);
CREATE_FLAG(g_preset_5_found);

static struct bt_has *g_has;
static uint8_t g_active_index;

static void discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
			enum bt_has_hearing_aid_type type, enum bt_has_capabilities caps)
{
	if (err) {
		FAIL("Failed to discover HAS (err %d)\n", err);
		return;
	}

	printk("HAS discovered type %d caps %d\n", type, caps);

	g_has = has;
	SET_FLAG(g_service_discovered);
}

static void preset_switch_cb(struct bt_has *has, int err, uint8_t index)
{
	if (err != 0) {
		return;
	}

	printk("Active preset index %d\n", index);

	SET_FLAG(g_preset_switched);
	g_active_index = index;
}

static void check_preset_record(const struct bt_has_preset_record *record,
				enum bt_has_properties expected_properties,
				const char *expected_name)
{
	if (record->properties != expected_properties || strcmp(record->name, expected_name)) {
		FAIL("mismatch 0x%02x %s vs 0x%02x %s expected\n",
		     record->properties, record->name, expected_properties, expected_name);
	}
}

static void preset_read_rsp_cb(struct bt_has *has, int err,
			       const struct bt_has_preset_record *record, bool is_last)
{
	if (err) {
		FAIL("%s: err %d\n", __func__, err);
		return;
	}

	if (record->index == test_preset_index_1) {
		SET_FLAG(g_preset_1_found);

		check_preset_record(record, test_preset_properties, test_preset_name_1);
	} else if (record->index == test_preset_index_5) {
		SET_FLAG(g_preset_5_found);

		check_preset_record(record, test_preset_properties, test_preset_name_5);
	} else {
		FAIL("unexpected index 0x%02x", record->index);
	}
}

static const struct bt_has_client_cb has_cb = {
	.discover = discover_cb,
	.preset_switch = preset_switch_cb,
	.preset_read_rsp = preset_read_rsp_cb,
};

static bool test_preset_switch(uint8_t index)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_set(g_has, index, false);
	if (err < 0) {
		printk("%s (err %d)\n", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == index;
}

static bool test_preset_next(uint8_t active_index_expected)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_next(g_has, false);
	if (err < 0) {
		printk("%s (err %d)\n", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == active_index_expected;
}

static bool test_preset_prev(uint8_t active_index_expected)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_prev(g_has, false);
	if (err < 0) {
		printk("%s (err %d)\n", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == active_index_expected;
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_has_client_cb_register(&has_cb);
	if (err < 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
		return;
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	err = bt_has_client_discover(default_conn);
	if (err < 0) {
		FAIL("Failed to discover HAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_service_discovered);
	WAIT_FOR_COND(g_preset_switched);

	err = bt_has_client_presets_read(g_has, BT_HAS_PRESET_INDEX_FIRST, 255);
	if (err < 0) {
		FAIL("Failed to read presets (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_preset_1_found);
	WAIT_FOR_COND(g_preset_5_found);

	if (!test_preset_switch(test_preset_index_1)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_switch(test_preset_index_5)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_next(test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_next(test_preset_index_5)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_next(test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_prev(test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_prev(test_preset_index_1)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_prev(test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	PASS("HAS main PASS\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_has_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_has);
}
#else
struct bst_test_list *test_has_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_HAS_CLIENT */
