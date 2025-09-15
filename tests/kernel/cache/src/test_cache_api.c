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

ZTEST(cache_api, test_cacheline_aligned_padded)
{
#if defined(CONFIG_DCACHE_LINE_SIZE)
	static uint8_t var1 __cacheline_padded;
	static uint8_t var2[5] __cacheline_padded;
	static uint8_t var3 __cacheline_padded;
	static uint8_t var4 __cacheline_aligned;
	static uint8_t var5[5] __cacheline_aligned;
	static uint8_t var6 __cacheline_aligned;
	extern uintptr_t __data_cache_start;
	extern uintptr_t __data_cache_end;

	zassert_true(IS_ALIGNED(&var1, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var2, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(&var3, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(&var4, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(var5, CONFIG_DCACHE_LINE_SIZE));
	zassert_true(IS_ALIGNED(&var6, CONFIG_DCACHE_LINE_SIZE));

	/* Validate that variables are placed in the dedicated section. */
	zassert_between_inclusive((uintptr_t)&var1,
				  (uintptr_t)&__data_cache_start,
				  (uintptr_t)&__data_cache_end);
	zassert_between_inclusive((uintptr_t)var2,
				  (uintptr_t)&__data_cache_start,
				  (uintptr_t)&__data_cache_end);
	zassert_between_inclusive((uintptr_t)&var3,
				  (uintptr_t)&__data_cache_start,
				  (uintptr_t)&__data_cache_end);

	var1 = 1;
	var2[0] = 2;
	var3 = 3;
	var4 = 4;
	var5[0] = 5;
	var6 = 6;
	zassert_equal(var1, 1);
	zassert_equal(var2[0], 2);
	zassert_equal(var3, 3);
	zassert_equal(var4, 4);
	zassert_equal(var5[0], 5);
	zassert_equal(var6, 6);
#endif
}

static void *cache_api_setup(void)
{
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
