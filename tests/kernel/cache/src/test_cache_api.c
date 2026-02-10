/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>

#define SIZE	(4096)

static ZTEST_BMEM uint8_t user_buffer[SIZE];

ZTEST(cache_api, test_instr_cache_api)
{
	int ret;

#ifdef CONFIG_XTENSA_MMU
	/* With MMU enabled, user_buffer is not marked as executable.
	 * Invalidating the i-cache by region will cause an instruction
	 * fetch prohibited exception. So skip all i-cache tests,
	 * instead of just the range ones to avoid confusions of
	 * only running the test partially.
	 */
	ztest_test_skip();
#endif

	ret = sys_cache_instr_flush_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_instr_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_instr_flush_and_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_instr_flush_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_instr_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_instr_flush_and_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));
}

ZTEST(cache_api, test_data_cache_api)
{
	int ret;

	ret = sys_cache_data_flush_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_flush_and_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_flush_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_flush_and_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

}

ZTEST_USER(cache_api, test_data_cache_api_user)
{
	int ret;

	ret = sys_cache_data_flush_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_flush_and_invd_range(user_buffer, SIZE);
	zassert_true((ret == 0) || (ret == -ENOTSUP));
}

#if defined(CONFIG_DCACHE_LINE_SIZE) && CONFIG_DCACHE_LINE_SIZE > 0

#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>

static uint8_t var_aligned1 __dcacheline_aligned;
static uint8_t var_aligned2[5] __dcacheline_aligned;
static uint8_t var_unaligned;

ZTEST(cache_api, test_dcacheline_aligned_exclusive)
{
	zassert_true(IS_ALIGNED(&var_aligned1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_aligned2, CONFIG_DCACHE_LINE_SIZE));
	zassert_false(IS_ALIGNED(&var_unaligned, CONFIG_DCACHE_LINE_SIZE));

	var_aligned1 = 4;
	var_aligned2[0] = 5;
	var_unaligned = 6;

	zassert_equal(var_aligned1, 4);
	zassert_equal(var_aligned2[0], 5);
	zassert_equal(var_unaligned, 6);
}

static uint8_t var_exclusive_noinit1 __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit2[5] __dcacheline_exclusive_noinit;
static uint8_t var_exclusive_noinit3[3] __dcacheline_exclusive_noinit;

static __dcacheline_exclusive_data uint8_t var_exclusive_data1 = 9;
static __dcacheline_exclusive_data uint8_t var_exclusive_data2[5] = {4};
static __dcacheline_exclusive_data uint8_t var_exclusive_data3[3] = {7};

ZTEST(cache_api, test_dcacheline_exclusive)
{
	zassert_between_inclusive((uintptr_t)&var_exclusive_noinit1,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);
	zassert_between_inclusive((uintptr_t)var_exclusive_noinit2,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);
	zassert_between_inclusive((uintptr_t)&var_exclusive_noinit3,
				  (uintptr_t)__dcacheline_exclusive_noinit_start,
				  (uintptr_t)__dcacheline_exclusive_noinit_end);

	zassert_true(IS_ALIGNED(&var_exclusive_noinit1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var_exclusive_noinit2, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(&var_exclusive_noinit3, CONFIG_DCACHE_LINE_SIZE));

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

	var_exclusive_noinit1 = 1;
	var_exclusive_noinit2[0] = 2;
	var_exclusive_noinit3[2] = 3;

	zassert_equal(var_exclusive_noinit1, 1);
	zassert_equal(var_exclusive_noinit2[0], 2);
	zassert_equal(var_exclusive_noinit3[2], 3);

	zassert_equal(var_exclusive_data1, 9);
	zassert_equal(var_exclusive_data2[0], 4);
	zassert_equal(var_exclusive_data3[0], 7);
}
#endif

static void *cache_api_setup(void)
{
	sys_cache_data_disable();
	sys_cache_data_flush_all();
	sys_cache_data_enable();
	sys_cache_instr_enable();

	return NULL;
}

static void cache_api_teardown(void *unused)
{
	sys_cache_data_disable();
	sys_cache_instr_disable();
}

ZTEST_SUITE(cache_api, NULL, cache_api_setup, NULL, NULL, cache_api_teardown);
