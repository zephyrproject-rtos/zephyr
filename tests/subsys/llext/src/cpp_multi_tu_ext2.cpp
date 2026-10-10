/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp_ext.hpp"
#include "cpp_multi_tu.hpp"

int Shape::area() const
{
	return 0;
}

int Square::area() const
{
	return side * side;
}

int box_twice_in_ext2(int v)
{
	Box<int> box(v);

	return twice(opaque(&box)->get());
}
