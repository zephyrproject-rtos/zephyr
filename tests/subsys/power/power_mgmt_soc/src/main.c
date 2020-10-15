/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
LOG_MODULE_DECLARE(brd_pm_test);
#include "power_mgmt.h"

#define MAX_CYCLES 2ul

void test_pm_dummyinit(void)
{
	test_dummy_init();
}

void test_pm_singlethread(void)
{
	test_pwr_mgmt_singlethread(MAX_CYCLES);
}

void test_pm_multithread(void)
{
	test_pwr_mgmt_multithread(MAX_CYCLES);
}

void test_main(void)
{
	ztest_test_suite(test_power_mgmt,
			ztest_unit_test(test_pm_dummyinit),
			ztest_unit_test(test_pm_multithread),
			ztest_unit_test(test_pm_singlethread));

	ztest_run_test_suite(test_power_mgmt);
}
