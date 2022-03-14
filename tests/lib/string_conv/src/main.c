/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ztest.h>
#include <sys/string_conv.h>

struct input {
	char *str;
	union {
		long l_val;
		unsigned long ul_val;
		struct {
			double val;
			double eps;
		} dbl_val;
	} exp_val;
	bool is_valid;
};

static void test_str2long(void)
{
	long test_val = 0;

	const struct input elems[] = {
		/* Test illegal boundary values */
#if __SIZEOF_LONG__ == 4
		{ .is_valid = false, .str = "-2147483649" },
		{ .is_valid = false, .str = "2147483648" },
#endif
		/* Test illegal huge input */
		{ .is_valid = false, .str = "2147483647000000000000" },
		/* Test corrupt input */
		{ .is_valid = false, .str = "Corrupt" },
		{ .is_valid = false, .str = "1234ac" },
		{ .is_valid = false, .str = "-1234ac" },
		/* Test legal boundary values */
		{ .is_valid = true, .str = "-2147483647", .exp_val.l_val = -2147483647 },
		{ .is_valid = true, .str = "2147483646", .exp_val.l_val = 2147483646 },
		/* Test input corner cases */
		{ .is_valid = true, .str = "-", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "+", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "0", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "+0", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "-0", .exp_val.ul_val = 0 },
		/* Test heading zeros */
		{ .is_valid = true, .str = "0000000001", .exp_val.l_val = 1 },
		{ .is_valid = true, .str = "-0000000001", .exp_val.l_val = -1 },
		/* Test whitespace correction */
		{ .is_valid = true, .str = " -2147483647", .exp_val.l_val = -2147483647 },
		{ .is_valid = true, .str = "2147483646 ", .exp_val.l_val = 2147483646 },
		{ .is_valid = true, .str = " 1", .exp_val.l_val = 1 },
		{ .is_valid = true, .str = " -1    ", .exp_val.l_val = -1 },
		{ .is_valid = false, .str = "         " },
	};

	for (int i = 0; i < ARRAY_SIZE(elems); i++) {
		if (elems[i].is_valid) {
			zassert_true(!string_conv_str2long(elems[i].str, &test_val),
				     "Failed to convert: \"%s\"", elems[i].str);
			zassert_true(test_val == elems[i].exp_val.l_val, "%d != %d", test_val,
				     elems[i].exp_val.l_val);
		} else {
			zassert_false(!string_conv_str2long(elems[i].str, &test_val),
				      "Conversion of \"%s\" did not return expected err",
				      elems[i].str);
		}
	}
}

static void test_str2ulong(void)
{
	unsigned long test_val = 0;

	const struct input elems[] = {
		/* Test illegal boundary values */
		{ .is_valid = false, .str = "-1" },
#if __SIZEOF_LONG__ == 4
		{ .is_valid = false, .str = "4294967296" },
#endif
		/* Test illegal huge input */
		{ .is_valid = false, .str = "4294967295000000000000" },
		/* Test corrupt input */
		{ .is_valid = false, .str = "Corrupt" },
		{ .is_valid = false, .str = "1234ac" },
		{ .is_valid = false, .str = "-1234ac" },
		{ .is_valid = false, .str = "-" },
		/* Test legal boundary values */
		{ .is_valid = true, .str = "0", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "4294967295", .exp_val.ul_val = 4294967295 },
		/* Test input corner cases */
		{ .is_valid = true, .str = "0", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "+0", .exp_val.ul_val = 0 },
		{ .is_valid = true, .str = "+", .exp_val.ul_val = 0 },
		/* Test heading zeros */
		{ .is_valid = true, .str = "0000000001", .exp_val.ul_val = 1 },
		/* Test whitespace correction */
		{ .is_valid = true, .str = "  2147483646 ", .exp_val.ul_val = 2147483646 },
		{ .is_valid = true, .str = " 1", .exp_val.ul_val = 1 },
		{ .is_valid = false, .str = "         " },
	};

	for (int i = 0; i < ARRAY_SIZE(elems); i++) {
		if (elems[i].is_valid) {
			zassert_true(!string_conv_str2ulong(elems[i].str, &test_val),
				     "Failed to convert: \"%s\"", elems[i].str);
			zassert_true(test_val == elems[i].exp_val.ul_val, "%d != %d", test_val,
				     elems[i].exp_val.l_val);
		} else {
			zassert_false(!string_conv_str2ulong(elems[i].str, &test_val),
				      "Conversion of \"%s\" did not return expected err",
				      elems[i].str);
		}
	}
}

