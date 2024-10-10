/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#define KEY_1 "first_key"
#define KEY_2 "second_key"

ZTEST(hash_map_string, test_insert_no_replacement)
{
	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, string_map_insert(&map, KEY_1, 1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(string_map_contains_key(&map, KEY_1));

	zassert_equal(1, string_map_insert(&map, KEY_2, 2, NULL, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
	zassert_true(string_map_contains_key(&map, KEY_2));
}

ZTEST(hash_map_string, test_insert_replacement)
{
	uint64_t old_value;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, string_map_insert(&map, KEY_1, 1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(string_map_contains_key(&map, KEY_1));

	old_value = 0x42;
	zassert_equal(0, string_map_insert(&map, KEY_1, 2, &old_value, NULL));
	zassert_equal(1, old_value);
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(string_map_contains_key(&map, KEY_1));
}

ZTEST(hash_map_string, test_insert_many)
{
	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);
		int ret = string_map_insert(&map, index_str, i, NULL, NULL);

		zassert_equal(1, ret, "failed to insert (%s, %zu): %d", index_str, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);

		zassert_true(string_map_contains_key(&map, index_str));
		free(index_str);
	}

	/* free the hashmap keys correctly before exiting */
	sys_hashmap_clear(&map, string_map_free_callback, NULL);
}
