/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

static void *fail_alloc(void *ptr, size_t size)
{
	ARG_UNUSED(ptr);
	ARG_UNUSED(size);

	return NULL;
}

ZTEST(hash_map, test_insert_rehash_alloc_failure)
{
	sys_hashmap_allocator_t alloc_func = map.alloc_func;
	uint64_t value;
	int ret;

	if (!IS_ENABLED(CONFIG_SYS_HASH_MAP_CHOICE_SC)) {
		ztest_test_skip();
	}

	zassert_equal(1, sys_hashmap_insert(&map, 1, 42, NULL));
	map.alloc_func = fail_alloc;
	ret = sys_hashmap_insert(&map, 2, 84, NULL);
	map.alloc_func = alloc_func;

	zassert_equal(-ENOMEM, ret);
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_get(&map, 1, &value));
	zassert_equal(42, value);
	zassert_false(sys_hashmap_contains_key(&map, 2));
	zassert_equal(1, sys_hashmap_insert(&map, 2, 84, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
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
