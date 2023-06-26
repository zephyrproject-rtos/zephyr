/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"
#include <string.h>

#include <zephyr/logging/log.h>

#include "common.h"

#define WAIT_TIME_S 60
#define WAIT_TIME   (WAIT_TIME_S * 1e6)

LOG_MODULE_REGISTER(bsim_bt_settings_cleanup, LOG_LEVEL_DBG);

extern void run_tester(void);
extern void run_dut(void);

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test failed (not passed after %d seconds)\n", WAIT_TIME_S);
	}
}

static void test_settings_cleanup_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "tester",
		.test_descr = "Tester",
		.test_post_init_f = test_settings_cleanup_init,
		.test_tick_f = test_tick,
		.test_main_f = run_tester,
	},
	{
		.test_id = "dut",
		.test_descr = "DUT",
		.test_post_init_f = test_settings_cleanup_init,
		.test_tick_f = test_tick,
		.test_main_f = run_dut,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_settings_cleanup_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_settings_cleanup_install, NULL};

void main(void)
{
	bst_main();
}
