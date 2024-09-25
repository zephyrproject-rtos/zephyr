/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_insert_no_replacement)
{
	zassert_true(sys_hashmap_is_empty(&map));

	KEY_TYPE key1 = ALLOC_KEY(1);

	zassert_equal(1, INSERT_FUNC_NAME(&map, key1, 1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(CONTAINS_KEY_FUNC_NAME(&map, key1));

	KEY_TYPE key2 = ALLOC_KEY(2);

	zassert_equal(1, INSERT_FUNC_NAME(&map, key2, 2, NULL, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
	zassert_true(CONTAINS_KEY_FUNC_NAME(&map, key2));

	FREE_KEY(key1);
	FREE_KEY(key2);
}

ZTEST(hash_map, test_insert_replacement)
{
	uint64_t old_value;

	zassert_true(sys_hashmap_is_empty(&map));

	KEY_TYPE key1 = ALLOC_KEY(1);

	zassert_equal(1, INSERT_FUNC_NAME(&map, key1, 1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(CONTAINS_KEY_FUNC_NAME(&map, key1));

	old_value = 0x42;

	zassert_equal(0, INSERT_FUNC_NAME(&map, key1, 2, &old_value, NULL));
	zassert_equal(1, old_value);
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(CONTAINS_KEY_FUNC_NAME(&map, key1));

	FREE_KEY(key1);
}

ZTEST(hash_map, test_insert_many)
{
	int ret;

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < MANY; ++i) {
		ret = INSERT_FUNC(&map, i, i, NULL, NULL);

		zassert_equal(1, ret, "failed to insert (" KEY_FORMAT ", %zu): %d",
									GET_KEY(i), i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = 0; i < MANY; ++i) {
		zassert_true(CONTAINS_KEY_FUNC(&map, i));
	}
}
