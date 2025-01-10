/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bstests.h"
#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bsim_id_settings, LOG_LEVEL_DBG);

extern void run_dut1(void);
extern void run_dut2(void);

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "dut1",
		.test_descr = "DUT 1",
		.test_main_f = run_dut1,
	},
	{
		.test_id = "dut2",
		.test_descr = "DUT 2",
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
