/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#ifdef CONFIG_TEST_USE_CUSTOM_EQ_FUNC
static void foreach_callback(char *key, uint64_t value, void *cookie)
{
	bool *called = (bool *)cookie;
	uint64_t uint_key = get_key_index(key);

	zassert_true(uint_key < 10);
	called[uint_key] = true;
}
#else
static void foreach_callback(uint64_t key, uint64_t value, void *cookie)
{
	bool *called = (bool *)cookie;

	zassert_true(key < 10);
	called[key] = true;
}
#endif

ZTEST(hash_map, test_foreach)
{
	bool called[10] = {0};

	BUILD_ASSERT(ARRAY_SIZE(called) <= MANY, "the size has to be <= than MANY"
							" to be able to use preallocated string keys");

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(called); ++i) {
		zassert_equal(1, INSERT_FUNC(&map, i, i, NULL, NULL));
	}

	zassert_equal(ARRAY_SIZE(called), sys_hashmap_size(&map));

	FOREACH_FUNC_NAME(&map, foreach_callback, called);
	for (size_t i = 0; i < ARRAY_SIZE(called); ++i) {
		zassert_true(called[i], "entry " KEY_FORMAT " was not called", GET_KEY(i + 1));
	}
}
