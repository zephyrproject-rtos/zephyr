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
 *
 * Note: GCC emits a "deleting destructor" (D0) for any polymorphic class
 * (one with a vtable), whether or not the destructor itself is declared
 * virtual. Its body calls operator delete(void*, size_t) even though this
 * test never actually calls `delete` on these objects - the symbol still
 * has to resolve at link time. Zephyr's minimal C++ runtime provides that
 * operator (lib/cpp/minimal/cpp_new.cpp) but does not EXPORT_SYMBOL it for
 * llext use, so a self-contained no-op definition is provided below rather
 * than depending on an unrelated, separate llext export gap.
 */

#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>
#include <cstddef>

void operator delete(void *, size_t) noexcept {}

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
	WeakVtableBase *base = &derived;

	zassert_equal(base->value(), 42,
		      "virtual call through weak-bound vtable returned %d, expected 42",
		      base->value());
}
EXPORT_SYMBOL(test_entry);
