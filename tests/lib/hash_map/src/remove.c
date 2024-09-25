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
		ret = INSERT_FUNC(&map, i, i, NULL, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = MANY; i > 0; --i) {
		zassert_equal(true, REMOVE_FUNC(&map, i - 1, NULL, NULL));
		zassert_equal(i - 1, sys_hashmap_size(&map));
	}

	/* after removing the last node, buckets should also be freed */
	zassert_equal(map.data->buckets, NULL);
	zassert_equal(map.data->n_buckets, 0);
}

ZTEST(hash_map, test_remove_false)
{
	KEY_TYPE key_rem = ALLOC_KEY(42);
	KEY_TYPE key_insert = ALLOC_KEY(1);

	zassert_true(sys_hashmap_is_empty(&map));
	zassert_false(REMOVE_FUNC_NAME(&map, key_rem, NULL, NULL));

	zassert_equal(1, INSERT_FUNC_NAME(&map, key_insert, 1, NULL, NULL));
	zassert_false(REMOVE_FUNC_NAME(&map, key_rem, NULL, NULL));

	FREE_KEY(key_rem);
	FREE_KEY(key_insert);
}
