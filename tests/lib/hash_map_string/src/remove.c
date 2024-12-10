/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"
#include "zephyr/ztest_assert.h"

ZTEST(hash_map_string, test_remove_true)
{
	int ret;

	for (size_t i = 0; i < MANY; ++i) {
		char *index_str = alloc_string_from_index(i);

		ret = string_map_insert(&map, index_str, i, NULL, NULL);
		zassert_equal(1, ret, "failed to insert (%s, %zu): %d", index_str, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = MANY; i > 0; --i) {
		char *index_str_2 = alloc_string_from_index(i - 1);
		char *key_removed = {0};
		uint64_t value_removed = 0;

		zassert_equal(true,
			      string_map_remove(&map, index_str_2, &value_removed, &key_removed));
		/* assert that the ond key is not null */
		zassert_not_equal(key_removed, NULL);
		/* compare key string value */
		zassert_equal(strcmp(index_str_2, key_removed), 0);
		/* compare stored value */
		zassert_equal(i - 1, value_removed);
		/* check size */
		zassert_equal(i - 1, sys_hashmap_size(&map));
		/* free created string keys */
		free(index_str_2);
		free(key_removed);
	}

	/* after removing the last node, buckets should also be freed */
	zassert_equal(map.data->buckets, NULL);
	zassert_equal(map.data->n_buckets, 0);
}

#define KEY_1     "first_key"
#define OTHER_KEY "other_key"

ZTEST(hash_map_string, test_remove_false)
{
	zassert_true(sys_hashmap_is_empty(&map));
	zassert_false(string_map_remove(&map, KEY_1, NULL, NULL));

	zassert_equal(1, string_map_insert(&map, OTHER_KEY, 1, NULL, NULL));
	zassert_false(string_map_remove(&map, KEY_1, NULL, NULL));
}
