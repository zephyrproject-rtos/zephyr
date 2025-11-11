/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Define symbols with different alignment requirements and verify that LLEXT
 * correctly handles them by testing the runtime address and the contents of
 * each defined symbol.
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/ztest_assert.h>

/*
 * Create constants requesting a specific alignment in memory and with a value
 * that is related but not equal to their alignment requirement.
 */
#define ALIGNED_ENTRY(name, n) \
	 const int name ## _ ## n __aligned(n) = n / 2 + 1

ALIGNED_ENTRY(common, 8);
ALIGNED_ENTRY(common, 16);
ALIGNED_ENTRY(common, 32);
ALIGNED_ENTRY(common, 64);
ALIGNED_ENTRY(common, 128);
ALIGNED_ENTRY(common, 256);
ALIGNED_ENTRY(common, 512);

/*
 * Create similar constants in a set of independent sections to test merging.
 */
#define ALIGNED_SECT_ENTRY(name, n) \
	Z_GENERIC_SECTION(name ## _sect_ ## n) ALIGNED_ENTRY(name, n)

ALIGNED_SECT_ENTRY(independent, 8);
ALIGNED_SECT_ENTRY(independent, 16);
ALIGNED_SECT_ENTRY(independent, 32);
ALIGNED_SECT_ENTRY(independent, 64);
ALIGNED_SECT_ENTRY(independent, 128);
ALIGNED_SECT_ENTRY(independent, 256);
ALIGNED_SECT_ENTRY(independent, 512);

/*
 * Test that each symbol matches its expected value and alignment.
 */
#define ASSERT_ENTRY(name, n) \
	zassert_equal(name ## _ ## n, n / 2 + 1); \
	zassert_true(IS_ALIGNED(&name ## _ ## n, n))

void test_entry(void)
{
	ASSERT_ENTRY(common, 8);
	ASSERT_ENTRY(common, 16);
	ASSERT_ENTRY(common, 32);
	ASSERT_ENTRY(common, 64);
	ASSERT_ENTRY(common, 128);
	ASSERT_ENTRY(common, 256);
	ASSERT_ENTRY(common, 512);

	ASSERT_ENTRY(independent, 8);
	ASSERT_ENTRY(independent, 16);
	ASSERT_ENTRY(independent, 32);
	ASSERT_ENTRY(independent, 64);
	ASSERT_ENTRY(independent, 128);
	ASSERT_ENTRY(independent, 256);
	ASSERT_ENTRY(independent, 512);
}
EXPORT_SYMBOL(test_entry);
