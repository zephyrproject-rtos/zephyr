/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

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
		TEST_FAIL("Test failed (not passed after %" PRIu64 " us)", WAIT_TIME);
	}
}
