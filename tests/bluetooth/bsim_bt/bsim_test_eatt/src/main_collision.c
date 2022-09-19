/* main_collision.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

static void test_peripheral_main(void)
{
	int err;

	backchannel_init();

	peripheral_setup_and_connect();

	/*
	 * we need to sync with peer to ensure that we get collisions
	 */
	backchannel_sync_send();
	backchannel_sync_wait();

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	/* Do not disconnect until the central also has connected all channels */
	k_sleep(K_MSEC(1000));

	disconnect();

	PASS("EATT Peripheral tests Passed\n");
}

static void test_central_main(void)
{
	int err;

	backchannel_init();

	central_setup_and_connect();

	backchannel_sync_wait();
	backchannel_sync_send();

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	wait_for_disconnect();

	PASS("EATT Central tests Passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral Collision",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central",
		.test_descr = "Central Collision",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_collision_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
