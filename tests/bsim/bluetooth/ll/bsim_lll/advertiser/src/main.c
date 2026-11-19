/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
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

/* Scan requests received since the advertising of the current step started. The scanner is the
 * only device that sends any.
 */
static atomic_t scan_req_count;

static bool vs_evt_cb(struct net_buf_simple *buf)
{
	const struct bt_hci_evt_vs *vs = (const void *)buf->data;

	if ((buf->len < sizeof(*vs)) || (vs->subevent != BT_HCI_EVT_VS_SCAN_REQ_RX)) {
		return false;
	}

	(void)atomic_inc(&scan_req_count);

	return true;
}

/* A legacy advertiser only reports the scan requests it receives with a vendor specific event */
static void scan_req_reports_enable(void)
{
	struct bt_hci_cp_vs_set_scan_req_reports *cp;
	struct net_buf *buf;
	int err;

	err = bt_hci_register_vnd_evt_cb(vs_evt_cb);
	TEST_ASSERT(err == 0, "Vendor event callback register failed (err %d)", err);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No HCI command buffer");

	cp = net_buf_add(buf, sizeof(*cp));
	cp->enable = BT_HCI_VS_SCAN_REQ_REPORTS_ENABLED;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_SET_SCAN_REQ_REPORTS, buf, NULL);
	TEST_ASSERT(err == 0, "Scan request reports enable failed (err %d)", err);
}

/* Tells the scanner that the step is on air and waits until the scanner is done with it, so that
 * the scanner never scans for a step while the previous one is still advertised.
 */
static void step_run(enum test_step step)
{
	/* The scanner starts the first step by itself */
	if (step != STEP_NONCONN) {
		bk_sync_send();
	}

	bk_sync_wait();
}

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
	(void)atomic_clear(&scan_req_count);

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
	bt_addr_le_t addr;
	int err;

	TEST_START("advertiser");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Backchannel init failed (err %d)", err);

	err = bt_addr_le_from_str("R:" ADVERTISER_ADDR, &addr);
	TEST_ASSERT(err == 0, "Invalid address (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	scan_req_reports_enable();

	adv_start(STEP_NONCONN, BT_LE_ADV_OPT_NONE);
	step_run(STEP_NONCONN);
	adv_stop();

	adv_start(STEP_SCAN, BT_LE_ADV_OPT_SCANNABLE);
	step_run(STEP_SCAN);

	data_set(STEP_SCAN_UPDATE);
	err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Advertising data update failed (err %d)", err);
	step_run(STEP_SCAN_UPDATE);
	adv_stop();

	fal_set("R:" SCANNER_ADDR);
	adv_start(STEP_FAL_ACCEPT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_FILTER_SCAN_REQ);
	step_run(STEP_FAL_ACCEPT);
	adv_stop();

	/* The same address as the scanner's but public, so that the address
	 * type has to be matched too.
	 */
	fal_set("P:" SCANNER_ADDR);
	adv_start(STEP_FAL_REJECT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_FILTER_SCAN_REQ);
	step_run(STEP_FAL_REJECT);
	adv_stop();

	adv_start(STEP_SCAN_FAL_REJECT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY);
	step_run(STEP_SCAN_FAL_REJECT);
	adv_stop();
	TEST_ASSERT(atomic_get(&scan_req_count) == 0,
		    "Scan request from a scanner that filters the advertiser out");

	adv_start(STEP_SCAN_FAL_ACCEPT, BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY);
	step_run(STEP_SCAN_FAL_ACCEPT);
	adv_stop();
	TEST_ASSERT(atomic_get(&scan_req_count) > 0,
		    "No scan request from a scanner that has the advertiser on its list");

	TEST_PASS_AND_EXIT("advertiser");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "advertiser",
		.test_descr = "Advertises step by step",
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
