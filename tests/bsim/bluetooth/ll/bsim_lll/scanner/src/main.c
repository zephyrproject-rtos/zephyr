/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
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

/* A scanner that filters the advertiser out gets no report of it, which only
 * shows as time passing without one: at least 13 advertising events, with the
 * 150 ms that the advertising interval of the advertiser is at most.
 */
#define SCAN_REJECT_WAIT K_SECONDS(2)

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

	TEST_ASSERT(s != STEP_SCAN_FAL_REJECT,
		    "Report of an advertiser not on the Filter Accept List");

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

/* Every other step scans with a window that ends before the scan interval,
 * the others with one that fills it.
 */
static void scan_start(enum test_step s)
{
	struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL_MIN,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	if ((s % 2U) != 0U) {
		param.interval = 2U * BT_GAP_SCAN_FAST_WINDOW;
	}

	if (s == STEP_SCAN_FAL_REJECT) {
		/* The same address as the advertiser's but public, so that the
		 * address type has to be matched too.
		 */
		fal_set("P:" ADVERTISER_ADDR);
		param.options = BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST;
	} else if (s == STEP_SCAN_FAL_ACCEPT) {
		fal_set("R:" ADVERTISER_ADDR);
		param.options = BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST;
	} else {
		/* The other steps do not filter the advertisers */
	}

	err = bt_le_scan_start(&param, device_found);
	TEST_ASSERT(err == 0, "Scan start failed for step %u (err %d)", s, err);
}

static void test_scanner(void)
{
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

	/* Advertise too, so that the scanning is preempted by another role */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	for (enum test_step s = STEP_NONCONN; s < STEP_COUNT; s++) {
		/* The advertiser tells when it advertises the step, apart from the first one */
		if (s != STEP_NONCONN) {
			bk_sync_wait();
		}

		scan_start(s);

		if (s == STEP_SCAN_FAL_REJECT) {
			(void)k_sleep(SCAN_REJECT_WAIT);
		} else {
			WAIT_FOR_FLAG(flag_step_done);
		}
		TEST_PRINT("Step %u done", s);

		err = bt_le_scan_stop();
		TEST_ASSERT(err == 0, "Scan stop failed (err %d)", err);

		/* The reports of the step that is done are ignored from now on */
		(void)atomic_set(&step, s + 1);
		adv_count = 0U;
		rsp_seen = false;
		UNSET_FLAG(flag_step_done);

		/* The advertiser moves on once it knows that the scanner has stopped scanning */
		bk_sync_send();
	}

	/* The advertiser ends the simulation once it gets the last sync */
	TEST_PASS("scanner");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "scanner",
		.test_descr = "Checks each step of the advertiser",
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
