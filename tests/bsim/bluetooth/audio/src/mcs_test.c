/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/printk.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_MCS
extern enum bst_result_t bst_result;

static void start_adv(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void test_main(void)
{
	int err;

	printk("Media Control Server test application.  Board: %s\n", CONFIG_BOARD);

	/* Initialize media player */
	err = media_proxy_pl_init();
	if (err) {
		FAIL("Initializing MPL failed (err %d)", err);
		return;
	}

	/* Initialize Bluetooth, get connected */
	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	PASS("MCS passed\n");

	while (1) {
		start_adv();
		WAIT_FOR_FLAG(flag_connected);
		WAIT_FOR_UNSET_FLAG(flag_connected);
	}
}

static const struct bst_test_instance test_mcs[] = {
	{
		.test_id = "mcs",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mcs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mcs);
}

#else

struct bst_test_list *test_mcs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MCS */
