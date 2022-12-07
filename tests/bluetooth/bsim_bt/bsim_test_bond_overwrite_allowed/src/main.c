/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

void central(void);
void peripheral(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

void main(void)
{
	bst_main();
}
