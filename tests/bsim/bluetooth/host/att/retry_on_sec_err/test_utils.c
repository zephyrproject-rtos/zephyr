/* Copyright (c) 2023 Codecoup
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_utils.h"

void test_init(void)
{
	bst_result = In_progress;
	bst_ticker_set_next_tick_absolute(SIMULATED_TEST_TIMEOUT);
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result == In_progress) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended. Consider increasing "
			       "simulation length.\n");
	}
}
