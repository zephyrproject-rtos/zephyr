/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "bstests.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bsim_ccc_update, LOG_LEVEL_DBG);

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

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_main_f = central_main,
	},
	{
		.test_id = "bad_central",
		.test_descr = "Bad Central device",
		.test_main_f = bad_central_main,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
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
