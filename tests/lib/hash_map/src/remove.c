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
