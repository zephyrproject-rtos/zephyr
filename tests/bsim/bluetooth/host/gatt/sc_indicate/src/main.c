/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "bs_bt_utils.h"

#define BS_SECONDS_TO_US(dur_sec)    ((bs_time_t)dur_sec * USEC_PER_SEC)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS_TO_US(60)

extern void central(void);
extern void peripheral(void);

static void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
