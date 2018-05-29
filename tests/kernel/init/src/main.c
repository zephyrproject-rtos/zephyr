/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief test delay during boot
 *
 * This module verifies the delay specified during boot.
 */

#include <ztest.h>

void verify_bootdelay(void)
{
	u32_t current_cycles = k_cycle_get_32();

	/* compare this with the boot delay specified */
	zassert_true(SYS_CLOCK_HW_CYCLES_TO_NS64(current_cycles) >=
		     (CONFIG_BOOT_DELAY * NSEC_PER_USEC * USEC_PER_MSEC),
		     "boot delay not executed");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_bootdelay,
			 ztest_unit_test(verify_bootdelay));
	ztest_run_test_suite(test_bootdelay);
}
