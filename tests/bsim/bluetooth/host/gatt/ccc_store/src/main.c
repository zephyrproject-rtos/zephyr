/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bstests.h"
#include <stdlib.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bsim_ccc_store, LOG_LEVEL_DBG);

static int n_times;

extern void run_peripheral(int times);
extern void run_central(int times);

static void central_main(void)
{
	run_central(n_times);
}

static void peripheral_main(void)
{
	run_peripheral(n_times);
}

static void test_args(int argc, char **argv)
{
	__ASSERT(argc == 1, "Please specify only 1 test argument\n");

	n_times = strtol(argv[0], NULL, 10);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_main_f = central_main,
		.test_args_f = test_args,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
		.test_main_f = peripheral_main,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER};

struct bst_test_list *test_ccc_store_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_ccc_store_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
