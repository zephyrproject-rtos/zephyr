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

	zassert_true(sys_hashmap_is_empty(&map));
	for (size_t i = 0; i < N; ++i) {
		zassert_equal(1, sys_hashmap_insert(&map, i, i, NULL));
	}

	zassert_equal(N, sys_hashmap_size(&map));

	sys_hashmap_clear(&map, NULL, NULL);
	zassert_true(sys_hashmap_is_empty(&map));
}

static void clear_callback(uint64_t key, uint64_t value, void *cookie)
{
	bool *cleared = (bool *)cookie;

	zassert_true(key < 10);
	cleared[key] = true;
}

ZTEST(hash_map, test_clear_callback)
{
	bool cleared[10] = {0};

	zassert_true(sys_hashmap_is_empty(&map));
	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		zassert_equal(1, sys_hashmap_insert(&map, i, i, NULL));
	}

	zassert_equal(ARRAY_SIZE(cleared), sys_hashmap_size(&map));

	sys_hashmap_clear(&map, clear_callback, cleared);
	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		zassert_true(cleared[i], "entry %zu was not cleared", i + 1);
	}
}
