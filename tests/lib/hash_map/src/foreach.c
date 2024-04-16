/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

static void foreach_callback(uint64_t key, uint64_t value, void *cookie)
{
	bool *called = (bool *)cookie;

	zassert_true(key < 10);
	called[key] = true;
}

ZTEST(hash_map, test_foreach)
{
	bool called[10] = {0};

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < ARRAY_SIZE(called); ++i) {
		zassert_equal(1, sys_hashmap_insert(&map, i, i, NULL));
	}

	zassert_equal(ARRAY_SIZE(called), sys_hashmap_size(&map));

	sys_hashmap_foreach(&map, foreach_callback, called);
	for (size_t i = 0; i < ARRAY_SIZE(called); ++i) {
		zassert_true(called[i], "entry %zu was not called", i + 1);
	}
}
