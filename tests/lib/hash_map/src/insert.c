/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

static uint32_t collision_hash(const void *data, size_t len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return 0U;
}

ZTEST(hash_map, test_insert_replacement_after_collision_remove)
{
	SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(collision_map, collision_hash,
						   SYS_HASHMAP_DEFAULT_ALLOCATOR,
						   SYS_HASHMAP_CONFIG(SIZE_MAX, 75));
	uint64_t old_value = 0;
	uint64_t value = 0;
	size_t n_buckets;

	for (uint64_t key = 1; key <= 5; key++) {
		zassert_equal(sys_hashmap_insert(&collision_map, key, key, NULL), 1);
	}

	n_buckets = sys_hashmap_num_buckets(&collision_map);
	zassert_true(sys_hashmap_remove(&collision_map, 1, NULL));
	zassert_equal(sys_hashmap_num_buckets(&collision_map), n_buckets);

	zexpect_equal(sys_hashmap_insert(&collision_map, 5, 50, &old_value), 0);
	zexpect_equal(old_value, 5);
	zexpect_equal(sys_hashmap_size(&collision_map), 4);
	zexpect_equal(sys_hashmap_num_buckets(&collision_map), n_buckets);
	zexpect_true(sys_hashmap_remove(&collision_map, 5, &value));
	zexpect_equal(value, 50);
	zexpect_false(sys_hashmap_contains_key(&collision_map, 5));
	zexpect_equal(sys_hashmap_size(&collision_map), 3);
	sys_hashmap_clear(&collision_map, NULL, NULL);
}

ZTEST(hash_map, test_insert_no_replacement)
{
	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));

	zassert_equal(1, sys_hashmap_insert(&map, 2, 2, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 2));
}

ZTEST(hash_map, test_insert_replacement)
{
	uint64_t old_value;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));

	old_value = 0x42;
	zassert_equal(0, sys_hashmap_insert(&map, 1, 2, &old_value));
	zassert_equal(1, old_value);
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));
}

ZTEST(hash_map, test_insert_many)
{
	int ret;

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = 0; i < MANY; ++i) {
		zassert_true(sys_hashmap_contains_key(&map, i));
	}
}
