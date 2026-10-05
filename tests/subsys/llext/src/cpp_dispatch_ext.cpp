/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Virtual dispatch with multiple inheritance, which needs secondary vtable
 * pointers and this-adjusting thunks, and through an abstract interface
 * returned by a factory function, the usual shape of a C++ module.
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>
#include "cpp_ext.hpp"

class Left {
public:
	virtual int left() const { return 1; }
};

class Right {
public:
	virtual int right() const { return 2; }
};

class Both : public Left, public Right {
public:
	int left() const override { return 10; }
	int right() const override { return 20; }
};

class Iface {
public:
	virtual int run(int x) const = 0;
	virtual int id() const = 0;
};

class Impl : public Iface {
public:
	int run(int x) const override { return x + 100; }
	int id() const override { return 3; }
};

/* Constant-initialized, so the vtable pointers are relocated data */
static Both global_both;
static Impl impl;

__attribute__((noinline)) Iface *cpp_dispatch_create(void)
{
	return &impl;
}

static void check_both(const Both *both)
{
	const Left *l = opaque(static_cast<const Left *>(both));
	const Right *r = opaque(static_cast<const Right *>(both));

	zassert_not_equal((const void *)r, (const void *)both,
			  "Right should be a non-primary base");
	zassert_equal(l->left(), 10, "left() returned %d", l->left());
	zassert_equal(r->right(), 20, "right() returned %d", r->right());
}

extern "C" void test_entry(void)
{
	Both local_both;
	Iface *iface = opaque(cpp_dispatch_create());

	check_both(&global_both);
	check_both(&local_both);

	zassert_equal(iface->run(1), 101, "run() returned %d", iface->run(1));
	zassert_equal(iface->id(), 3, "id() returned %d", iface->id());
}
EXPORT_SYMBOL(test_entry);
