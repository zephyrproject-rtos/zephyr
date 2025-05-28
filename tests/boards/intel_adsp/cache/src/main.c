/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <adsp_memory.h>

ZTEST(adsp_cache, test_adsp_cache_flush_inv_all)
{
	uint32_t *cached, *uncached;

	cached = (uint32_t *)LP_SRAM_BASE;
	uncached = sys_cache_uncached_ptr_get(cached);

	*cached = 42;
	*uncached = 40;

	/* Just some sanity checks */
	zassert_equal(*cached, 42);
	zassert_equal(*uncached, 40);

	sys_cache_data_flush_and_invd_all();

	/* After sys_cache_data_flush_and_invd_all(), uncached should be updated */
	zassert_equal(*cached, 42);
	zassert_equal(*uncached, 42);

	/* Flush and invalidate again, this time to check the invalidate part */
	sys_cache_data_flush_and_invd_all();
	*uncached = 80;

	/* As cache is invalid, cached should be updated with uncached new value */
	zassert_equal(*cached, 80);
	zassert_equal(*uncached, 80);

	*cached = 82;

	/* Only cached should have changed */
	zassert_equal(*cached, 82);
	zassert_equal(*uncached, 80);

	sys_cache_data_flush_all();

	/* After sys_cache_data_flush_all(), uncached should be updated */
	zassert_equal(*cached, 82);
	zassert_equal(*uncached, 82);

	*uncached = 100;

	/* As cache is not invalid, only uncached should be updated */
	zassert_equal(*cached, 82);
	zassert_equal(*uncached, 100);

	sys_cache_data_invd_all();

	/* Now, cached should be updated */
	zassert_equal(*cached, 100);
	zassert_equal(*uncached, 100);
}

ZTEST_SUITE(adsp_cache, NULL, NULL, NULL, NULL, NULL);
