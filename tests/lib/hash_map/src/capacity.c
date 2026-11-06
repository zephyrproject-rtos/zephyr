/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/hash_map.h>
#include <zephyr/ztest.h>

static void check_capacity(struct sys_hashmap *map)
{
	const size_t limit = map->config->max_size;
	uint64_t old_value = 0;
	uint64_t value = 0;
	int fill_ret = 1;
	int replace_ret = 0;
	int ret;
	size_t size;
	bool found = false;
	bool extra_present;

	for (size_t i = 0; i < limit; ++i) {
		ret = sys_hashmap_insert(map, i, i + 10, NULL);
		if (ret != 1) {
			fill_ret = ret;
		}
	}
	if (limit > 0) {
		replace_ret = sys_hashmap_insert(map, 0, 99, &old_value);
	}
	ret = sys_hashmap_insert(map, 42, 42, NULL);
	size = sys_hashmap_size(map);
	extra_present = sys_hashmap_contains_key(map, 42);
	if (limit > 0) {
		found = sys_hashmap_get(map, 0, &value);
	}
	sys_hashmap_clear(map, NULL, NULL);

	zassert_equal(fill_ret, 1);
	zassert_equal(ret, -ENOSPC);
	zassert_equal(size, limit);
	zassert_false(extra_present);
	if (limit > 0) {
		zassert_equal(replace_ret, 0);
		zassert_equal(old_value, 10);
		zassert_true(found);
		zassert_equal(value, 99);
	}
}

ZTEST(hash_map, test_capacity_zero)
{
	SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(
		bounded, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,
		SYS_HASHMAP_CONFIG(0, SYS_HASHMAP_DEFAULT_LOAD_FACTOR));

	check_capacity(&bounded);
}

ZTEST(hash_map, test_capacity_one)
{
	SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(
		bounded, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,
		SYS_HASHMAP_CONFIG(1, SYS_HASHMAP_DEFAULT_LOAD_FACTOR));

	check_capacity(&bounded);
}

ZTEST(hash_map, test_capacity_two)
{
	SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(
		bounded, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,
		SYS_HASHMAP_CONFIG(2, SYS_HASHMAP_DEFAULT_LOAD_FACTOR));

	check_capacity(&bounded);
}
