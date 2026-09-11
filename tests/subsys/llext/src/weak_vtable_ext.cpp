/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Regression test for virtual dispatch through a C++ vtable with STB_WEAK
 * (rather than STB_GLOBAL) ELF binding.
 *
 * WeakVtableBase below defines all its virtual methods in the class body, so
 * it has no key function. On Xtensa, GCC emits its vtable and methods as weak
 * COMDAT symbols, and does the same for the vtable of a class that has a key
 * function.
 *
 * Note: the classes must have external linkage (not in an anonymous
 * namespace, not `static`) for this to reproduce - internal-linkage classes
 * get STB_LOCAL vtables regardless of key function, which never exercises
 * the bug.
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>
#include "cpp_ext.hpp"

class WeakVtableBase {
public:
	virtual int value() const { return 1; }
};

class WeakVtableDerived : public WeakVtableBase {
public:
	int value() const override { return 42; }
};

extern "C" void test_entry(void)
{
	WeakVtableDerived derived;
	WeakVtableBase *base = opaque(static_cast<WeakVtableBase *>(&derived));

	zassert_equal(base->value(), 42,
		      "virtual call through weak-bound vtable returned %d, expected 42",
		      base->value());
}
EXPORT_SYMBOL(test_entry);
