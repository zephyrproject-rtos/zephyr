/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

void central(void);
void peripheral(void);
void central_overwrite_oldest(void);
void peripheral_overwrite_oldest(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_main_f = peripheral,
	},
	{
		.test_id = "central_overwrite_oldest",
		.test_main_f = central_overwrite_oldest,
	},
	{
		.test_id = "peripheral_overwrite_oldest",
		.test_main_f = peripheral_overwrite_oldest,
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
