/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "bs_bt_utils.h"

extern void central(void);
extern void peripheral(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_main_f = peripheral,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
