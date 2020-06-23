/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

static std::array<int, 4> array = {1, 2, 3, 4};

void test_array(void)
{
	zassert_equal(array.size(), 4, "unexpected size");
	zassert_equal(array[0], 1, "array[0] wrong");
	zassert_equal(array[3], 4, "array[3] wrong");

	std::array<uint8_t, 2> local = {1, 2};
	zassert_equal(local.size(), 2, "unexpected size");
	zassert_equal(local[0], 1, "local[0] wrong");
	zassert_equal(local[1], 2, "local[1] wrong");
}
