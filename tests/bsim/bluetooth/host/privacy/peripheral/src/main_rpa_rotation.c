/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

void tester_procedure(void);
void dut_procedure(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = tester_procedure,
	},
	{
		.test_id = "peripheral",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = dut_procedure,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_rpa_rotation_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};
