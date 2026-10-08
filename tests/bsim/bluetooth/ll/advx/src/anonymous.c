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
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "babblekit/testcase.h"

/* The static random identity address of the scanner, C0:00:00:00:00:02. The
 * directed advertising in test_data/anonymous.Tx.csv has it as the TargetA.
 */
static const bt_addr_le_t scanner_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x02, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* A device that is not in the simulation, C0:00:00:00:00:03. The directed
 * advertising in test_data/anonymous_other.Tx.csv has it as the TargetA.
 */
static const bt_addr_le_t other_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x03, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* The AD data of the anonymous advertising in AUX_ADV_INDs, which the
 * playback device of the test scripts sends from the files in test_data:
 * undirected, directed with the TargetA in the AUX_ADV_IND, and directed with
 * it in the ADV_EXT_IND
 */
#define AD_UNDIRECTED   0x01U
#define AD_DIRECTED     0x02U
#define AD_DIRECTED_PRI 0x03U

/* The kinds of anonymous advertising reported. The advertising sets of the
 * advertiser device have no ADI and no AD data, so the reports do not tell
 * them apart otherwise.
 */
#define REPORT_UNDIRECTED      BIT(0)
#define REPORT_DIRECTED        BIT(1)
#define REPORT_UNDIRECTED_AD   BIT(2)
#define REPORT_DIRECTED_AD     BIT(3)
#define REPORT_DIRECTED_PRI_AD BIT(4)
#define REPORT_OTHER           BIT(5)

static atomic_t reports;

static void adv_start(const bt_addr_le_t *peer)
{
	struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_ANONYMOUS, BT_GAP_ADV_FAST_INT_MIN_2,
		BT_GAP_ADV_FAST_INT_MAX_2, peer);
	struct bt_le_ext_adv *adv;
	int err;

	if (peer != NULL) {
		param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
	}

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);
}

/* Undirected anonymous advertising, and directed to the given device. The
 * Controller sends no AuxPtr without AD data.
 */
static void anon_adv(const bt_addr_le_t *peer)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	adv_start(NULL);
	adv_start(peer);
}

static void test_anon_adv(void)
{
	TEST_START("anon_adv");

	anon_adv(&scanner_addr);

	/* The scanner checks which of the sets it reports */
	TEST_PASS("anon_adv");
}

static void test_anon_adv_other(void)
{
	TEST_START("anon_adv_other");

	anon_adv(&other_addr);

	/* The scanner checks which of the sets it reports */
	TEST_PASS("anon_adv_other");
}

static bool ad_parse(struct bt_data *data, void *user_data)
{
	uint8_t *ad_value = user_data;

	if ((data->type == BT_DATA_MANUFACTURER_DATA) && (data->data_len == 1U)) {
		*ad_value = data->data[0];

		return false;
	}

	return true;
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	bool is_directed = ((info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0U);
	uint8_t ad_value = 0U;

	/* The Host reports anonymous advertising as from BT_ADDR_LE_ANY */
	if (!bt_addr_le_eq(info->addr, BT_ADDR_LE_ANY)) {
		return;
	}

	bt_data_parse(buf, ad_parse, &ad_value);

	if (ad_value == 0U) {
		atomic_or(&reports, is_directed ? REPORT_DIRECTED : REPORT_UNDIRECTED);
	} else if (!is_directed && (ad_value == AD_UNDIRECTED)) {
		atomic_or(&reports, REPORT_UNDIRECTED_AD);
	} else if (is_directed && (ad_value == AD_DIRECTED)) {
		atomic_or(&reports, REPORT_DIRECTED_AD);
	} else if (is_directed && (ad_value == AD_DIRECTED_PRI)) {
		atomic_or(&reports, REPORT_DIRECTED_PRI_AD);
	} else {
		atomic_or(&reports, REPORT_OTHER);
	}
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

static atomic_val_t scan(uint8_t options)
{
	struct bt_le_scan_param param = BT_LE_SCAN_PARAM_INIT(
		BT_LE_SCAN_TYPE_PASSIVE, options, BT_GAP_SCAN_FAST_INTERVAL,
		BT_GAP_SCAN_FAST_INTERVAL);
	int err;

	atomic_clear(&reports);

	err = bt_le_scan_start(&param, NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	/* Scan for a fixed time, as the kinds that are not reported are part
	 * of what is checked. The playback device of the test scripts sends
	 * for the first 6 seconds of the simulation, which two scans fit in.
	 */
	k_sleep(K_SECONDS(3));

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Scan stop failed (err %d)", err);

	return atomic_get(&reports);
}

static void scanner_init(void)
{
	bt_addr_le_t addr = scanner_addr;
	int err;

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	bt_le_scan_cb_register(&scan_cb);
}

static void test_anon_scan(void)
{
	atomic_val_t reported;

	TEST_START("anon_scan");

	scanner_init();

	reported = scan(BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(reported == (REPORT_UNDIRECTED | REPORT_DIRECTED | REPORT_UNDIRECTED_AD |
				 REPORT_DIRECTED_AD | REPORT_DIRECTED_PRI_AD),
		    "Anonymous advertising reported as 0x%lx", (unsigned long)reported);

	/* The Filter Accept List has no entry for anonymous advertising */
	reported = scan(BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST);
	TEST_ASSERT(reported == 0, "Anonymous advertising reported as 0x%lx with the list",
		    (unsigned long)reported);

	TEST_PASS("anon_scan");
}

static void test_anon_scan_other(void)
{
	atomic_val_t reported;

	TEST_START("anon_scan_other");

	scanner_init();

	reported = scan(BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(reported == (REPORT_UNDIRECTED | REPORT_UNDIRECTED_AD),
		    "Anonymous advertising reported as 0x%lx", (unsigned long)reported);

	TEST_PASS("anon_scan_other");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "anon_adv",
		.test_descr = "Anonymous advertising, undirected and directed to the scanner",
		.test_main_f = test_anon_adv,
	},
	{
		.test_id = "anon_adv_other",
		.test_descr = "Anonymous advertising, undirected and directed to another device",
		.test_main_f = test_anon_adv_other,
	},
	{
		.test_id = "anon_scan",
		.test_descr = "Passive scanner of anonymous advertising to the scanner",
		.test_main_f = test_anon_scan,
	},
	{
		.test_id = "anon_scan_other",
		.test_descr = "Passive scanner of anonymous advertising to another device",
		.test_main_f = test_anon_scan_other,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_anonymous_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
