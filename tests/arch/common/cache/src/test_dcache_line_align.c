/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_DCACHE_LINE_SIZE)

#include <zephyr/ztest.h>
#include <zephyr/cache.h>

/**
 * @brief DCache line alignment attributes tests
 * @defgroup tests_dcache_line DCache line alignment attributes
 * @ingroup all_tests
 * @{
 */


#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>

static uint8_t var_aligned1 __dcacheline_aligned;
static uint8_t var_aligned2[5] __dcacheline_aligned;
static uint8_t var_control1;

ZTEST(dcache_line_align, test_dcacheline_aligned)
{
	zassert_true(IS_ALIGNED(&var_aligned1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_aligned2, CONFIG_DCACHE_LINE_SIZE));
	zassert_false(IS_ALIGNED(&var_aligned1+1, CONFIG_DCACHE_LINE_SIZE));

	var_aligned1 = 4;
	var_aligned2[0] = 5;
	var_control1 = 6;

	zassert_equal(var_aligned1, 4);
	zassert_equal(var_aligned2[0], 5);
	zassert_equal(var_control1, 6);
}

static uint8_t var_exclusive_noinit1 __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit2[5] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit3[3] __dcacheline_exclusive_noinit;

ZTEST(dcache_line_align, test_dcacheline_exclusive_noinit)
{
	zassert_between_inclusive((uintptr_t)&var_exclusive_noinit1,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);
	zassert_between_inclusive((uintptr_t)var_exclusive_noinit2,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);
	zassert_between_inclusive((uintptr_t)var_exclusive_noinit3,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);

	zassert_true(IS_ALIGNED(&var_exclusive_noinit1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_exclusive_noinit2, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_exclusive_noinit3, CONFIG_DCACHE_LINE_SIZE));

	zassert_true(ROUND_DOWN((uintptr_t)&var_exclusive_noinit1, CONFIG_DCACHE_LINE_SIZE)
		!= ROUND_DOWN((uintptr_t)var_exclusive_noinit2, CONFIG_DCACHE_LINE_SIZE));

	var_exclusive_noinit1 = 1;
	var_exclusive_noinit2[0] = 2;
	var_exclusive_noinit3[2] = 3;
	var_control1 = 7;

	zassert_equal(var_exclusive_noinit1, 1);
	zassert_equal(var_exclusive_noinit2[0], 2);
	zassert_equal(var_exclusive_noinit3[2], 3);
	zassert_equal(var_control1, 7);
}

static __dcacheline_exclusive_data uint8_t var_exclusive_data1 = 9;
static __dcacheline_exclusive_data uint8_t var_exclusive_data2[5] = {4};
static __dcacheline_exclusive_data uint8_t var_exclusive_data3[3] = {7};
static uint8_t var_control2 = 3;

ZTEST(dcache_line_align, test_dcacheline_exclusive_data)
{
	zassert_between_inclusive((uintptr_t)&var_exclusive_data1,
				  (uintptr_t)__dcacheline_exclusive_data_start,
				  (uintptr_t)__dcacheline_exclusive_data_end);
	zassert_between_inclusive((uintptr_t)var_exclusive_data2,
				  (uintptr_t)__dcacheline_exclusive_data_start,
				  (uintptr_t)__dcacheline_exclusive_data_end);
	zassert_between_inclusive((uintptr_t)&var_exclusive_data3,
				  (uintptr_t)__dcacheline_exclusive_data_start,
				  (uintptr_t)__dcacheline_exclusive_data_end);

	zassert_true(IS_ALIGNED(&var_exclusive_data1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_exclusive_data2, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(&var_exclusive_data3, CONFIG_DCACHE_LINE_SIZE));

	zassert_true(ROUND_DOWN((uintptr_t)&var_exclusive_data1, CONFIG_DCACHE_LINE_SIZE)
		!= ROUND_DOWN((uintptr_t)var_exclusive_data2, CONFIG_DCACHE_LINE_SIZE));

	zassert_equal(var_exclusive_data1, 9);
	zassert_equal(var_exclusive_data2[0], 4);
	zassert_equal(var_exclusive_data3[0], 7);
	zassert_equal(var_control2, 3);
}

ZTEST_SUITE(dcache_line_align, NULL, NULL, NULL, NULL, NULL);

/**
 * @}
 */

#endif /* CONFIG_DCACHE_LINE_SIZE */
