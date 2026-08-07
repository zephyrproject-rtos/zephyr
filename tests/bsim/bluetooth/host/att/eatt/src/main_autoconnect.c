/* main_connect.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babblekit/testcase.h"
#include "common.h"

static void test_peripheral_main(void)
{
	peripheral_setup_and_connect();

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	/* Do not disconnect until the central also has connected all channels */
	k_sleep(K_MSEC(1000));

	disconnect();

	TEST_PASS("EATT Peripheral tests Passed");
}

static void test_central_main(void)
{
	central_setup_and_connect();

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	wait_for_disconnect();

	TEST_PASS("EATT Central tests Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral_autoconnect",
		.test_descr = "Peripheral autoconnect",
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central_autoconnect",
		.test_descr = "Central autoconnect",
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_autoconnect_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