bool compare_float(double x, double y, double epsilon)
{
	double temp = x - y;

	temp = temp < 0 ? -temp : temp;
	return (temp < epsilon);
}

static void test_str2dbl(void)
{
#ifdef CONFIG_FPU
	double test_val = 0;

	const struct input elems[] = {
		/* Test illegal boundary values */
#if __SIZEOF_LONG__ == 4
		{ .is_valid = false, .str = "-2147483649" },
		{ .is_valid = false, .str = "2147483648" },
#endif
		/* Test illegal huge input */
		{ .is_valid = false, .str = "4294967295000000000000.1" },
		/* Test corrupt input */
		{ .is_valid = false, .str = "Corrupt" },
		{ .is_valid = false, .str = "1234ac" },
		{ .is_valid = false, .str = "-1234ac" },
		{ .is_valid = false, .str = "321.-123" },
		{ .is_valid = false, .str = "." },
		/* Test legal boundary values */
		{ .is_valid = true, .str = "2147483647", .exp_val.dbl_val.val = 2147483647,
		  .exp_val.dbl_val.eps = 0.1f },
		{ .is_valid = true, .str = "-2147483648", .exp_val.dbl_val.val = -2147483648,
		  .exp_val.dbl_val.eps = 0.1f },
		/* Test precision boundary values */
		{ .is_valid = true, .str = "0.999999999", .exp_val.dbl_val.val = 0.999999999,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = "-0.999999999", .exp_val.dbl_val.val = -0.999999999,
		  .exp_val.dbl_val.eps = 0.000000001f },
		/* Test precision overflow */
		{ .is_valid = true, .str = "-0.9999999995", .exp_val.dbl_val.val = -0.999999999,
		  .exp_val.dbl_val.eps = 0.000000001f },
		/* Test input corner cases */
		{ .is_valid = true, .str = "-.123", .exp_val.dbl_val.val = -0.123,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = ".123", .exp_val.dbl_val.val = 0.123,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = "00.000012", .exp_val.dbl_val.val = 0.000012,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = "58754.", .exp_val.dbl_val.val = 58754,
		  .exp_val.dbl_val.eps = 0.000000001f },
		/* Test whitespace correction */
		{ .is_valid = true, .str = " 0.999999999 ", .exp_val.dbl_val.val = 0.999999999,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = " -0.999999999  ", .exp_val.dbl_val.val = -0.999999999,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = "  -.123 ", .exp_val.dbl_val.val = -0.123,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = true, .str = " 58754.123    ", .exp_val.dbl_val.val = 58754.123,
		  .exp_val.dbl_val.eps = 0.000000001f },
		{ .is_valid = false, .str = "         " },
	};

	for (int i = 0; i < ARRAY_SIZE(elems); i++) {
		if (elems[i].is_valid) {
			zassert_true(!string_conv_str2dbl(elems[i].str, &test_val),
				     "Failed to convert: \"%s\"", elems[i].str);
			zassert_true(compare_float(test_val, elems[i].exp_val.dbl_val.val,
						   elems[i].exp_val.dbl_val.eps),
				     "Float comparison for \"%s\" gave unequal values");
		} else {
			zassert_false(!string_conv_str2dbl(elems[i].str, &test_val),
				      "Conversion of \"%s\" did not return expected err",
				      elems[i].str);
		}
	}
#endif
}

void test_main(void)
{
	ztest_test_suite(lib_string_conv_test,
			 ztest_unit_test(test_str2dbl),
			 ztest_unit_test(test_str2long),
			 ztest_unit_test(test_str2ulong));
	ztest_run_test_suite(lib_string_conv_test);
}
