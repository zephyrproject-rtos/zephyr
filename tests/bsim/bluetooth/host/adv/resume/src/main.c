/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

void dut_procedure(void);
void tester_peripheral_procedure(void);
void tester_central_procedure(void);

void dut_procedure_2(void);
void tester_procedure_2(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "dut",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = dut_procedure,
	},
	{
		.test_id = "tester_peripheral",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = tester_peripheral_procedure,
	},
	{
		.test_id = "tester_central",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = tester_central_procedure,
	},
	{
		.test_id = "dut_2",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = dut_procedure_2,
	},
	{
		.test_id = "tester_2",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = tester_procedure_2,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = { install, NULL };

int main(void)
{
	bst_main();
	return 0;
}
