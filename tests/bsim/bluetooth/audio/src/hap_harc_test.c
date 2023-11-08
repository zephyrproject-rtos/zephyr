/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <argparse.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/hap.h>
#include <zephyr/bluetooth/audio/has.h>

#include "common.h"

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"
#include "testlib/log_utils.h"
#include "testlib/scan.h"
#include "testlib/security.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hap_harc_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

extern const char *test_preset_name_1;
extern const char *test_preset_name_3;
extern const char *test_preset_name_5;
extern const uint8_t test_preset_index_1;
extern const uint8_t test_preset_index_3;
extern const uint8_t test_preset_index_5;
extern const enum bt_has_properties test_preset_properties;

CREATE_FLAG(g_harc_connected);
CREATE_FLAG(g_harc_proc_complete);

static void harc_connected_cb(struct bt_hap_harc *harc, int err)
{
	if (err != 0) {
		LOG_DBG("Failed to connect HARC (err %d)", err);
		return;
	}

	LOG_DBG("HARC %p connected", harc);
	SET_FLAG(g_harc_connected);
}

static void harc_disconnected_cb(struct bt_hap_harc *harc)
{

}

static struct bt_hap_harc_cb harc_cb = {
	.connected = harc_connected_cb,
	.disconnected = harc_disconnected_cb,
};

static void preset_active_cb(struct bt_hap_harc *harc, uint8_t index)
{
	LOG_DBG("%p", harc);
}

static void preset_store_cb(struct bt_hap_harc *harc, const struct bt_has_preset_record *record)
{
	LOG_DBG("%p", harc);
}

static void preset_remove_cb(struct bt_hap_harc *harc, uint8_t start_index, uint8_t end_index)
{
	LOG_DBG("%p", harc);
}

static void preset_available_cb(struct bt_hap_harc *harc, uint8_t index, bool available)
{
	LOG_DBG("%p", harc);
}

static void preset_commit_cb(struct bt_hap_harc *harc)
{
	LOG_DBG("%p", harc);
}

static int preset_get_cb(struct bt_hap_harc *harc, uint8_t index,
			 struct bt_has_preset_record *record)
{
	if (index == test_preset_index_1) {
		record->index = test_preset_index_1;
		record->properties = test_preset_properties;
		record->name = test_preset_name_1;
	} else if (index == test_preset_index_3) {
		record->index = test_preset_index_3;
		record->properties = test_preset_properties;
		record->name = test_preset_name_3;
	} else if (index == test_preset_index_5) {
		record->index = test_preset_index_5;
		record->properties = test_preset_properties;
		record->name = test_preset_name_5;
	} else {
		return -ENOENT;
	}

	return 0;
}

static struct bt_hap_harc_preset_cb preset_cb = {
	.active = preset_active_cb,
	.store = preset_store_cb,
	.remove = preset_remove_cb,
	.available = preset_available_cb,
	.commit = preset_commit_cb,
	.get = preset_get_cb,
};

static void expect_harc_connected(void)
{
	WAIT_FOR_FLAG(g_harc_connected);
	UNSET_FLAG(g_harc_connected);
}

static void expect_harc_complete(void)
{
	WAIT_FOR_FLAG(g_harc_proc_complete);
	UNSET_FLAG(g_harc_proc_complete);
}

static void proc_complete_cb(int err, void *params)
{
	LOG_DBG("err %d params %p", err, params);

	__ASSERT_NO_MSG(err == 0);
	SET_FLAG(g_harc_proc_complete);
}

static void proc_status_cb(struct bt_hap_harc *harc, int err, void *params)
{
	LOG_DBG("harc %p err %d params %p", harc, err, params);

	__ASSERT_NO_MSG(err == 0);
}

