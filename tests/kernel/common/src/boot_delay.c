/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/**
 * @brief Test delay during boot
 * @defgroup kernel_init_tests Init
 * @ingroup all_tests
 * @{
 */

/**
 * @brief This module verifies the delay specified during boot.
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
