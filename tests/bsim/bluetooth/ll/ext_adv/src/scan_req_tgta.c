/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#define SCANNER_ADDR "R:C0:00:00:00:00:02"

/* A resolvable private address that no device of the simulation resolves */
#define OTHER_RPA "R:40:00:00:00:00:03"

DEFINE_FLAG_STATIC(flag_scanned);

static bt_addr_le_t scanner_addr;

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "TargetA", 7),
};

static void scanned(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info)
{
	if (bt_addr_le_eq(info->addr, &scanner_addr)) {
		SET_FLAG(flag_scanned);
	}
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.scanned = scanned,
};

static void test_tgta_adv(void)
{
	struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ |
			BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, &scanner_addr);
	struct bt_le_ext_adv *adv;
	bt_addr_le_t other_addr;
	int err;

	TEST_START("tgta_adv");

	err = bt_addr_le_from_str(SCANNER_ADDR, &scanner_addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_addr_le_from_str(OTHER_RPA, &other_addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

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

	/* The notifications of the requests answered before the stop come
	 * first.
	 */
	k_sleep(K_MSEC(500));
	UNSET_FLAG(flag_scanned);

	param.peer = &other_addr;
	err = bt_le_ext_adv_create(&param, &adv_cb, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Scan response data set failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	/* The scanner keeps sending scan requests, as its filter policy accepts
	 * directed advertising to an RPA that it can not resolve.
	 */
	k_sleep(K_SECONDS(5));
	TEST_ASSERT(!IS_FLAG_SET(flag_scanned),
		    "Scan request of another device than the TargetA answered");

	TEST_PASS("tgta_adv");
}

static void test_tgta_scan(void)
{
	struct bt_hci_cp_le_set_ext_scan_enable *cp_enable;
	struct bt_hci_cp_le_set_random_address *cp_addr;
	struct bt_hci_cp_le_set_ext_scan_param *cp;
	struct bt_hci_ext_scan_phy *phy;
	struct net_buf *buf;
	bt_addr_le_t addr;
	int err;

	TEST_START("tgta_scan");

	err = bt_addr_le_from_str(SCANNER_ADDR, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	/* The Host has no API for the extended scanner filter policies, so the
	 * scanner is set up with HCI commands, the identity address being the
	 * random address of the scan requests.
	 */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No command buffer");

	cp_addr = net_buf_add(buf, sizeof(*cp_addr));
	bt_addr_copy(&cp_addr->bdaddr, &addr.a);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
	TEST_ASSERT(err == 0, "Random address set failed (err %d)", err);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No command buffer");

	cp = net_buf_add(buf, sizeof(*cp));
	cp->own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
	cp->filter_policy = BT_HCI_LE_SCAN_FP_EXT_NO_FILTER;
	cp->phys = BT_HCI_LE_EXT_SCAN_PHY_1M;

	phy = net_buf_add(buf, sizeof(*phy));
	phy->type = BT_HCI_LE_SCAN_ACTIVE;
	phy->interval = sys_cpu_to_le16(BT_GAP_SCAN_FAST_INTERVAL_MIN);
	phy->window = sys_cpu_to_le16(BT_GAP_SCAN_FAST_WINDOW);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_SCAN_PARAM, buf, NULL);
	TEST_ASSERT(err == 0, "Scan parameters set failed (err %d)", err);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No command buffer");

	cp_enable = net_buf_add(buf, sizeof(*cp_enable));
	cp_enable->enable = BT_HCI_LE_SCAN_ENABLE;
	cp_enable->filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
	cp_enable->duration = 0U;
	cp_enable->period = 0U;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE, buf, NULL);
	TEST_ASSERT(err == 0, "Scan enable failed (err %d)", err);

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
