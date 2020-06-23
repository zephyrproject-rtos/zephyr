/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <vector>
#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

static std::array<int, 4> array = {1, 2, 3, 4};
static std::vector<int> vector;

void test_vector(void)
{
	zassert_equal(vector.size(), 0, "vector init nonzero");
	for (auto v : array) {
		vector.push_back(v);
	}
	zassert_equal(vector.size(), array.size(), "vector store failed");
}
