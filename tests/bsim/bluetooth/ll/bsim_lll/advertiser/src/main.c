/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/util.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

static uint8_t adv_data[TEST_DATA_LEN];
static uint8_t rsp_data[TEST_DATA_LEN];

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_data, sizeof(adv_data)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, rsp_data, sizeof(rsp_data)),
};

static void data_set(enum test_step step)
{
	test_data_init(adv_data, TEST_DATA_ADV, step);
	test_data_init(rsp_data, TEST_DATA_SCAN_RSP, step);
}

static void adv_start(enum test_step step, uint32_t options)
{
	const struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		options, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	int err;

	data_set(step);

	if ((options & BT_LE_ADV_OPT_SCANNABLE) != 0U) {
		err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	} else {
		err = bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), NULL, 0);
	}
	TEST_ASSERT(err == 0, "Advertising start failed for step %u (err %d)", step, err);
}

static void adv_stop(void)
{
	int err;

	err = bt_le_adv_stop();
	TEST_ASSERT(err == 0, "Advertising stop failed (err %d)", err);
}

static void fal_set(const char *str)
{
	bt_addr_le_t addr;
	int err;

	err = bt_addr_le_from_str(str, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_le_filter_accept_list_clear();
	TEST_ASSERT(err == 0, "Filter Accept List clear failed (err %d)", err);

	err = bt_le_filter_accept_list_add(&addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);
}

static void test_advertiser(void)
{
	int err;

	TEST_START("advertiser");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Backchannel init failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	adv_start(STEP_NONCONN, BT_LE_ADV_OPT_NONE);
	bk_sync_wait();
	adv_stop();

	adv_start(STEP_SCAN, BT_LE_ADV_OPT_SCANNABLE);
	bk_sync_wait();

	data_set(STEP_SCAN_UPDATE);
	err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Advertising data update failed (err %d)", err);
	bk_sync_wait();
	adv_stop();

	fal_set("R:" SCANNER_ADDR);
	adv_start(STEP_FAL_ACCEPT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_FILTER_SCAN_REQ);
	bk_sync_wait();
	adv_stop();

	/* The same address as the scanner's but public, so that the address
	 * type has to be matched too.
	 */
	fal_set("P:" SCANNER_ADDR);
	adv_start(STEP_FAL_REJECT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_FILTER_SCAN_REQ);
	bk_sync_wait();
	adv_stop();

	TEST_PASS_AND_EXIT("advertiser");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "advertiser",
		.test_descr = "Advertises on the BabbleSim LLL, step by step",
		.test_main_f = test_advertiser,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *test_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = { test_install, NULL };

int main(void)
{
	bst_main();

	return 0;
}
