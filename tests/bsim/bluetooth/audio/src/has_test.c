/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <argparse.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/csip.h>
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
LOG_MODULE_REGISTER(has_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

const uint8_t test_preset_index_1 = 0x01;
const uint8_t test_preset_index_3 = 0x03;
const uint8_t test_preset_index_5 = 0x05;
const char *test_preset_name_1 = "test_preset_name_1";
const char *test_preset_name_3 = "test_preset_name_3";
const char *test_preset_name_5 = "test_preset_name_5";
const enum bt_has_properties test_preset_properties = BT_HAS_PROP_AVAILABLE | BT_HAS_PROP_WRITABLE;

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

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
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
}

static void test_main(void)
{
	test_common();

	PASS("%s\n", __func__);
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

	PASS("%s\n", __func__);
}

static void csip_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csip_set_member_svc_inst *svc_inst,
				 bool locked)
{
	LOG_DBG("Client %p %s the lock", conn, locked ? "locked" : "released");
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn,
				struct bt_csip_set_member_svc_inst *svc_inst)
{
	return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;
}

static struct bt_csip_set_member_cb csip_cbs = {
	.lock_changed = csip_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static struct bt_csip_set_member_register_param csip_set_member_register_param = {
	.set_size = 2,
	.rank = 1,
	.lockable = false,
	/* Using the CSIS test sample SIRK */
	.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
		      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
	.cb = &csip_cbs,
};

static struct bt_has_features_param has_register_param = {
	.type = BT_HAS_HEARING_AID_TYPE_BINAURAL,
	.preset_sync_support = false,
	.independent_presets = false,
};

static void test_binaural(void)
{
	static struct bt_csip_set_member_svc_inst *csip_set_member_svc_inst;
	struct bt_has_preset_register_param preset_param;
	uint8_t rsi[BT_CSIP_RSI_SIZE];
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_CSIP_DATA_RSI(rsi),
	};
	char name[4] = {'h', 'a', 0x30 + get_device_nbr()};
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(err == 0);

	err = bt_set_name(name);
	__ASSERT_NO_MSG(err == 0);

	err = bt_csip_set_member_register(&csip_set_member_register_param,
					  &csip_set_member_svc_inst);
	__ASSERT_NO_MSG(err == 0);

	err = bt_csip_set_member_generate_rsi(csip_set_member_svc_inst, rsi);
	__ASSERT_NO_MSG(err == 0);

	err = bt_has_register(&has_register_param);
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

	err = bt_testlib_adv_conn(NULL, BT_ID_DEFAULT,
				  BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD,
				  ad, ARRAY_SIZE(ad), NULL, 0);
	__ASSERT_NO_MSG(err == 0);

	PASS("%s\n", __func__);
}

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "rank") == 0) {
			csip_set_member_register_param.rank = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "lockable") == 0) {
			csip_set_member_register_param.lockable = true;
		} else if (strcmp(arg, "set_id") == 0) {
			csip_set_member_register_param.set_sirk[0] = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "preset_sync") == 0) {
			has_register_param.preset_sync_support = true;
		} else if (strcmp(arg, "preset_independent") == 0) {
			has_register_param.independent_presets = true;
		}

		else {
			FAIL("Invalid arg: %s", arg);
		}
	}
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
	{
		.test_id = "has_binaural",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_binaural,
		.test_args_f = test_args,
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
