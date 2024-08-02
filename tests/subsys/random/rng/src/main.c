/* test random number generator APIs */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This tests the following random number routines:
 * void z_early_rand_get(uint8_t *buf, size_t length)
 * uint32_t sys_rand32_get(void);
 */


#include <zephyr/ztest.h>
#include <kernel_internal.h>
#include <zephyr/random/random.h>

#define N_VALUES 10


/**
 *
 * @brief Regression test's entry point
 *
 */

ZTEST(rng_common, test_rand32)
{
	uint32_t gen, last_gen, tmp;
	int rnd_cnt;
	int equal_count = 0;
	uint32_t buf[N_VALUES];

	/* Test early boot random number generation function */
	/* Cover the case, where argument "length" is < size of "size_t" */
	z_early_rand_get((uint8_t *)&tmp, (size_t)1);
	z_early_rand_get((uint8_t *)&last_gen, sizeof(last_gen));
	z_early_rand_get((uint8_t *)&gen, sizeof(gen));
	zassert_true(last_gen != gen && last_gen != tmp && tmp != gen,
			"z_early_rand_get failed");

	/*
	 * Test subsequently calls sys_rand32_get(), checking
	 * that two values are not equal.
	 */
	printk("Generating random numbers\n");
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

	printk("Generating bulk fill random numbers\n");
	memset(buf, 0, sizeof(buf));
	sys_rand_get((uint8_t *)(&buf[0]), sizeof(buf));

	for (rnd_cnt = 0; rnd_cnt < (N_VALUES - 1); rnd_cnt++) {
		gen = buf[rnd_cnt];
		if (gen == last_gen) {
			equal_count++;
		}
		last_gen = gen;
	}

	if (equal_count > N_VALUES / 2) {
		zassert_false((equal_count > N_VALUES / 2),
		"random numbers returned same value with high probability");
	}

#if defined(CONFIG_CSPRNG_ENABLED)

	printk("Generating bulk fill cryptographically secure random numbers\n");

	memset(buf, 0, sizeof(buf));

	int err = sys_csrand_get(buf, sizeof(buf));

	zassert_true(err == 0, "sys_csrand_get returned an error");

	for (rnd_cnt = 0; rnd_cnt < (N_VALUES - 1); rnd_cnt++) {
		gen = buf[rnd_cnt];
		if (gen == last_gen) {
			equal_count++;
		}
		last_gen = gen;
	}

	if (equal_count > N_VALUES / 2) {
		zassert_false((equal_count > N_VALUES / 2),
		"random numbers returned same value with high probability");
	}

#else

	printk("Cryptographically secure random number APIs not enabled\n");

#endif /* CONFIG_CSPRNG_ENABLED */
}

ZTEST_SUITE(rng_common, NULL, NULL, NULL, NULL, NULL);
