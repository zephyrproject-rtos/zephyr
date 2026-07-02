/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>

/**
 * @brief Architecture cache management API tests
 * @defgroup tests_arch_cache_api Cache management API
 * @ingroup all_tests
 * @{
 */

#define SIZE	(4096)

static ZTEST_BMEM uint8_t user_buffer[SIZE];

/**
 * @brief Verify that the instruction-cache management API is callable and
 *        returns a supported status.
 *
 * @details
 * Exercises every instruction-cache maintenance entry point exposed by the
 * cache API. The purpose is to confirm that the architecture/SoC layer
 * provides a consistent implementation: each operation must either complete
 * successfully (0) or explicitly report that it is not supported (-ENOTSUP).
 * No other return value is acceptable. On platforms where the i-cache cannot
 * be operated on safely (e.g. Xtensa MMU, where the user buffer is not
 * executable and a range invalidate would raise an instruction-fetch
 * prohibited exception) the whole case is skipped to avoid partial coverage.
 *
 * Test steps:
 * - Skip the test on configurations where i-cache operations are unsafe.
 * - Call the whole-cache flush, invalidate and flush-and-invalidate ops.
 * - Call the by-range flush, invalidate and flush-and-invalidate ops over a
 *   known buffer.
 *
 * Expected result:
 * - Every call returns 0 or -ENOTSUP.
 *
 * @see sys_cache_instr_flush_all()
 * @see sys_cache_instr_invd_all()
 * @see sys_cache_instr_flush_and_invd_all()
 * @see sys_cache_instr_flush_range()
 * @see sys_cache_instr_invd_range()
 * @see sys_cache_instr_flush_and_invd_range()
 */
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

/**
 * @brief Verify that the data-cache management API is callable and returns a
 *        supported status.
 *
 * @details
 * Exercises the data-cache maintenance entry points from supervisor mode.
 * Each operation must either complete successfully (0) or report that it is
 * not supported (-ENOTSUP); any other value indicates a broken
 * architecture/SoC cache backend. Both whole-cache and by-range variants are
 * covered so a platform that implements one granularity but not the other is
 * still validated.
 *
 * Test steps:
 * - Call the whole-cache flush, invalidate and flush-and-invalidate ops. The
 *   invalidate is issued after a flush so it cannot drop dirty lines the
 *   framework still relies on.
 * - Call the by-range flush, invalidate and flush-and-invalidate ops over a
 *   known buffer.
 *
 * Expected result:
 * - Every call returns 0 or -ENOTSUP.
 *
 * @see sys_cache_data_flush_all()
 * @see sys_cache_data_invd_all()
 * @see sys_cache_data_flush_and_invd_all()
 * @see sys_cache_data_flush_range()
 * @see sys_cache_data_invd_range()
 * @see sys_cache_data_flush_and_invd_range()
 */
ZTEST(cache_api, test_data_cache_api)
{
	int ret;

	ret = sys_cache_data_flush_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	/* Flush first so the whole-cache invalidate does not discard dirty
	 * lines that the test framework still depends on.
	 */
	ret = sys_cache_data_invd_all();
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

/**
 * @brief Verify that the by-range data-cache API is callable from user mode.
 *
 * @details
 * Repeats the by-range data-cache maintenance operations from an unprivileged
 * (user-mode) thread to confirm they are reachable without faulting and honor
 * the same return-value contract as in supervisor mode. The buffer operated on
 * lives in user-accessible memory (ZTEST_BMEM). Only the range variants are
 * exercised, since those are the operations a user thread can legitimately
 * request against its own buffer.
 *
 * Test steps:
 * - From a user-mode thread, call the by-range data-cache flush, invalidate
 *   and flush-and-invalidate ops over a user-accessible buffer.
 *
 * Expected result:
 * - Every call returns 0 or -ENOTSUP, and no user-mode fault occurs.
 *
 * @see sys_cache_data_flush_range()
 * @see sys_cache_data_invd_range()
 * @see sys_cache_data_flush_and_invd_range()
 */
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

/**
 * @}
 */
