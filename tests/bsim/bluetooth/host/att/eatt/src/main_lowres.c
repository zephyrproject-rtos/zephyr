/* main_lowres.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor
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

	disconnect();

	TEST_PASS("EATT Peripheral tests Passed");
}

static void test_central_main(void)
{
	central_setup_and_connect();

	/* Central (us) will try to open 5 channels.
	 * Peripheral will only accept and open 2.
	 *
	 * The purpose of the test is to verify that the accepted EATT channels
	 * still get opened when the response to the ecred conn req is "Some
	 * connections refused - not enough resources available".
	 */

	wait_for_disconnect();

	TEST_PASS("EATT Central tests Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral_lowres",
		.test_descr = "Peripheral lowres",
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central_lowres",
		.test_descr = "Central lowres",
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_lowres_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
