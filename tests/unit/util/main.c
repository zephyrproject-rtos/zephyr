/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/util.h>
#include <string.h>

/**
 * @brief Test of u8_to_dec
 *
 * This test verifies conversion of various input values.
 *
 */
static void test_u8_to_dec(void)
{
	char text[4];
	u8_t len;

	len = u8_to_dec(text, sizeof(text), 0);
	zassert_equal(len, 1, "Length of 0 is not 1");
	zassert_equal(strcmp(text, "0"), 0,
		      "Value=0 is not converted to \"0\"");

	len = u8_to_dec(text, sizeof(text), 1);
	zassert_equal(len, 1, "Length of 1 is not 1");
	zassert_equal(strcmp(text, "1"), 0,
		      "Value=1 is not converted to \"1\"");

	len = u8_to_dec(text, sizeof(text), 11);
	zassert_equal(len, 2, "Length of 11 is not 2");
	zassert_equal(strcmp(text, "11"), 0,
		      "Value=10 is not converted to \"11\"");

	len = u8_to_dec(text, sizeof(text), 100);
	zassert_equal(len, 3, "Length of 100 is not 3");
	zassert_equal(strcmp(text, "100"), 0,
		      "Value=100 is not converted to \"100\"");

	len = u8_to_dec(text, sizeof(text), 101);
	zassert_equal(len, 3, "Length of 101 is not 3");
	zassert_equal(strcmp(text, "101"), 0,
		      "Value=101 is not converted to \"101\"");

	len = u8_to_dec(text, sizeof(text), 255);
	zassert_equal(len, 3, "Length of 255 is not 3");
	zassert_equal(strcmp(text, "255"), 0,
		      "Value=255 is not converted to \"255\"");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 2, 123);
	zassert_equal(len, 2,
		      "Length of converted value using 2 byte buffer isn't 2");
	zassert_equal(
		strcmp(text, "12"), 0,
		"Value=123 is not converted to \"12\" using 2-byte buffer");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 1, 123);
	zassert_equal(len, 1,
		      "Length of converted value using 1 byte buffer isn't 1");
	zassert_equal(
		strcmp(text, "1"), 0,
		"Value=123 is not converted to \"1\" using 1-byte buffer");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 0, 123);
	zassert_equal(len, 0,
		      "Length of converted value using 0 byte buffer isn't 0");
}

#define TEST_DEFINE_1 1
#define TEST_DEFINE_0 0

void test_COND_CODE_1(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_1(1, (u32_t x0 = 1;), (u32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_1(NOT_EXISTING_DEFINE, (u32_t x1 = 1;), (u32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_1(TEST_DEFINE_1, (u32_t x2 = 1;), (u32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_1(2, (u32_t x3 = 1;), (u32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

void test_COND_CODE_0(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_0(0, (u32_t x0 = 1;), (u32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_0(NOT_EXISTING_DEFINE, (u32_t x1 = 1;), (u32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_0(TEST_DEFINE_0, (u32_t x2 = 1;), (u32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_0(2, (u32_t x3 = 1;), (u32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

void test_UTIL_LISTIFY(void)
{
	int i = 0;

#define INC(x, _)		\
	do {			\
		i += x;		\
	} while (0);

#define DEFINE(x, _) int a##x;
#define MARK_UNUSED(x, _) ARG_UNUSED(a##x);

	UTIL_LISTIFY(4, DEFINE, _)
	UTIL_LISTIFY(4, MARK_UNUSED, _)

	UTIL_LISTIFY(4, INC, _)

	zassert_equal(i, 0 + 1 + 2 + 3, NULL);
}

static int inc_func(void)
{
	static int a = 1;

	return a++;
}

/* Test checks if @ref Z_MAX and @ref Z_MIN return correct result and perform
 * single evaluation of input arguments.
 */
static void test_z_max_z_min(void)
{
	zassert_equal(Z_MAX(inc_func(), 0), 1, "Unexpected macro result");
	/* Z_MAX should have call inc_func only once */
	zassert_equal(inc_func(), 2, "Unexpected return value");

	zassert_equal(Z_MIN(inc_func(), 2), 2, "Unexpected macro result");
	/* Z_MIN should have call inc_func only once */
	zassert_equal(inc_func(), 4, "Unexpected return value");
}

void test_main(void)
{
	ztest_test_suite(test_lib_sys_util_tests,
			 ztest_unit_test(test_u8_to_dec),
			 ztest_unit_test(test_COND_CODE_1),
			 ztest_unit_test(test_COND_CODE_0),
			 ztest_unit_test(test_UTIL_LISTIFY),
			 ztest_unit_test(test_z_max_z_min)
	);

	ztest_run_test_suite(test_lib_sys_util_tests);
}
