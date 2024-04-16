/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"
#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bsim_ccc_update, LOG_LEVEL_DBG);

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

extern enum bst_result_t bst_result;

#define WAIT_TIME_S 60
#define WAIT_TIME   (WAIT_TIME_S * 1e6)

extern void run_peripheral(void);
extern void run_central(void);
extern void run_bad_central(void);

static void central_main(void)
{
	run_central();
}

static void bad_central_main(void)
{
	run_bad_central();
}

static void peripheral_main(void)
{
	run_peripheral();
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error_time_line("Test failed (not passed after %d seconds)\n",
					 WAIT_TIME_S);
	}
}

static void test_ccc_update_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_post_init_f = test_ccc_update_init,
		.test_tick_f = test_tick,
		.test_main_f = central_main,
	},
	{
		.test_id = "bad_central",
		.test_descr = "Bad Central device",
		.test_post_init_f = test_ccc_update_init,
		.test_tick_f = test_tick,
		.test_main_f = bad_central_main,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
		.test_post_init_f = test_ccc_update_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral_main,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_ccc_update_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_ccc_update_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
