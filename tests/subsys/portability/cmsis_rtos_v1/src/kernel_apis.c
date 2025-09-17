/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os.h>

#define WAIT_TIME_US 1000000

/* Specify accepted tolerance. On some Zephyr platforms  (e.g. nRF5x) the busy
 * wait loop and the system timer are based on different mechanisms and may not
 * align perfectly. 1 percent base intolerance is to cover CPU processing in the
 * test.
 */
#if CONFIG_NRF_RTC_TIMER
/* High frequency clock used for k_busy_wait may have up to 8% tolerance.
 * Additionally, if RC is used for low frequency clock then it has 5% tolerance.
 */
#define TOLERANCE_PPC \
	(1 + 8 + (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC) ? 5 : 0))
#else
#define TOLERANCE_PPC 1
#endif

/**
 * @brief Test kernel start
 *
 * @see osKernelInitialize(), osKernelStart(),
 * osKernelRunning()
 */
ZTEST(kernel_apis, test_kernel_start)
{
	if (osFeature_MainThread) {
		/* When osFeature_MainThread is 1 the kernel offers to start
		 * with 'main'. The kernel is in this case already started.
		 */
		zassert_true(!osKernelInitialize() && !osKernelStart()
			     && osKernelRunning());
	} else {
		/* When osFeature_MainThread is 0 the kernel requires
		 * explicit start with osKernelStart.
		 */
		zassert_false(osKernelRunning());
	}
}

/**
 * @brief Test kernel system timer
 *
 * @see osKernelSysTick()
 */
ZTEST(kernel_apis, test_kernel_systick)
{
	uint32_t start_time, stop_time, diff, max, min;

	start_time = osKernelSysTick();
	k_busy_wait(WAIT_TIME_US);
	stop_time = osKernelSysTick();

	diff = (uint32_t)k_cyc_to_ns_floor64(stop_time -
					 start_time) / NSEC_PER_USEC;

	max = WAIT_TIME_US + (TOLERANCE_PPC * WAIT_TIME_US / 100);
	min = WAIT_TIME_US - (TOLERANCE_PPC * WAIT_TIME_US / 100);

	zassert_true(diff <= max && diff >= min,
		     "start %d stop %d (diff %d) wait %d\n",
		     start_time, stop_time, diff, WAIT_TIME_US);
}
ZTEST_SUITE(kernel_apis, NULL, NULL, NULL, NULL, NULL);
