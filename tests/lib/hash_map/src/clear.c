/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_clear_no_callback)
{
	const size_t N = 10;
	BUILD_ASSERT(N <= MANY, "N has to be <= than MANY to"
							 " be able to use preallocated string keys");

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < N; ++i) {
		zassert_equal(1, INSERT_FUNC(&map, i, i, NULL, NULL));
	}

	zassert_equal(N, sys_hashmap_size(&map));

	sys_hashmap_clear(&map, NULL, NULL);
	zassert_true(sys_hashmap_is_empty(&map));
}

#ifdef CONFIG_TEST_USE_CUSTOM_EQ_FUNC
static void clear_callback(char *key, uint64_t value, void *cookie)
{
	bool *cleared = (bool *)cookie;
	uint64_t uint_key = get_key_index(key);

	zassert_true(uint_key < 10);
	cleared[uint_key] = true;
}
#else
static void clear_callback(uint64_t key, uint64_t value, void *cookie)
{
	bool *cleared = (bool *)cookie;

	zassert_true(key < 10);
	cleared[key] = true;
}
#endif

ZTEST(hash_map, test_clear_callback)
{
	bool cleared[10] = {0};
	BUILD_ASSERT(ARRAY_SIZE(cleared) <= MANY, "the size has to be <= than MANY"
							 " to be able to use preallocated string keys");

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		zassert_equal(1, INSERT_FUNC(&map, i, i, NULL, NULL));
	}

	zassert_equal(ARRAY_SIZE(cleared), sys_hashmap_size(&map));

	CLEAR_FUNC_NAME(&map, clear_callback, cleared);
	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		zassert_true(cleared[i], "entry " KEY_FORMAT " was not cleared", GET_KEY(i + 1));
	}
}
