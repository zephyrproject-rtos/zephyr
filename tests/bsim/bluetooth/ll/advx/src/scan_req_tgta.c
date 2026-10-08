/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* The static random identity address of the scanner, C0:00:00:00:00:02 */
static const bt_addr_le_t scanner_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x02, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* A resolvable private address that no device of the simulation resolves,
 * 40:00:00:00:00:03. The test script looks for it in the Tx dump of the
 * advertiser.
 */
static const bt_addr_le_t other_rpa = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x03, 0x00, 0x00, 0x00, 0x00, 0x40 } },
};

DEFINE_FLAG_STATIC(flag_scanned);

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "TargetA", 7),
};

static void scanned(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info)
{
	if (bt_addr_le_eq(info->addr, &scanner_addr)) {
		SET_FLAG(flag_scanned);
	}
}

static void test_tgta_adv(void)
{
	static const struct bt_le_ext_adv_cb adv_cb = {
		.scanned = scanned,
	};
	struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ |
			BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, &scanner_addr);
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("tgta_adv");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(&param, &adv_cb, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Scan response data set failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	WAIT_FOR_FLAG(flag_scanned);
	TEST_PRINT("Scan request of the TargetA answered");

	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "Advertising stop failed (err %d)", err);

	err = bt_le_ext_adv_delete(adv);
	TEST_ASSERT(err == 0, "Advertising set delete failed (err %d)", err);

	/* Notifications of scan requests that the stopped set answered can
	 * still be on their way from the Controller. Let them arrive before
	 * the flag is cleared, so that they do not count for the next set.
	 */
	(void)k_sleep(K_MSEC(500));
	UNSET_FLAG(flag_scanned);

	param.peer = &other_rpa;
	err = bt_le_ext_adv_create(&param, &adv_cb, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Scan response data set failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	/* The scanner sends scan requests to this set too, as its filter
	 * policy accepts directed advertising to an RPA that it can not
	 * resolve. Advertise long enough for tens of them, none of which may
	 * be answered.
	 */
	(void)k_sleep(K_SECONDS(5));
	TEST_ASSERT(!IS_FLAG_SET(flag_scanned),
		    "Scan request of another device than the TargetA answered");

	TEST_PASS("tgta_adv");
}

static void test_tgta_scan(void)
{
	struct bt_le_scan_param param = BT_LE_SCAN_PARAM_INIT(
		BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_EXT_FILTER_POLICY,
		BT_GAP_SCAN_FAST_INTERVAL_MIN, BT_GAP_SCAN_FAST_WINDOW);
	bt_addr_le_t addr = scanner_addr;
	int err;

	TEST_START("tgta_scan");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_scan_start(&param, NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	/* The advertiser checks which of the scan requests it answers */
	TEST_PASS("tgta_scan");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "tgta_adv",
		.test_descr = "Directed scannable advertising, to the scanner then to an RPA",
		.test_main_f = test_tgta_adv,
	},
	{
		.test_id = "tgta_scan",
		.test_descr = "Active scanner with the extended filter policy",
		.test_main_f = test_tgta_scan,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_scan_req_tgta_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
