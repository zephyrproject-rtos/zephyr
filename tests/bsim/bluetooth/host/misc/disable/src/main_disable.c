/* main_disable.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#define LOG_MODULE_NAME main_disable
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"

extern enum bst_result_t bst_result;

#define NUM_ITERATIONS 35

static void test_disable_main(void)
{
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		int err;

		err = bt_enable(NULL);
		if (err != 0) {
			TEST_FAIL("Enable failed (err %d)", err);
		}

		err = bt_disable();
		if (err) {
			TEST_FAIL("Enable failed (err %d)", err);
		}
	}

	TEST_PASS("Disable test passed");
}

static void test_disable_set_default_id(void)
{
	/* FIXME: Temporary workaround to get around a bug in the controller.
	 * The controller gets stuck in the POWER_CLOCK ISR without this.
	 * See open PR: https://github.com/zephyrproject-rtos/zephyr/pull/73342
	 * for more details.
	 */
	k_sleep(K_MSEC(1));

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		int err;
		size_t id_count;
		bt_addr_le_t stored_id;
		bt_addr_le_t addr = {.type = BT_ADDR_LE_RANDOM,
				     .a = {.val = {i, 2, 3, 4, 5, 0xc0}}};

		err = bt_id_create(&addr, NULL);
		if (err != 0) {
			TEST_FAIL("Creating ID failed (err %d)", err);
		}

		err = bt_enable(NULL);
		if (err != 0) {
			TEST_FAIL("Enable failed (err %d)", err);
		}

		bt_id_get(NULL, &id_count);
		if (id_count != 1) {
			TEST_FAIL("Expected only one ID, but got: %u", id_count);
		}

		id_count = 1;
		bt_id_get(&stored_id, &id_count);
		if (id_count != 1) {
			TEST_FAIL("Expected only one ID, but got: %u", id_count);
		}

		if (!bt_addr_le_eq(&stored_id, &addr)) {
			uint8_t addr_set_str[BT_ADDR_LE_STR_LEN];
			uint8_t addr_stored_str[BT_ADDR_LE_STR_LEN];

			bt_addr_le_to_str(&addr, addr_set_str, BT_ADDR_LE_STR_LEN);
			bt_addr_le_to_str(&stored_id, addr_stored_str, BT_ADDR_LE_STR_LEN);
			TEST_FAIL("Expected stored ID to be equal to set ID: %s, %s", addr_set_str,
				  addr_stored_str);
		}

		err = bt_disable();
		if (err) {
			TEST_FAIL("Enable failed (err %d)", err);
		}
	}

	TEST_PASS("Disable set default ID test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "disable",
		.test_descr = "disable_test",
		.test_main_f = test_disable_main
	},
	{
		.test_id = "disable_set_default_id",
		.test_descr = "disable_test where each iteration sets the default ID",
		.test_main_f = test_disable_set_default_id
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_disable_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
