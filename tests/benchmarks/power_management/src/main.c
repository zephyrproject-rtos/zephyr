/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
#include <zephyr.h>
#include <tc_util.h>
LOG_MODULE_DECLARE(brd_pm_test);
#include "power_mgmt.h"

#define MAX_CYCLES 5ul

void main(void)
{
	TC_PRINT("Start power management test.\n");

	test_pwr_mgmt_singlethread(MAX_CYCLES);

	test_pwr_mgmt_multithread(MAX_CYCLES);

	TC_PRINT("Power management test finished.\n");

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
