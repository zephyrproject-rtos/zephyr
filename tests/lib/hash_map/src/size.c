/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_size)
{
	zassert_equal(0, sys_hashmap_size(&map));

	KEY_TYPE key1 = ALLOC_KEY(1);

	zassume_equal(1, INSERT_FUNC_NAME(&map, key1, 1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));

	KEY_TYPE key2 = ALLOC_KEY(2);

	zassume_equal(1, INSERT_FUNC_NAME(&map, key2, 2, NULL, NULL));
	zassert_equal(2, sys_hashmap_size(&map));

	FREE_KEY(key1);
	FREE_KEY(key2);
}
