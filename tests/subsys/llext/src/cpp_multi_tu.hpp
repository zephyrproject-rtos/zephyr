/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LLEXT_TEST_CPP_MULTI_TU_HPP
#define LLEXT_TEST_CPP_MULTI_TU_HPP

class Shape {
public:
	/* Key function: the vtable is emitted only in cpp_multi_tu_ext2.cpp */
	virtual int area() const;
	virtual int sides() const { return 0; }
};

class Square : public Shape {
public:
	explicit Square(int s) : side(s) {}
	int area() const override;
	int sides() const override { return 4; }

private:
	int side;
};

/* No key function: vtable and members are weak copies in both files */
template <typename T> class Box {
public:
	explicit Box(T v) : val(v) {}
	virtual T get() const { return val; }

private:
	T val;
};

template <typename T> __attribute__((noinline)) T twice(T v)
{
	return v * 2;
}

int box_twice_in_ext2(int v);

#endif /* LLEXT_TEST_CPP_MULTI_TU_HPP */
