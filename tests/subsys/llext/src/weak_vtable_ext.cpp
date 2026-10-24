/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Regression test for virtual dispatch through a C++ vtable with STB_WEAK
 * (rather than STB_GLOBAL) ELF binding.
 *
 * Per the Itanium C++ ABI, a class has a "key function" only if it declares
 * at least one non-inline, non-pure virtual method; the compiler then emits
 * the vtable as STB_GLOBAL in that method's translation unit. A class like
 * WeakVtableBase below, whose virtual methods are all defined in the class
 * body, has no key function, so its vtable is emitted STB_WEAK in every
 * translation unit that uses it (to allow safe deduplication). This is the
 * common case, not an edge case: it reproduces any time a C++ llext module
 * dispatches through such a class.
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
