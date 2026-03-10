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

#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_privacy, LOG_LEVEL_INF);

extern void central_test_args_parse(int argc, char *argv[]);
extern void peripheral_test_args_parse(int argc, char *argv[]);

extern void test_central_main(void);
extern void test_peripheral(void);

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central device",
		.test_main_f = test_central_main,
		.test_args_f = central_test_args_parse,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral device",
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
