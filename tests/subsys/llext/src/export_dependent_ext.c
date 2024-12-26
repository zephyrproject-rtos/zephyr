/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test importing symbols, exported by other LLEXTs */

#include <zephyr/ztest.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/util.h>

extern long test_dependency(int a, int b);

void test_entry(void)
{
	unsigned long half_ptr_bits = sizeof(uintptr_t) * 8 / 2;
	unsigned long mask = BIT(half_ptr_bits) - 1;
	int a = mask & (uintptr_t)test_entry;
	int b = mask & ((uintptr_t)test_entry >> half_ptr_bits);

	zassert_equal(test_dependency(a, b), (long)a * b);
}
EXPORT_SYMBOL(test_entry);
