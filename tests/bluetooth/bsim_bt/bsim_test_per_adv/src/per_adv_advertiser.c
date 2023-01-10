/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "common.h"

extern enum bst_result_t bst_result;

static void main_per_adv_advertiser(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed: %d\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	/* Create a non-connectable non-scannable advertising set */
	printk("Creating extended advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set: %d\n", err);
		return;
	}
	printk("done.\n");

	/* Set periodic advertising parameters */
	printk("Setting periodic advertising parameters...");
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return;
	}
	printk("done.\n");

	/* Enable Periodic Advertising */
	printk("Starting periodic advertising...");
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to start periodic advertising: %d\n", err);
		return;
	}
	printk("done.\n");

	printk("Starting Extended Advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return;
	}
	printk("done.\n");

	/* Advertise for a bit */
	k_sleep(K_SECONDS(10));

	printk("Stopping Extended Advertising...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n",
		       err);
		return;
	}
	printk("done.\n");

	printk("Stopping Periodic Advertising...");
	err = bt_le_per_adv_stop(adv);
	if (err) {
		printk("Failed to stop periodic advertising: %d\n",
		       err);
		return;
	}
	printk("done.\n");

	printk("Delete extended advertising set...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed Delete extended advertising set: %d\n", err);
		return;
	}
	adv = NULL;
	printk("done.\n");

	PASS("Periodic advertiser passed\n");
}

static const struct bst_test_instance per_adv_advertiser[] = {
	{
		.test_id = "per_adv_advertiser",
		.test_descr = "Basic periodic advertising test. "
			      "Will just start periodic advertising.",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = main_per_adv_advertiser
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_per_adv_advertiser(struct bst_test_list *tests)
{
	return bst_add_tests(tests, per_adv_advertiser);
}
