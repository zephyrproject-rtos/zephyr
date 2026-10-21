/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include "babblekit/testcase.h"
#include "bstests.h"

/* Upper bound for the time between two reports of the advertiser, which advertises every
 * 100 to 150 ms
 */
#define REPORT_MAX_GAP K_SECONDS(1)

static K_SEM_DEFINE(report_sem, 0, 1);

/* The only other device is the advertiser */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	ARGS_UNUSED(addr, rssi, adv_type, buf);

	k_sem_give(&report_sem);
}

static void expect_report(const char *when)
{
	TEST_ASSERT(k_sem_take(&report_sem, REPORT_MAX_GAP) == 0, "No advertising report %s", when);
}

static void test_scan_identity_adv(void)
{
	/* A public identity address, so that the legacy advertiser does not use the random
	 * address that the scanner's RPA is set in
	 */
	static const uint8_t public_addr[BT_ADDR_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	const struct bt_le_scan_param scan_param =
		BT_LE_SCAN_PARAM_INIT(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE,
				      BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
	int err;

	bt_ctlr_set_public_addr(public_addr);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_scan_start(&scan_param, scan_cb);
	TEST_ASSERT(err == 0, "Failed to start scanning (err %d)", err);

	/* While it is enabled, the Controller refuses a new random address for the scanner */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);

	expect_report("before the forced rotation");

	err = bt_le_rotate_rpa();
	TEST_ASSERT(err == -EACCES, "Rotating next to the identity advertiser returned %d", err);
	k_sem_reset(&report_sem);
	expect_report("after the forced rotation");

	/* The RPA timeout, restarted by the forced rotation, is not reported to the application */
	(void)k_sleep(K_SECONDS(CONFIG_BT_RPA_TIMEOUT));
	k_sem_reset(&report_sem);
	expect_report("after the RPA timeout");

	TEST_PASS_AND_EXIT("PASS");
}

static void test_advertiser(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);

	/* Advertising goes on until the scanner ends the simulation */
	TEST_PASS("PASS");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "scan_identity_adv",
		.test_descr = "An active scanner keeps scanning when a legacy advertiser on the "
			      "public identity address keeps its RPA from being rotated, and "
			      "bt_le_rotate_rpa() reports it",
		.test_main_f = test_scan_identity_adv,
	},
	{
		.test_id = "advertiser",
		.test_descr = "Non-connectable advertiser for scan_identity_adv",
		.test_main_f = test_advertiser,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_scan_identity_adv_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
