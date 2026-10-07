/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/math/ilog2.h>

/*
 * ilog2() is intended to be usable in constant expressions such as array
 * sizes. Only the underlying ilog2_compile_time_const_u32() macro is
 * guaranteed to be an integer constant expression, so assert against that.
 */
BUILD_ASSERT(ilog2_compile_time_const_u32(1) == 0);
BUILD_ASSERT(ilog2_compile_time_const_u32(2) == 1);
BUILD_ASSERT(ilog2_compile_time_const_u32(1024) == 10);
BUILD_ASSERT(ilog2_compile_time_const_u32(BIT(31)) == 31);

/* Reference floor(log2()) for a non-zero value, computed independently. */
static uint32_t expected_ilog2(uint32_t value)
{
	uint32_t result = 0;

	while (value > 1U) {
		value >>= 1;
		result++;
	}

	return result;
}

ZTEST(ilog2, test_compile_time_const_macro)
{
	/* Values below 2 are documented to return 0. */
	zassert_equal(0, ilog2_compile_time_const_u32(0));
	zassert_equal(0, ilog2_compile_time_const_u32(1));

	zassert_equal(1, ilog2_compile_time_const_u32(2));
	zassert_equal(1, ilog2_compile_time_const_u32(3));
	zassert_equal(2, ilog2_compile_time_const_u32(4));
	zassert_equal(2, ilog2_compile_time_const_u32(7));
	zassert_equal(3, ilog2_compile_time_const_u32(8));
	zassert_equal(3, ilog2_compile_time_const_u32(15));
	zassert_equal(4, ilog2_compile_time_const_u32(16));
	zassert_equal(7, ilog2_compile_time_const_u32(255));
	zassert_equal(8, ilog2_compile_time_const_u32(256));
	zassert_equal(9, ilog2_compile_time_const_u32(1023));
	zassert_equal(10, ilog2_compile_time_const_u32(1024));
	zassert_equal(30, ilog2_compile_time_const_u32(0x7fffffff));
	zassert_equal(31, ilog2_compile_time_const_u32(0x80000000));
	zassert_equal(31, ilog2_compile_time_const_u32(0xffffffff));
}

ZTEST(ilog2, test_constant_path)
{
	/*
	 * A literal argument selects the compile-time branch of ilog2()
	 * through __builtin_constant_p(); values below 2 return 0.
	 */
	zassert_equal(0, ilog2(0));
	zassert_equal(0, ilog2(1));
	zassert_equal(1, ilog2(2));
	zassert_equal(1, ilog2(3));
	zassert_equal(2, ilog2(4));
	zassert_equal(3, ilog2(8));
	zassert_equal(7, ilog2(255));
	zassert_equal(8, ilog2(256));
	zassert_equal(10, ilog2(1024));
	zassert_equal(30, ilog2(0x7fffffff));
	zassert_equal(31, ilog2(0x80000000));
	zassert_equal(31, ilog2(0xffffffff));
}

ZTEST(ilog2, test_runtime_powers_of_two)
{
	/*
	 * A volatile argument is not a compile-time constant, so ilog2()
	 * takes the runtime find_msb_set() branch. Zero is not tested here:
	 * log2(0) is undefined and the runtime branch does not return the 0
	 * that the compile-time branch does.
	 */
	for (uint32_t i = 0; i < 32; i++) {
		volatile uint32_t value = BIT(i);
		uint32_t result = ilog2(value);

		zassert_equal(i, result, "ilog2(BIT(%u)) = %u", i, result);
	}
}

ZTEST(ilog2, test_runtime_arbitrary)
{
	static const uint32_t values[] = {
		1, 2, 3, 5, 6, 7, 9, 15, 17, 31, 33, 100, 255, 256,
		1000, 1024, 65535, 65536, 0x7fffffff, 0x80000000, 0xffffffff,
	};

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		volatile uint32_t value = values[i];
		uint32_t expected = expected_ilog2(values[i]);
		uint32_t result = ilog2(value);

		zassert_equal(expected, result, "ilog2(%u) = %u, expected %u",
			      values[i], result, expected);
	}
}

ZTEST_SUITE(ilog2, NULL, NULL, NULL, NULL, NULL);
