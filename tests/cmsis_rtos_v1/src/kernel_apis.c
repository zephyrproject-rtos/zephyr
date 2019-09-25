/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#define WAIT_TIME_US 1000000

/**
 * @brief Test kernel start
 *
 * @see osKernelInitialize(), osKernelStart(),
 * osKernelRunning()
 */
void test_kernel_start(void)
{
	if (osFeature_MainThread) {
		/* When osFeature_MainThread is 1 the kernel offers to start
		 * with 'main'. The kernel is in this case already started.
		 */
		zassert_true(!osKernelInitialize() && !osKernelStart()
			     && osKernelRunning(), NULL);
	} else {
		/* When osFeature_MainThread is 0 the kernel requires
		 * explicit start with osKernelStart.
		 */
		zassert_false(osKernelRunning(), NULL);
	}
}

/**
 * @brief Test kernel system timer
 *
 * @see osKernelSysTick()
 */
void test_kernel_systick(void)
{
	u32_t start_time, stop_time, diff, max, min;

	start_time = osKernelSysTick();
	k_busy_wait(WAIT_TIME_US);
	stop_time = osKernelSysTick();

	diff = SYS_CLOCK_HW_CYCLES_TO_NS(stop_time -
					 start_time) / NSEC_PER_USEC;

	/* Check that it's within 1%.  On some Zephyr platforms
	 * (e.g. nRF5x) the busy wait loop and the system timer are
	 * based on different mechanisms and may not align perfectly.
	 */
	max = WAIT_TIME_US + (WAIT_TIME_US / 100);
	min = WAIT_TIME_US - (WAIT_TIME_US / 100);

	zassert_true(diff <= max && diff >= min,
		     "start %d stop %d (diff %d) wait %d\n",
		     start_time, stop_time, diff, WAIT_TIME_US);
}
