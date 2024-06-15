/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bstests.h"

void central(void);
void peripheral_unpair_in_sec_cb(void);
void peripheral_disconnect_in_sec_cb(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = central,
	},
	{
		.test_id = "peripheral_unpair_in_sec_cb",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral_unpair_in_sec_cb,
	},
	{
		.test_id = "peripheral_disconnect_in_sec_cb",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = peripheral_disconnect_in_sec_cb,
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
