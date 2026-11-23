/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v1/src/kernel_apis.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_sched.h>

#define WAIT_TIME_US 1000000

/* Specify accepted tolerance. On Realtek Bee family SoCs, the busy wait loop
 * and the system timer are based on different mechanisms and may not align
 * perfectly. 1 percent base intolerance is to cover CPU processing in the
 * test.
 */
#define TOLERANCE_PPC 1

/**
 * @brief Test os_sys_time_get
 *
 * @see os_sys_time_get_zephyr()
 */
ZTEST(osif_sched, test_os_sys_time_get)
{
	uint64_t start_time, stop_time, diff, max, min;

	start_time = os_sys_time_get();
	k_busy_wait(WAIT_TIME_US);
	stop_time = os_sys_time_get();

	diff = (uint64_t)(stop_time - start_time) * USEC_PER_MSEC;

	max = WAIT_TIME_US + (TOLERANCE_PPC * WAIT_TIME_US / 100);
	min = WAIT_TIME_US - (TOLERANCE_PPC * WAIT_TIME_US / 100);

	zassert_true(diff <= max && diff >= min, "start %lld stop %lld (diff %lld) wait %d us\n",
		     start_time, stop_time, diff, WAIT_TIME_US);
}
ZTEST_SUITE(osif_sched, NULL, NULL, NULL, NULL, NULL);
