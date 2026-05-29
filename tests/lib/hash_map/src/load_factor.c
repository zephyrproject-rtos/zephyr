/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_load_factor_default)
{
	int ret;
	uint32_t load_factor;

	zassert_true(sys_hashmap_is_empty(&map));
	zassert_equal(0, sys_hashmap_load_factor(&map));

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		load_factor = sys_hashmap_load_factor(&map);
		zassert_true(load_factor > 0 && load_factor <= SYS_HASHMAP_DEFAULT_LOAD_FACTOR);
	}

	for (size_t i = MANY; i > 0; --i) {
		zassert_equal(true, sys_hashmap_remove(&map, i - 1, NULL));
		load_factor = sys_hashmap_load_factor(&map);
		zassert_true(load_factor <= SYS_HASHMAP_DEFAULT_LOAD_FACTOR);
	}
}

ZTEST(hash_map, test_load_factor_custom)
{
	int ret;
	uint32_t load_factor;
	struct sys_hashmap *const hmap = &custom_load_factor_map;

	zassert_equal(hmap->config->load_factor, CUSTOM_LOAD_FACTOR);

	zassert_true(sys_hashmap_is_empty(hmap));
	zassert_equal(0, sys_hashmap_load_factor(hmap));

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(hmap, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		load_factor = sys_hashmap_load_factor(hmap);
		zassert_true(load_factor > 0 && load_factor <= CUSTOM_LOAD_FACTOR);
	}

	for (size_t i = MANY; i > 0; --i) {
		zassert_equal(true, sys_hashmap_remove(hmap, i - 1, NULL));
		load_factor = sys_hashmap_load_factor(hmap);
		zassert_true(load_factor <= CUSTOM_LOAD_FACTOR);
	}
}
