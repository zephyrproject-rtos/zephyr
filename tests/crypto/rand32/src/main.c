/* test random number generator APIs */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This tests the following random number routines:
 * u32_t z_early_boot_rand32_get(void);
 * u32_t sys_rand32_get(void);
 */


#include <ztest.h>
#include <logging/sys_log.h>
#include <kernel_internal.h>

#define N_VALUES 10


/**
 *
 * @brief Regression test's entry point
 *
 *
 * @return N/A
 */

void test_rand32(void)
{
	u32_t gen, last_gen;
	int rnd_cnt;
	int equal_count = 0;

	/* Test early boot random number generation function */
	last_gen = z_early_boot_rand32_get();
	zassert_true(last_gen != z_early_boot_rand32_get(),
			"z_early_boot_rand32_get failed");

	/*
	 * Test subsequently calls sys_rand32_get(), checking
	 * that two values are not equal.
	 */
	SYS_LOG_DBG("Generating random numbers");
	last_gen = sys_rand32_get();
	/*
	 * Get several subsequent numbers as fast as possible.
	 * Based on review comments in
	 * https://github.com/zephyrproject-rtos/zephyr/pull/5066
	 * If minimum half of the numbers generated were the same
	 * as the previously generated one, then test fails, this
	 * should catch a buggy sys_rand32_get() function.
	 */
	for (rnd_cnt = 0; rnd_cnt < (N_VALUES - 1); rnd_cnt++) {
		gen = sys_rand32_get();
		if (gen == last_gen) {
			equal_count++;
		}
		last_gen = gen;
	}

	if (equal_count > N_VALUES / 2) {
		zassert_false((equal_count > N_VALUES / 2),
		"random numbers returned same value with high probability");
	}
}


void test_main(void)
{
	ztest_test_suite(common_test,
			 ztest_unit_test(test_rand32)
			 );

	ztest_run_test_suite(common_test);
}
