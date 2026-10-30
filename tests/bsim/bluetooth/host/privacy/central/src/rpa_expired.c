/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "babblekit/testcase.h"
#include "bstests.h"

/* Upper bound for the HCI commands of one RPA rotation */
#define ROTATION_MAX_MS 100U

static K_SEM_DEFINE(rpa_expired_sem, 0, K_SEM_MAX_LIMIT);

/* What rpa_expired_cb() does, and when it was called */
static bool keep_rpa;
static uint16_t next_timeout_s;
static int64_t call_ms[7];
static size_t calls;

static bool rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	int err;

	ARG_UNUSED(adv);

	TEST_ASSERT(calls < ARRAY_SIZE(call_ms), "Unexpected rpa_expired call");
	call_ms[calls++] = k_uptime_get();

	err = bt_le_set_rpa_timeout(next_timeout_s);
	TEST_ASSERT(err == 0, "Failed to set the RPA timeout (err %d)", err);

	k_sem_give(&rpa_expired_sem);

	return !keep_rpa;
}

static void get_rpa(const struct bt_le_ext_adv *adv, bt_addr_le_t *rpa)
{
	struct bt_le_ext_adv_info info;
	int err;

	err = bt_le_ext_adv_get_info(adv, &info);
	TEST_ASSERT(err == 0, "Failed to get the set info (err %d)", err);
	bt_addr_le_copy(rpa, info.addr);
}

/* bt_le_oob_get_local() rotates the RPAs now and calls rpa_expired_cb() from this thread. */
static void force_rotation(void)
{
	struct bt_le_oob oob;
	int err;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	TEST_ASSERT(err == 0, "Failed to rotate the RPAs (err %d)", err);
	TEST_ASSERT(k_sem_take(&rpa_expired_sem, K_NO_WAIT) == 0,
		    "No rpa_expired call for the forced rotation");
}

/* Waits for call n, which is due timeout_s after call n - 1 */
static void expect_call(size_t n, uint16_t timeout_s)
{
	int64_t late_ms;

	TEST_ASSERT(k_sem_take(&rpa_expired_sem, K_SECONDS(timeout_s + 1U)) == 0,
		    "No rpa_expired call %zu", n);
	TEST_ASSERT(calls == n + 1U, "rpa_expired call %zu is call %zu", n, calls - 1U);

	late_ms = call_ms[n] - call_ms[n - 1U] - timeout_s * MSEC_PER_SEC;
	TEST_ASSERT(late_ms >= 0 && late_ms < ROTATION_MAX_MS,
		    "rpa_expired call %zu came %lld ms after its timeout", n, late_ms);
}

static void test_rpa_expired(void)
{
	static const struct bt_le_ext_adv_cb cb = {
		.rpa_expired = rpa_expired_cb,
	};
	struct bt_le_ext_adv *adv;
	bt_addr_le_t kept_rpa;
	bt_addr_le_t rpa;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &cb, &adv);
	TEST_ASSERT(err == 0, "Failed to create the advertising set (err %d)", err);
	get_rpa(adv, &kept_rpa);

	/* A set that is not advertising gets rpa_expired as long as it keeps its RPA, and the
	 * timeout set from the callback is the time to the next call.
	 */
	keep_rpa = true;
	next_timeout_s = 2U;
	force_rotation();
	expect_call(1U, 2U);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);
	get_rpa(adv, &rpa);
	TEST_ASSERT(bt_addr_le_eq(&rpa, &kept_rpa), "The set did not start with its kept RPA");

	/* The same for an advertising set, after a rotation it allowed */
	keep_rpa = false;
	next_timeout_s = 3U;
	force_rotation();
	get_rpa(adv, &rpa);
	TEST_ASSERT(!bt_addr_le_eq(&rpa, &kept_rpa), "The set did not get a new RPA");
	bt_addr_le_copy(&kept_rpa, &rpa);

	keep_rpa = true;
	next_timeout_s = 2U;
	expect_call(3U, 3U);
	expect_call(4U, 2U);
	get_rpa(adv, &rpa);
	TEST_ASSERT(bt_addr_le_eq(&rpa, &kept_rpa), "The set did not keep its RPA");

	/* A stopped set that keeps its RPA starts again with the same one */
	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "Failed to stop advertising (err %d)", err);
	expect_call(5U, 2U);
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);
	get_rpa(adv, &rpa);
	TEST_ASSERT(bt_addr_le_eq(&rpa, &kept_rpa), "The set did not restart with its kept RPA");

	/* A stopped set that lets its RPA go gets a new one when it is started again */
	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "Failed to stop advertising (err %d)", err);
	keep_rpa = false;
	force_rotation();
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start advertising (err %d)", err);
	get_rpa(adv, &rpa);
	TEST_ASSERT(!bt_addr_le_eq(&rpa, &kept_rpa), "The set restarted on its old RPA");

	TEST_PASS_AND_EXIT("PASS");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "rpa_expired",
		.test_descr = "rpa_expired for a stopped and an advertising set: keeping the RPA "
			      "and setting the time to the next call",
		.test_main_f = test_rpa_expired,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_rpa_expired_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
