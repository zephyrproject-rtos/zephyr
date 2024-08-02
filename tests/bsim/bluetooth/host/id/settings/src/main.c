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

LOG_MODULE_REGISTER(bt_bsim_id_settings, LOG_LEVEL_DBG);

extern void run_dut1(void);
extern void run_dut2(void);

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test failed (not passed after %d seconds)\n", WAIT_TIME_S);
	}
}

static void test_id_settings_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "dut1",
		.test_descr = "DUT 1",
		.test_pre_init_f = test_id_settings_init,
		.test_tick_f = test_tick,
		.test_main_f = run_dut1,
	},
	{
		.test_id = "dut2",
		.test_descr = "DUT 2",
		.test_pre_init_f = test_id_settings_init,
		.test_tick_f = test_tick,
		.test_main_f = run_dut2,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_id_settings_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_id_settings_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
