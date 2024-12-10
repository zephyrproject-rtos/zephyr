/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#define KEY_1   "first_key"
#define VALUE_1 1
#define KEY_2   "second_key"
#define VALUE_2 2

ZTEST(hash_map_string, test_get_true)
{
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));
	zassert_equal(1, string_map_insert(&map, KEY_1, VALUE_1, NULL, NULL));
	zassert_true(string_map_get(&map, KEY_1, NULL));
	zassert_true(string_map_get(&map, KEY_1, &value));
	zassert_equal(VALUE_1, value);

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);

		ret = string_map_insert(&map, index_str, i, NULL, NULL);
		zassert_equal(1, ret, "failed to insert (%s, %zu): %d", index_str, i, ret);
	}

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);

		zassert_true(string_map_get(&map, index_str, NULL));
		free(index_str);
	}
}

ZTEST(hash_map_string, test_get_false)
{
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_false(string_map_get(&map, KEY_2, &value));
	zassert_equal(value, 0x42);

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);

		ret = string_map_insert(&map, index_str, i, NULL, NULL);
		zassert_equal(1, ret, "failed to insert (%s, %zu): %d", index_str, i, ret);
	}

	zassert_false(string_map_get(&map, KEY_1, NULL));

	/* free the hashmap keys correctly before exiting */
	sys_hashmap_clear(&map, string_map_free_callback, NULL);
}
