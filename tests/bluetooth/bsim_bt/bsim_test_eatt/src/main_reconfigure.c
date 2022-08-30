/* main_reconfigure.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/bluetooth/gatt.h>

#define NEW_MTU 100

CREATE_FLAG(flag_reconfigured);

void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU Updated: tx %d, rx %d\n", tx, rx);
	if (rx == NEW_MTU || tx == NEW_MTU) {
		SET_FLAG(flag_reconfigured);
	}
}

static struct bt_gatt_cb cb = {
	.att_mtu_updated = att_mtu_updated,
};

static void test_peripheral_main(void)
{
	peripheral_setup_and_connect();

	bt_gatt_cb_register(&cb);

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	WAIT_FOR_FLAG(flag_reconfigured);

	disconnect();

	PASS("EATT Peripheral tests Passed\n");
}

static void test_central_main(void)
{
	int err;

	central_setup_and_connect();

	bt_gatt_cb_register(&cb);

	while (bt_eatt_count(default_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	err = bt_eatt_reconfigure(default_conn, NEW_MTU);
	if (err < 0) {
		FAIL("Reconfigure failed (%d)\n", err);
	}

	WAIT_FOR_FLAG(flag_reconfigured);

	wait_for_disconnect();

	PASS("EATT Central tests Passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral_reconfigure",
		.test_descr = "Peripheral reconfigure",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central_reconfigure",
		.test_descr = "Central reconfigure",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_reconfigure_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
