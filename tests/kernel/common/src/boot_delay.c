/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>


/**
 * @defgroup kernel_init_tests Kernel Initialization
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_init_tests
 * @{
 */

/**
 * @brief Verify that the configured boot delay elapses before the kernel runs.
 *
 * @ingroup kernel_init_tests
 *
 * @details
 * Passing proves that the kernel honors CONFIG_BOOT_DELAY by busy-waiting the
 * configured number of milliseconds during early boot, so that by the time the
 * test runs at least that much time has elapsed on the hardware cycle counter.
 *
 * Test steps:
 * - Skip the test on platforms with GHz-scale counters that can roll over a
 *   32-bit cycle count during firmware startup.
 * - Sample the current cycle count via k_cycle_get_32().
 * - Convert the elapsed cycles to nanoseconds and compare against
 *   CONFIG_BOOT_DELAY expressed in nanoseconds.
 *
 * Expected result:
 * - The elapsed time is greater than or equal to the configured boot delay.
 *
 * @see k_cycle_get_32()
 */
ZTEST(boot_delay, test_bootdelay)
{
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 1000000000) {
		/* Systems with very fast counters (like the x86 TSC)
		 * and long firmware startup (often 10+ seconds on a
		 * EFI PC!)  can easily roll this over during startup,
		 * and there's no way to detect that case with a 32
		 * bit OS API.  Just skip it if we have a GHz-scale
		 * counter.
		 */
		ztest_test_skip();
	}

	uint32_t current_cycles = k_cycle_get_32();

	/* compare this with the boot delay specified */
	zassert_true(k_cyc_to_ns_floor64(current_cycles) >=
			(NSEC_PER_MSEC * CONFIG_BOOT_DELAY),
			"boot delay not executed: %d < %d",
			(uint32_t)k_cyc_to_ns_floor64(current_cycles),
			(NSEC_PER_MSEC * CONFIG_BOOT_DELAY));
}

/**
 * @}
 */

extern void *common_setup(void);
ZTEST_SUITE(boot_delay, NULL, common_setup, NULL, NULL, NULL);
