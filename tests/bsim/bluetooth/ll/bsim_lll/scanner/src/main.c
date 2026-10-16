/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

/* Enough reports of the advertising without a scan response to tell that
 * the scan requests sent after them were ignored.
 */
#define REJECT_ADV_COUNT 10U

DEFINE_FLAG_STATIC(flag_step_done);

static atomic_t step;
static uint8_t adv_count;
static bool rsp_seen;

struct test_data {
	uint8_t kind;
	uint8_t step;
};

static bool test_data_parse(struct bt_data *data, void *user_data)
{
	struct test_data *td = user_data;

	if ((data->type == BT_DATA_MANUFACTURER_DATA) && (data->data_len == TEST_DATA_LEN) &&
	    (sys_get_le16(data->data) == TEST_COMPANY_ID)) {
		td->kind = data->data[2];
		td->step = data->data[3];

		return false;
	}

	return true;
}

static bool is_step_done(enum test_step s)
{
	switch (s) {
	case STEP_NONCONN:
		return adv_count > 0U;
	case STEP_FAL_REJECT:
		return adv_count >= REJECT_ADV_COUNT;
	default:
		return (adv_count > 0U) && rsp_seen;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	enum test_step s = atomic_get(&step);
	struct test_data td = { 0U, 0U };

	bt_data_parse(ad, test_data_parse, &td);

	/* Another device, or a step that is already done */
	if ((td.kind == 0U) || (td.step != s)) {
		return;
	}

	switch (type) {
	case BT_GAP_ADV_TYPE_ADV_NONCONN_IND:
		TEST_ASSERT(s == STEP_NONCONN, "Unexpected ADV_NONCONN_IND in step %u", s);
		TEST_ASSERT(td.kind == TEST_DATA_ADV, "Scan response data in ADV_NONCONN_IND");
		adv_count++;
		break;
	case BT_GAP_ADV_TYPE_ADV_SCAN_IND:
		TEST_ASSERT(s != STEP_NONCONN, "Unexpected ADV_SCAN_IND in step %u", s);
		TEST_ASSERT(td.kind == TEST_DATA_ADV, "Scan response data in ADV_SCAN_IND");
		adv_count++;
		break;
	case BT_GAP_ADV_TYPE_SCAN_RSP:
		TEST_ASSERT(s != STEP_FAL_REJECT,
			    "SCAN_RSP to a scanner not on the Filter Accept List");
		TEST_ASSERT(s != STEP_NONCONN, "Unexpected SCAN_RSP in step %u", s);
		TEST_ASSERT(td.kind == TEST_DATA_SCAN_RSP, "Advertising data in SCAN_RSP");
		rsp_seen = true;
		break;
	default:
		TEST_FAIL("Unexpected report type %u in step %u", type, s);
		break;
	}

	if (is_step_done(s)) {
		SET_FLAG(flag_step_done);
	}
}

static void test_scanner(void)
{
	const struct bt_le_scan_param *param =
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
				 BT_GAP_SCAN_FAST_INTERVAL_MIN, BT_GAP_SCAN_FAST_WINDOW);
	bt_addr_le_t addr;
	int err;

	TEST_START("scanner");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Backchannel init failed (err %d)", err);

	err = bt_addr_le_from_str("R:" SCANNER_ADDR, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_scan_start(param, device_found);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	for (enum test_step s = STEP_NONCONN; s < STEP_COUNT; s++) {
		WAIT_FOR_FLAG(flag_step_done);
		TEST_PRINT("Step %u done", s);

		/* The reports of the step that is done are ignored from now on,
		 * and the advertiser only moves on once it gets the sync.
		 */
		atomic_set(&step, s + 1);
		adv_count = 0U;
		rsp_seen = false;
		UNSET_FLAG(flag_step_done);

		bk_sync_send();
	}

	/* The advertiser ends the simulation once it gets the last sync */
	TEST_PASS("scanner");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "scanner",
		.test_descr = "Checks each step of the advertiser on the Nordic LLL",
		.test_main_f = test_scanner,
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
