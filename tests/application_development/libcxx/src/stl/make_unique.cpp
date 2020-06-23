/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <functional>
#include <memory>
#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

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

void test_make_unique(void)
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
