/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#define N 10

ZTEST(hash_map_string, test_clear_no_callback)
{
	char *string_keys[N] = {0};

	zassert_true(sys_hashmap_is_empty(&map));
	for (size_t i = 0; i < N; ++i) {
		string_keys[i] = alloc_string_from_index(i);
		zassert_equal(1, string_map_insert(&map, string_keys[i], i, NULL, NULL));
	}

	zassert_equal(N, sys_hashmap_size(&map));

	sys_hashmap_clear(&map, NULL, NULL);
	zassert_true(sys_hashmap_is_empty(&map));

	/* we still have to free the keys */
	for (size_t i = 0; i < N; ++i) {
		free(string_keys[i]);
	}
}

static void clear_callback(uint64_t key, uint64_t value, void *cookie)
{
	bool *cleared = (bool *)cookie;

	zassert_true(value < 10);
	cleared[value] = true;

	/* we also free the string key by calling the free callback */
	string_map_free_callback(key, value, cookie);
}

ZTEST(hash_map_string, test_clear_callback)
{
	bool cleared[10] = {0};

	zassert_true(sys_hashmap_is_empty(&map));
	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		char *index_str = alloc_string_from_index(i);
		int ret = string_map_insert(&map, index_str, i, NULL, NULL);

		zassert_equal(1, ret, "failed to insert (%s, %zu): %d", index_str, i, ret);
	}

	zassert_equal(ARRAY_SIZE(cleared), sys_hashmap_size(&map));

	sys_hashmap_clear(&map, clear_callback, cleared);
	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(cleared); ++i) {
		zassert_true(cleared[i], "entry %zu was not cleared", i + 1);
	}
}
