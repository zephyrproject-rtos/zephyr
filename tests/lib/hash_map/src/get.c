/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_get_true) {
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, INSERT_FUNC(&map, 0, 0, NULL, NULL));
	zassert_true(GET_FUNC(&map, 0, NULL));
	zassert_true(GET_FUNC(&map, 0, &value));
	zassert_equal(0, value);

	for (size_t i = 1; i < MANY; ++i) {
		ret = INSERT_FUNC(&map, i, i, NULL, NULL);

		zassert_equal(1, ret, "failed to insert (" KEY_FORMAT ", %zu): %d",
									GET_KEY(i), i, ret);
	}

	for (size_t i = 0; i < MANY; ++i) {
		zassert_true(GET_FUNC(&map, i, NULL));
	}
}

ZTEST(hash_map, test_get_false)
{
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));

	KEY_TYPE key_empty = ALLOC_KEY(73);

	zassert_false(GET_FUNC_NAME(&map, key_empty, &value));
	zassert_equal(value, 0x42);
	FREE_KEY(key_empty);

	for (size_t i = 0; i < MANY; ++i) {
		ret = INSERT_FUNC(&map, i, i, NULL, NULL);

		zassert_equal(1, ret, "failed to insert (" KEY_FORMAT ", %zu): %d",
									GET_KEY(i), i, ret);
	}

	KEY_TYPE key_not_exist = ALLOC_KEY(0x4242424242424242ULL);

	zassert_false(GET_FUNC_NAME(&map, key_not_exist, NULL));
	FREE_KEY(key_not_exist);
}
