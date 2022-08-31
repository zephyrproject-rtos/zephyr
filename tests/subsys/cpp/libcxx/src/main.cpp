/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <functional>
#include <memory>
#include <vector>
#include <zephyr/ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

std::array<int, 4> array = {1, 2, 3, 4};
std::vector<int> vector;

ZTEST(libcxx_tests, test_array)
{
	zassert_equal(array.size(), 4, "unexpected size");
	zassert_equal(array[0], 1, "array[0] wrong");
	zassert_equal(array[3], 4, "array[3] wrong");

	std::array<uint8_t, 2> local = {1, 2};
	zassert_equal(local.size(), 2, "unexpected size");
	zassert_equal(local[0], 1, "local[0] wrong");
	zassert_equal(local[1], 2, "local[1] wrong");
}

ZTEST(libcxx_tests, test_vector)
{
	zassert_equal(vector.size(), 0, "vector init nonzero");
	for (auto v : array) {
		vector.push_back(v);
	}
	zassert_equal(vector.size(), array.size(), "vector store failed");
}

struct make_unique_data {
	static int ctors;
	static int dtors;
	int inst;

	make_unique_data () :
	inst{++ctors}
	{ }

	~make_unique_data ()
	{
		++dtors;
	}
};
int make_unique_data::ctors;
int make_unique_data::dtors;

ZTEST(libcxx_tests, test_make_unique)
{
	zassert_equal(make_unique_data::ctors, 0, "ctor count not initialized");
	zassert_equal(make_unique_data::dtors, 0, "dtor count not initialized");
	auto d = std::make_unique<make_unique_data>();
	zassert_true(static_cast<bool>(d), "allocation failed");
	zassert_equal(make_unique_data::ctors, 1, "ctr update failed");
	zassert_equal(d->inst, 1, "instance init failed");
	zassert_equal(make_unique_data::dtors, 0, "dtor count not zero");
	d.reset();
	zassert_false(d, "release failed");
	zassert_equal(make_unique_data::dtors, 1, "dtor count not incremented");
}

#if defined(CONFIG_EXCEPTIONS) && !defined(CONFIG_BOARD_M2GL025_MIV)
static void throw_exception(void)
{
	throw 42;
}

ZTEST(libcxx_tests, test_exception)
{
	try
	{
		throw_exception();
	}
	catch (int i)
	{
		zassert_equal(i, 42, "Incorrect exception value");
		return;
	}

	zassert_unreachable("Missing exception catch");
}
#else

ZTEST(libcxx_tests, test_exception)
{
	ztest_test_skip();
}
#endif

static void *libcxx_tests_setup(void)
{
	TC_PRINT("version %u\n", (uint32_t)__cplusplus);

	return NULL;
}
ZTEST_SUITE(libcxx_tests, NULL, libcxx_tests_setup, NULL, NULL, NULL);