static void test_binaural(void)
{
	struct bt_hap_harc_preset_write_params preset_write_params;
	struct bt_hap_harc_preset_read_params preset_read_params;
	struct bt_hap_harc_preset_set_params preset_set_params;
	struct bt_hap_harc_info harc_info = { 0 };
	struct bt_hap_harc *harc[2] = { NULL };
	struct bt_conn *conn[2] = { NULL };
	bt_addr_le_t adva;
	int err;

	__ASSERT_NO_MSG(get_device_nbr() == 0);

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(err == 0);

	err = bt_hap_harc_preset_cb_register(&preset_cb);
	if (err < 0) {
		FAIL("Failed to register preset callbacks (err %d)\n", err);
		return;
	}

	err = bt_hap_harc_cb_register(&harc_cb);
	if (err < 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
		return;
	}

	err = bt_testlib_scan_find_name(&adva, "ha1");
	__ASSERT_NO_MSG(err == 0);

	err = bt_testlib_connect(&adva, &conn[0]);
	__ASSERT_NO_MSG(err == 0);

	err = bt_testlib_scan_find_name(&adva, "ha2");
	__ASSERT_NO_MSG(err == 0);

	err = bt_testlib_connect(&adva, &conn[1]);
	__ASSERT_NO_MSG(err == 0);

	err = bt_hap_harc_bind(conn[0], &harc[0]);
	if (err < 0) {
		FAIL("Failed to connect HARC (err %d)\n", err);
		return;
	}

	expect_harc_connected();

	err = bt_hap_harc_bind(conn[1], &harc[1]);
	if (err < 0) {
		FAIL("Failed to connect HARC (err %d)\n", err);
		return;
	}

	expect_harc_connected();

	err = bt_hap_harc_info_get(harc[0], &harc_info);
	if (err < 0) {
		FAIL("Failed to get HARC info (err %d)\n", err);
		return;
	}

	__ASSERT_NO_MSG(harc_info.type == BT_HAS_HEARING_AID_TYPE_BINAURAL);
	__ASSERT_NO_MSG(harc_info.binaural.pair == harc[1]);

	preset_read_params.complete = proc_complete_cb;
	preset_read_params.start_index = BT_HAS_PRESET_INDEX_FIRST;
	preset_read_params.max_count = 255;

	err = bt_hap_harc_preset_read(harc[0], &preset_read_params);
	if (err < 0) {
		FAIL("Failed to set preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	preset_read_params.complete = proc_complete_cb;
	preset_read_params.start_index = BT_HAS_PRESET_INDEX_FIRST;
	preset_read_params.max_count = 255;

	err = bt_hap_harc_preset_read(harc[1], &preset_read_params);
	if (err < 0) {
		FAIL("Failed to set preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	preset_set_params.complete = proc_complete_cb;
	preset_set_params.status = proc_status_cb;

	err = bt_hap_harc_preset_set(harc, 2, 0x01, &preset_set_params);
	if (err < 0) {
		FAIL("Failed to set preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	err = bt_hap_harc_preset_set_next(harc, 2, &preset_set_params);
	if (err < 0) {
		FAIL("Failed to set next preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	err = bt_hap_harc_preset_set_prev(harc, 2, &preset_set_params);
	if (err < 0) {
		FAIL("Failed to set prev preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	preset_write_params.complete = proc_complete_cb;
	preset_write_params.status = proc_status_cb;
	preset_write_params.index = 0x01;
	preset_write_params.name = "Vacuum";

	err = bt_hap_harc_preset_write(harc, 2, &preset_write_params);
	if (err < 0) {
		FAIL("Failed to write preset (err %d)\n", err);
		return;
	}

	expect_harc_complete();

	PASS("%s\n", __func__);
}

static const struct bst_test_instance test_hap_harc[] = {
	{
		.test_id = "hap_harc_test_binaural",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_binaural,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hap_harc_install(struct bst_test_list *tests)
{
	if (IS_ENABLED(CONFIG_BT_HAP_HARC)) {
		return bst_add_tests(tests, test_hap_harc);
	} else {
		return tests;
	}
}
