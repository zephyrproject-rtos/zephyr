/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "argparse.h"

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * 1000000)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(70)

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;

		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

void print_address(bt_addr_le_t *addr)
{
	char array[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, array, sizeof(array));
	printk("Address : %s\n", array);
}
