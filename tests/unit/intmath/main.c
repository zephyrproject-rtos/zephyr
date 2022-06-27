/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

/* Built-time math test.  Zephyr code depends on a standard C ABI with
 * 2's complement signed math.  As this isn't technically guaranteed
 * by the compiler or language standard, validate it explicitly here.
 */

/* Recent GCC's can detect integer overflow in static expressions and
 * will warn about it helpfully.  But obviously integer overflow is
 * the whole point here, so turn that warning off.
 */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Woverflow"
#endif

/* Two's complement negation check: "-N" must equal "(~N)+1" */
#define NEG_CHECK(T, N) BUILD_ASSERT((-((T)N)) == (~((T)N)) + 1)

/* Checks that MAX+1==MIN in the given type */
#define ROLLOVER_CHECK(T, MAX, MIN) BUILD_ASSERT((T)((T)1 + (T)MAX) == (T)MIN)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winteger-overflow"
#endif

ROLLOVER_CHECK(unsigned int, 0xffffffff, 0);
ROLLOVER_CHECK(unsigned short, 0xffff, 0);
ROLLOVER_CHECK(unsigned char, 0xff, 0);

NEG_CHECK(signed char, 1);
NEG_CHECK(signed char, 0);
NEG_CHECK(signed char, -1);
NEG_CHECK(signed char, 0x80);
NEG_CHECK(signed char, 0x7f);
ROLLOVER_CHECK(signed char, 127, -128);

NEG_CHECK(short, 1);
NEG_CHECK(short, 0);
NEG_CHECK(short, -1);
NEG_CHECK(short, 0x8000);
NEG_CHECK(short, 0x7fff);
ROLLOVER_CHECK(short, 32767, -32768);

NEG_CHECK(int, 1);
NEG_CHECK(int, 0);
NEG_CHECK(int, -1);
NEG_CHECK(int, 0x80000000);
NEG_CHECK(int, 0x7fffffff);
ROLLOVER_CHECK(int, 2147483647, -2147483648);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test integer arithmetic operations
 *
 * @details Test multiplication and division of two
 * integers
 */
void test_intmath(void)
{
	/*
	 * Declaring volatile so the compiler doesn't try to optimize any
	 * of the math away at build time
	 */
	volatile uint64_t bignum, ba, bb;
	volatile uint32_t num, a, b;

	ba = 0x00000012ABCDEF12ULL;
	bb = 0x0000001000000111ULL;
	bignum = ba * bb;
	zassert_true((bignum == 0xbcdf0509369bf232ULL), "64-bit multiplication failed");

	a = 30000U;
	b = 5872U;
	num = a * b;
	zassert_true((num == 176160000U), "32-bit multiplication failed");

	a = 234424432U;
	b = 98982U;
	num = a / b;
	zassert_true((num == 2368U), "32-bit division failed");
}
/**
 * @}
 */



void test_main(void)
{
	ztest_test_suite(intmath,
			 ztest_unit_test(test_intmath)
			 );

	ztest_run_test_suite(intmath);
}
