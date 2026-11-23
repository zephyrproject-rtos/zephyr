/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_remove_true)
{
	int ret;

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = MANY; i > 0; --i) {
		zassert_equal(true, sys_hashmap_remove(&map, i - 1, NULL));
		zassert_equal(i - 1, sys_hashmap_size(&map));
	}

	/* after removing the last node, buckets should also be freed */
	zassert_equal(map.data->buckets, NULL);
	zassert_equal(map.data->n_buckets, 0);
}

ZTEST(hash_map, test_remove_false)
{
	zassert_true(sys_hashmap_is_empty(&map));
	zassert_false(sys_hashmap_remove(&map, 42, NULL));

	zassert_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_false(sys_hashmap_remove(&map, 42, NULL));
}

ZTEST(hash_map, test_remove_entry)
{
	uint64_t entry = 0xF00DF00DF00DF00DU;
	uint64_t old_value;

	/* Fill hashmap so that the rehashing condition is not always met when running the test */
	for (size_t i = 0; i < 20; ++i) {
		zassert_false(sys_hashmap_insert(&map, i, i, NULL) < 0);
	}

	/* Remove key 16, expecting its entry to be erased */
	old_value = 0;
	zassert_true(sys_hashmap_remove(&map, 16, &old_value));
	zassert_equal(16, old_value);

	/* Insert an entry at key 16, expecting no old entry to be returned */
	old_value = 0;
	zassert_equal(1, sys_hashmap_insert(&map, 16, entry, &old_value));
	zassert_equal(0, old_value);
}
