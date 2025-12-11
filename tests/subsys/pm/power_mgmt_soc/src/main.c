/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
LOG_MODULE_DECLARE(brd_pm_test);
#include "power_mgmt.h"

#define MAX_CYCLES 2ul

ZTEST(power_mgmt, test_pm_dummyinit)
{
	test_dummy_init();
}

ZTEST(power_mgmt, test_pm_singlethread)
{
	test_pwr_mgmt_singlethread(MAX_CYCLES);
}

ZTEST(power_mgmt, test_pm_multithread)
{
	test_pwr_mgmt_multithread(MAX_CYCLES);
}

ZTEST_SUITE(power_mgmt, NULL, NULL, NULL, NULL, NULL);
