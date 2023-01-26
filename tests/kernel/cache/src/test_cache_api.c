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

	ret = sys_cache_data_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP));

	ret = sys_cache_data_flush_and_invd_all();
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
