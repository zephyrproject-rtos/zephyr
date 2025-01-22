/* main_collision.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babblekit/testcase.h"
#include "babblekit/sync.h"
#include "common.h"

static void test_peripheral_main(void)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	peripheral_setup_and_connect();

	/*
	 * we need to sync with peer to ensure that we get collisions
	 */
	bk_sync_send();
	bk_sync_wait();

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		TEST_FAIL("Sending credit based connection request failed (err %d)", err);
	}

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
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	central_setup_and_connect();

	bk_sync_wait();
	bk_sync_send();

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		TEST_FAIL("Sending credit based connection request failed (err %d)", err);
	}

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	wait_for_disconnect();

	TEST_PASS("EATT Central tests Passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral Collision",
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central",
		.test_descr = "Central Collision",
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_collision_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
