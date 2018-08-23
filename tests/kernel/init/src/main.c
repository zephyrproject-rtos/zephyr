/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define NSEC_PER_MSEC (u64_t)(NSEC_PER_USEC * USEC_PER_MSEC)
/**
 * @brief Test delay during boot
 * @defgroup kernel_bootdelay_tests Init
 * @ingroup all_tests
 * @{
 */

/**
 * @brief This module verifies the delay specified during boot.
 * @see k_cycle_get_32, #SYS_CLOCK_HW_CYCLES_TO_NS64(X)
 */
void verify_bootdelay(void)
{
	u32_t current_cycles = k_cycle_get_32();

	/* compare this with the boot delay specified */
	zassert_true(SYS_CLOCK_HW_CYCLES_TO_NS64(current_cycles) >=
			(NSEC_PER_MSEC * CONFIG_BOOT_DELAY),
			"boot delay not executed");
}

/**
 * @}
 */

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_bootdelay,
			 ztest_unit_test(verify_bootdelay));
	ztest_run_test_suite(test_bootdelay);
}
