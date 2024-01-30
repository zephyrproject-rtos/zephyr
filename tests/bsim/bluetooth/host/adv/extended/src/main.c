/**
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "bstests.h"
#include "common.h"

extern enum bst_result_t bst_result;

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after %i seconds)\n", WAIT_SECONDS);
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

extern struct bst_test_list *test_ext_adv_advertiser(struct bst_test_list *tests);
extern struct bst_test_list *test_ext_adv_scanner(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_ext_adv_advertiser,
	test_ext_adv_scanner,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
