/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * C++ extension built from two files: virtual dispatch through a vtable
 * defined in the other file, and template code instantiated in both.
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>
#include "cpp_ext.hpp"
#include "cpp_multi_tu.hpp"

extern "C" void test_entry(void)
{
	Square square(3);
	Shape *shape = opaque(static_cast<Shape *>(&square));
	Box<int> box(5);

	zassert_equal(shape->area(), 9, "area() returned %d", shape->area());
	zassert_equal(shape->sides(), 4, "sides() returned %d", shape->sides());
	zassert_equal(twice(opaque(&box)->get()), 10, "Box<int> in ext1 failed");
	zassert_equal(box_twice_in_ext2(7), 14, "Box<int> in ext2 failed");
}
EXPORT_SYMBOL(test_entry);
