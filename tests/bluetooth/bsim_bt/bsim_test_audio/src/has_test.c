/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_HAS
#include <bluetooth/gatt.h>
#include <bluetooth/audio/has.h>

#include "common.h"

extern enum bst_result_t bst_result;

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	PASS("HAS passed\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_has);
}
#else
struct bst_test_list *test_has_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_HAS */
