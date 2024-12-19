/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"
#include "zephyr/sys/util.h"
#include <stdint.h>
#include <string.h>

#define KEY_1   "first_key"
#define VALUE_1 10
#define KEY_2   "second_key"
#define VALUE_2 20
#define KEY_3   "third_key"
#define VALUE_3 30

ZTEST(hash_map_string, test_string_insert)
{
	int ret = 0;
	struct sys_hashmap *const string_map = &map;

	zassert_true(sys_hashmap_is_empty(string_map));

	ret = string_map_insert(string_map, KEY_1, VALUE_1, NULL, NULL);
	zassert_equal(ret, 1);

	zassert_equal(sys_hashmap_size(string_map), 1);

	ret = string_map_insert(string_map, KEY_2, VALUE_2, NULL, NULL);
	zassert_equal(ret, 1);
	ret = string_map_insert(string_map, KEY_3, VALUE_3, NULL, NULL);
	zassert_equal(ret, 1);

	zassert_equal(sys_hashmap_size(string_map), 3);

	uint64_t value = 0;
	/* get with same pointer */
	string_map_get(string_map, KEY_1, &value);
	zassert_equal(value, VALUE_1);

	string_map_get(string_map, KEY_2, &value);
	zassert_equal(value, VALUE_2);

	string_map_get(string_map, KEY_3, &value);
	zassert_equal(value, VALUE_3);

	/* get with same string */
	size_t key_max_len = MAX(strlen(KEY_1), MAX(strlen(KEY_2), strlen(KEY_3))) + 1;
	char *key_buffer = calloc(key_max_len, sizeof(char));

	strcpy(key_buffer, KEY_1);
	string_map_get(string_map, key_buffer, &value);
	zassert_equal(value, VALUE_1);

	strcpy(key_buffer, KEY_2);
	string_map_get(string_map, key_buffer, &value);
	zassert_equal(value, VALUE_2);

	strcpy(key_buffer, KEY_3);
	string_map_get(string_map, key_buffer, &value);
	zassert_equal(value, VALUE_3);

	free(key_buffer);
}

#define VALUE_1_REPLACE 42

ZTEST(hash_map_string, test_string_insert_replacement)
{
	int ret = 0;
	struct sys_hashmap *const string_map = &map;

	zassert_true(sys_hashmap_is_empty(string_map));

	char key_1_permanent_storage[] = KEY_1;

	/* insert */
	ret = string_map_insert(string_map, key_1_permanent_storage, VALUE_1, NULL, NULL);
	zassert_equal(ret, 1);

	uint64_t value = 0;

	/* get with same pointer */
	string_map_get(string_map, key_1_permanent_storage, &value);
	zassert_equal(value, VALUE_1);

	size_t key_max_len = MAX(strlen(KEY_1), MAX(strlen(KEY_2), strlen(KEY_3))) + 1;
	char *key_buffer = calloc(key_max_len, sizeof(char));

	strcpy(key_buffer, KEY_1);

	/* get using a different pointer to an equal string */
	string_map_get(string_map, key_buffer, &value);
	zassert_equal(value, VALUE_1);

	zassert_equal(sys_hashmap_size(string_map), 1);

	char *old_key = {0};

	/* insert using a different pointer to an equal string */
	ret = string_map_insert(string_map, key_buffer, VALUE_1_REPLACE, &value, &old_key);
	/* no new entry */
	zassert_equal(ret, 0);
	zassert_equal(value, VALUE_1);
	zassert_equal_ptr(old_key, key_1_permanent_storage);

	zassert_equal(sys_hashmap_size(string_map), 1);

	/* get using a different pointer to an equal string */
	string_map_get(string_map, key_1_permanent_storage, &value);
	zassert_equal(value, VALUE_1_REPLACE);

	/* get with same pointer */
	string_map_get(string_map, key_buffer, &value);
	zassert_equal(value, VALUE_1_REPLACE);

	free(key_buffer);
}
