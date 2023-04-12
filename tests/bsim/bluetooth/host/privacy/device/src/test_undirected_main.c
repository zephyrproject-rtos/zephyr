/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_privacy, LOG_LEVEL_INF);

extern enum bst_result_t bst_result;

#define WAIT_TIME_S 20
#define WAIT_TIME   (WAIT_TIME_S * 1e6) /* 20 seconds */

extern void central_test_args_parse(int argc, char *argv[]);
extern void peripheral_test_args_parse(int argc, char *argv[]);

extern void test_central_main(void);
extern void test_peripheral(void);

void sim_timeout(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error_time_line("Test failed (not passed after %d seconds)\n",
					 WAIT_TIME_S);
	}
}

static void test_privacy_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_post_init_f = test_privacy_init,
		.test_tick_f = sim_timeout,
		.test_main_f = test_central_main,
		.test_args_f = central_test_args_parse,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
		.test_post_init_f = test_privacy_init,
		.test_tick_f = sim_timeout,
		.test_main_f = test_peripheral,
		.test_args_f = peripheral_test_args_parse,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_privacy_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_privacy_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
