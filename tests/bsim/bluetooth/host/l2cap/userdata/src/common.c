/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

extern enum bst_result_t bst_result;

void test_init(void)
{
	bst_result = In_progress;
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after %i us)\n", WAIT_TIME);
	}
}
