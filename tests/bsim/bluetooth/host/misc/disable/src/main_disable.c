/* main_disable.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME main_disable
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

#include "common.h"

extern enum bst_result_t bst_result;

#define NUM_ITERATIONS 35

static void test_disable_main(void)
{
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		int err;

		err = bt_enable(NULL);
		if (err != 0) {
			FAIL("Enable failed (err %d)\n", err);
		}

		err = bt_disable();
		if (err) {
			FAIL("Enable failed (err %d)\n", err);
		}
	}

	PASS("Disable test passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "disable",
		.test_descr = "disable_test",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_disable_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_disable_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
