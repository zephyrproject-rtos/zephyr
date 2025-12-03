/*
 * Copyright (c) 2019 Oticon A/S
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_utf8.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

ZTEST(util, test_u8_to_dec) {
	char text[4];
	uint8_t len;

	len = u8_to_dec(text, sizeof(text), 0);
	zassert_equal(len, 1, "Length of 0 is not 1");
	zassert_str_equal(text, "0", "Value=0 is not converted to \"0\"");

	len = u8_to_dec(text, sizeof(text), 1);
	zassert_equal(len, 1, "Length of 1 is not 1");
	zassert_str_equal(text, "1", "Value=1 is not converted to \"1\"");

	len = u8_to_dec(text, sizeof(text), 11);
	zassert_equal(len, 2, "Length of 11 is not 2");
	zassert_str_equal(text, "11", "Value=10 is not converted to \"11\"");

	len = u8_to_dec(text, sizeof(text), 100);
	zassert_equal(len, 3, "Length of 100 is not 3");
	zassert_str_equal(text, "100",
			  "Value=100 is not converted to \"100\"");

	len = u8_to_dec(text, sizeof(text), 101);
	zassert_equal(len, 3, "Length of 101 is not 3");
	zassert_str_equal(text, "101",
			  "Value=101 is not converted to \"101\"");

	len = u8_to_dec(text, sizeof(text), 255);
	zassert_equal(len, 3, "Length of 255 is not 3");
	zassert_str_equal(text, "255",
			  "Value=255 is not converted to \"255\"");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 2, 123);
	zassert_equal(len, 2,
		      "Length of converted value using 2 byte buffer isn't 2");
	zassert_str_equal(text, "12",
			  "Value=123 is not converted to \"12\" using 2-byte buffer");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 1, 123);
	zassert_equal(len, 1,
		      "Length of converted value using 1 byte buffer isn't 1");
	zassert_str_equal(text, "1",
			  "Value=123 is not converted to \"1\" using 1-byte buffer");

	memset(text, 0, sizeof(text));
	len = u8_to_dec(text, 0, 123);
	zassert_equal(len, 0,
		      "Length of converted value using 0 byte buffer isn't 0");
}

ZTEST(util, test_sign_extend) {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;

	u8 = 0x0f;
	zassert_equal(sign_extend(u8, 3), -1);
	zassert_equal(sign_extend(u8, 4), 0xf);

	u16 = 0xfff;
	zassert_equal(sign_extend(u16, 11), -1);
	zassert_equal(sign_extend(u16, 12), 0xfff);

	u32 = 0xfffffff;
	zassert_equal(sign_extend(u32, 27), -1);
	zassert_equal(sign_extend(u32, 28), 0xfffffff);
}

ZTEST(util, test_arithmetic_shift_right)
{
	/* Test positive number */
	zassert_equal(arithmetic_shift_right(0x8, 2), 0x2);
	zassert_equal(arithmetic_shift_right(0x10, 3), 0x2);
	zassert_equal(arithmetic_shift_right(0x20, 4), 0x2);

	/* Test negative number */
	zassert_equal(arithmetic_shift_right(-0x8, 2), -0x2);
	zassert_equal(arithmetic_shift_right(-0x10, 3), -0x2);
	zassert_equal(arithmetic_shift_right(-0x20, 4), -0x2);

	/* Test zero shift */
	zassert_equal(arithmetic_shift_right(0x2A, 0), 0x2A);
	zassert_equal(arithmetic_shift_right(-0x2A, 0), -0x2A);

	/* Test large shifts */
	zassert_equal(arithmetic_shift_right(0x7FFFFFFFFFFFFFFF, 63), 0x0);
	zassert_equal(arithmetic_shift_right(0x8000000000000000, 63), -0x1);
}

ZTEST(util, test_sign_extend_64) {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

	u8 = 0x0f;
	zassert_equal(sign_extend_64(u8, 3), -1);
	zassert_equal(sign_extend_64(u8, 4), 0xf);

	u16 = 0xfff;
	zassert_equal(sign_extend_64(u16, 11), -1);
	zassert_equal(sign_extend_64(u16, 12), 0xfff);

	u32 = 0xfffffff;
	zassert_equal(sign_extend_64(u32, 27), -1);
	zassert_equal(sign_extend_64(u32, 28), 0xfffffff);

	u64 = 0xfffffffffffffff;
	zassert_equal(sign_extend_64(u64, 59), -1);
	zassert_equal(sign_extend_64(u64, 60), 0xfffffffffffffff);
}

ZTEST(util, test_COND_CODE_1) {
	#define TEST_DEFINE_1 1
	#define TEST_DEFINE_0 0
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or unused variable).
	 */
	COND_CODE_1(1, (uint32_t x0 = 1;), (uint32_t y0;))
	zassert_true((x0 == 1));

	COND_CODE_1(NOT_EXISTING_DEFINE, (uint32_t x1 = 1;), (uint32_t y1 = 1;))
	zassert_true((y1 == 1));

	COND_CODE_1(TEST_DEFINE_1, (uint32_t x2 = 1;), (uint32_t y2 = 1;))
	zassert_true((x2 == 1));

	COND_CODE_1(2, (uint32_t x3 = 1;), (uint32_t y3 = 1;))
	zassert_true((y3 == 1));
}

ZTEST(util, test_COND_CODE_0) {
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or unused variable).
	 */
	COND_CODE_0(0, (uint32_t x0 = 1;), (uint32_t y0;))
	zassert_true((x0 == 1));

	COND_CODE_0(NOT_EXISTING_DEFINE, (uint32_t x1 = 1;), (uint32_t y1 = 1;))
	zassert_true((y1 == 1));

	COND_CODE_0(TEST_DEFINE_0, (uint32_t x2 = 1;), (uint32_t y2 = 1;))
	zassert_true((x2 == 1));

	COND_CODE_0(2, (uint32_t x3 = 1;), (uint32_t y3 = 1;))
	zassert_true((y3 == 1));
}

ZTEST(util, test_COND_CASE_1) {
	/* Intentionally undefined symbols used to verify that only the selected
	 * branch expands.
	 */
	int val;

	#define CASE_TRUE 1
	#define CASE_FALSE 0

	val = COND_CASE_1(CASE_TRUE, (42),
			  CASE_TRUE, (COND_CASE_1_SHOULD_NOT_REACH_SECOND_TRUE_CASE),
			  (0));
	zexpect_equal(val, 42);

	val = COND_CASE_1(CASE_FALSE, (COND_CASE_1_SHOULD_NOT_USE_FIRST_CASE),
			  CASE_TRUE, (7),
			  (11));
	zexpect_equal(val, 7);

	val = COND_CASE_1(CASE_FALSE, (COND_CASE_1_SHOULD_NOT_USE_SECOND_CASE),
			  CASE_FALSE, (COND_CASE_1_SHOULD_NOT_USE_THIRD_CASE),
			  (5));
	zexpect_equal(val, 5);

	val = COND_CASE_1((9));
	zexpect_equal(val, 9);

	#undef CASE_TRUE
	#undef CASE_FALSE
}

#undef ZERO
#undef SEVEN
#undef A_BUILD_ERROR
#define ZERO 0
#define SEVEN 7
#define A_BUILD_ERROR (this would be a build error if you used || or &&)
ZTEST(util, test_UTIL_OR) {
	zassert_equal(UTIL_OR(SEVEN, A_BUILD_ERROR), 7);
	zassert_equal(UTIL_OR(7, 0), 7);
	zassert_equal(UTIL_OR(SEVEN, ZERO), 7);
	zassert_equal(UTIL_OR(0, 7), 7);
	zassert_equal(UTIL_OR(ZERO, SEVEN), 7);
	zassert_equal(UTIL_OR(0, 0), 0);
	zassert_equal(UTIL_OR(ZERO, ZERO), 0);
}

ZTEST(util, test_UTIL_AND) {
	zassert_equal(UTIL_AND(ZERO, A_BUILD_ERROR), 0);
	zassert_equal(UTIL_AND(7, 0), 0);
	zassert_equal(UTIL_AND(SEVEN, ZERO), 0);
	zassert_equal(UTIL_AND(0, 7), 0);
	zassert_equal(UTIL_AND(ZERO, SEVEN), 0);
	zassert_equal(UTIL_AND(0, 0), 0);
	zassert_equal(UTIL_AND(ZERO, ZERO), 0);
	zassert_equal(UTIL_AND(7, 7), 7);
	zassert_equal(UTIL_AND(7, SEVEN), 7);
	zassert_equal(UTIL_AND(SEVEN, 7), 7);
	zassert_equal(UTIL_AND(SEVEN, SEVEN), 7);
}

ZTEST(util, test_IF_ENABLED) {
	#define test_IF_ENABLED_FLAG_A 1
	#define test_IF_ENABLED_FLAG_B 0

	IF_ENABLED(test_IF_ENABLED_FLAG_A, (goto skipped;))
	/* location should be skipped if IF_ENABLED macro is correct. */
	zassert_false(true, "location should be skipped");
skipped:
	IF_ENABLED(test_IF_ENABLED_FLAG_B, (zassert_false(true, "");))

	IF_ENABLED(test_IF_ENABLED_FLAG_C, (zassert_false(true, "");))

	zassert_true(true, "");

	#undef test_IF_ENABLED_FLAG_A
	#undef test_IF_ENABLED_FLAG_B
}

ZTEST(util, test_LISTIFY) {
	int ab0 = 1;
	int ab1 = 1;
#define A_PTR(x, name0, name1) &UTIL_CAT(UTIL_CAT(name0, name1), x)

	int *a[] = { LISTIFY(2, A_PTR, (,), a, b) };

	zassert_equal(ARRAY_SIZE(a), 2);
	zassert_equal(a[0], &ab0);
	zassert_equal(a[1], &ab1);
}

ZTEST(util, test_MACRO_MAP_CAT) {
	int item_a_item_b_item_c_ = 1;

#undef FOO
#define FOO(x) item_##x##_
	zassert_equal(MACRO_MAP_CAT(FOO, a, b, c), 1, "MACRO_MAP_CAT");
#undef FOO
}

static int inc_func(bool cleanup)
{
	static int a;

	if (cleanup) {
		a = 1;
	}

	return a++;
}

/* Test checks if @ref max, @ref min and @ref clamp return correct result
 * and perform single evaluation of input arguments.
 */
ZTEST(util, test_max_min_clamp) {
#ifdef __cplusplus
	/* C++ has its own std::min() and std::max(), min and max are not available. */
	ztest_test_skip();
#else
	zassert_equal(max(inc_func(true), 0), 1, "Unexpected macro result");
	/* max should have call inc_func only once */
	zassert_equal(inc_func(false), 2, "Unexpected return value");

	zassert_equal(min(inc_func(false), 2), 2, "Unexpected macro result");
	/* min should have call inc_func only once */
	zassert_equal(inc_func(false), 4, "Unexpected return value");

	zassert_equal(clamp(inc_func(false), 1, 3), 3, "Unexpected macro result");
	/* clamp should have call inc_func only once */
	zassert_equal(inc_func(false), 6, "Unexpected return value");

	zassert_equal(clamp(inc_func(false), 10, 15), 10,
		      "Unexpected macro result");
	/* clamp should have call inc_func only once */
	zassert_equal(inc_func(false), 8, "Unexpected return value");

	/* Nested calls do not generate build warnings */
	zassert_equal(max(inc_func(false),
			  max(inc_func(false),
			      min(inc_func(false),
				  inc_func(false)))), 11, "Unexpected macro result");
	zassert_equal(inc_func(false), 13, "Unexpected return value");
#endif
}

ZTEST(util, test_max3_min3) {
	/* check for single call */
	zassert_equal(max3(inc_func(true), 0, 0), 1, "Unexpected macro result");
	zassert_equal(inc_func(false), 2, "Unexpected return value");

	zassert_equal(min3(inc_func(false), 9, 10), 3, "Unexpected macro result");
	zassert_equal(inc_func(false), 4, "Unexpected return value");

	/* test the general functionality */
	zassert_equal(max3(1, 2, 3), 3, "Unexpected macro result");
	zassert_equal(max3(3, 1, 2), 3, "Unexpected macro result");
	zassert_equal(max3(2, 3, 1), 3, "Unexpected macro result");
	zassert_equal(max3(-1, 0, 1), 1, "Unexpected macro result");

	zassert_equal(min3(1, 2, 3), 1, "Unexpected macro result");
	zassert_equal(min3(3, 1, 2), 1, "Unexpected macro result");
	zassert_equal(min3(2, 3, 1), 1, "Unexpected macro result");
	zassert_equal(min3(-1, 0, 1), -1, "Unexpected macro result");
}

ZTEST(util, test_max_from_list_macro) {
	/* Test with one argument */
	zassert_equal(MAX_FROM_LIST(10), 10, "Should return the single value.");

	/* Test with two arguments */
	zassert_equal(MAX_FROM_LIST(10, 20), 20, "Should return 20.");
	zassert_equal(MAX_FROM_LIST(30, 15), 30, "Should return 30.");

	/* Test with three arguments */
	zassert_equal(MAX_FROM_LIST(10, 5, 20), 20, "Should return 20.");
	zassert_equal(MAX_FROM_LIST(30, 15, 25), 30, "Should return 30.");
	zassert_equal(MAX_FROM_LIST(5, 40, 35), 40, "Should return 40.");

	/* Test with five arguments */
	zassert_equal(MAX_FROM_LIST(10, 50, 20, 5, 30), 50, "Should return 50.");

	/* Test with seven arguments */
	zassert_equal(MAX_FROM_LIST(10, 50, 20, 5, 30, 45, 25), 50, "Should return 50.");

	/* Test with eight arguments */
	zassert_equal(MAX_FROM_LIST(1, 2, 3, 4, 5, 6, 7, 8), 8, "Should return 8.");
	zassert_equal(MAX_FROM_LIST(10, 5, 20, 15, 30, 25, 35, 40), 40, "Should return 40.");

	/* Test with nine arguments */
	zassert_equal(MAX_FROM_LIST(1, 2, 3, 4, 5, 6, 7, 8, 9), 9, "Should return 9.");
	zassert_equal(MAX_FROM_LIST(10, 5, 20, 15, 30, 25, 35, 40, 45), 45, "Should return 45.");

	/* Test with ten arguments */
	zassert_equal(MAX_FROM_LIST(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), 10, "Should return 10.");
	zassert_equal(MAX_FROM_LIST(10, 9, 8, 7, 6, 5, 4, 3, 2, 1), 10, "Should return 10.");
	zassert_equal(MAX_FROM_LIST(5, 15, 25, 35, 45, 55, 65, 75, 85, 95),
		95, "Should return 95.");

	/* Test with various values */
	zassert_equal(MAX_FROM_LIST(25600, 12800, 9800), 25600, "Should return 25600.");
	zassert_equal(MAX_FROM_LIST(9800, 25600, 12800), 25600, "Should return 25600.");
}

ZTEST(util, test_CLAMP) {
	zassert_equal(CLAMP(5, 3, 7), 5, "Unexpected clamp result");
	zassert_equal(CLAMP(3, 3, 7), 3, "Unexpected clamp result");
	zassert_equal(CLAMP(7, 3, 7), 7, "Unexpected clamp result");
	zassert_equal(CLAMP(1, 3, 7), 3, "Unexpected clamp result");
	zassert_equal(CLAMP(8, 3, 7), 7, "Unexpected clamp result");

	zassert_equal(CLAMP(-5, -7, -3), -5, "Unexpected clamp result");
	zassert_equal(CLAMP(-9, -7, -3), -7, "Unexpected clamp result");
	zassert_equal(CLAMP(1, -7, -3), -3, "Unexpected clamp result");

	zassert_equal(CLAMP(0xffffffffaULL, 0xffffffff0ULL, 0xfffffffffULL),
		      0xffffffffaULL, "Unexpected clamp result");
}

ZTEST(util, test_IN_RANGE) {
	zassert_true(IN_RANGE(0, 0, 0), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(1, 0, 1), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(1, 0, 2), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(-1, -2, 2), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(-3, -5, -1), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(0, 0, UINT64_MAX), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(UINT64_MAX, 0, UINT64_MAX), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(0, INT64_MIN, INT64_MAX), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(INT64_MIN, INT64_MIN, INT64_MAX), "Unexpected IN_RANGE result");
	zassert_true(IN_RANGE(INT64_MAX, INT64_MIN, INT64_MAX), "Unexpected IN_RANGE result");

	zassert_false(IN_RANGE(5, 0, 2), "Unexpected IN_RANGE result");
	zassert_false(IN_RANGE(5, 10, 0), "Unexpected IN_RANGE result");
	zassert_false(IN_RANGE(-1, 0, 1), "Unexpected IN_RANGE result");
}

ZTEST(util, test_FOR_EACH) {
	#define FOR_EACH_MACRO_TEST(arg) *buf++ = arg
	#define FOR_EACH_MACRO_TEST2(arg) arg

	uint8_t array[3] = {0};
	uint8_t *buf = array;

	FOR_EACH(FOR_EACH_MACRO_TEST, (;), 1, 2, 3);

	zassert_equal(array[0], 1, "Unexpected value %d", array[0]);
	zassert_equal(array[1], 2, "Unexpected value %d", array[1]);
	zassert_equal(array[2], 3, "Unexpected value %d", array[2]);

	uint8_t test0[] = { 0, FOR_EACH(FOR_EACH_MACRO_TEST2, (,))};

	BUILD_ASSERT(sizeof(test0) == 1, "Unexpected length due to FOR_EACH fail");

	uint8_t test1[] = { 0, FOR_EACH(FOR_EACH_MACRO_TEST2, (,), 1)};

	BUILD_ASSERT(sizeof(test1) == 2, "Unexpected length due to FOR_EACH fail");
}

ZTEST(util, test_FOR_EACH_NONEMPTY_TERM) {
	#define SQUARE(arg) (arg * arg)
	#define SWALLOW_VA_ARGS_1(...) EMPTY
	#define SWALLOW_VA_ARGS_2(...)
	#define REPEAT_VA_ARGS(...) __VA_ARGS__

	uint8_t array[] = {
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,))
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,),)
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), ,)
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), EMPTY, EMPTY)
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), SWALLOW_VA_ARGS_1(a, b))
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), SWALLOW_VA_ARGS_2(c, d))
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), 1)
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), 2, 3)
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), REPEAT_VA_ARGS(4))
		FOR_EACH_NONEMPTY_TERM(SQUARE, (,), REPEAT_VA_ARGS(5, 6))
		255
	};

	size_t size = ARRAY_SIZE(array);

	zassert_equal(size, 7, "Unexpected size %d", size);
	zassert_equal(array[0], 1, "Unexpected value %d", array[0]);
	zassert_equal(array[1], 4, "Unexpected value %d", array[1]);
	zassert_equal(array[2], 9, "Unexpected value %d", array[2]);
	zassert_equal(array[3], 16, "Unexpected value %d", array[3]);
	zassert_equal(array[4], 25, "Unexpected value %d", array[4]);
	zassert_equal(array[5], 36, "Unexpected value %d", array[5]);
	zassert_equal(array[6], 255, "Unexpected value %d", array[6]);
}

static void fsum(uint32_t incr, uint32_t *sum)
{
	*sum = *sum + incr;
}

ZTEST(util, test_FOR_EACH_FIXED_ARG) {
	uint32_t sum = 0;

	FOR_EACH_FIXED_ARG(fsum, (;), &sum, 1, 2, 3);

	zassert_equal(sum, 6, "Unexpected value %d", sum);
}

ZTEST(util, test_FOR_EACH_IDX) {
	#define FOR_EACH_IDX_MACRO_TEST(n, arg) uint8_t a##n = arg

	FOR_EACH_IDX(FOR_EACH_IDX_MACRO_TEST, (;), 1, 2, 3);

	zassert_equal(a0, 1, "Unexpected value %d", a0);
	zassert_equal(a1, 2, "Unexpected value %d", a1);
	zassert_equal(a2, 3, "Unexpected value %d", a2);

	#define FOR_EACH_IDX_MACRO_TEST2(n, arg) array[n] = arg
	uint8_t array[32] = {0};

	FOR_EACH_IDX(FOR_EACH_IDX_MACRO_TEST2, (;), 1, 2, 3, 4, 5, 6, 7, 8,
						9, 10, 11, 12, 13, 14, 15);
	for (int i = 0; i < 15; i++) {
		zassert_equal(array[i], i + 1,
				"Unexpected value: %d", array[i]);
	}
	zassert_equal(array[15], 0, "Unexpected value: %d", array[15]);

	#define FOR_EACH_IDX_MACRO_TEST3(n, arg) &a##n

	uint8_t *a[] = {
		FOR_EACH_IDX(FOR_EACH_IDX_MACRO_TEST3, (,), 1, 2, 3)
	};

	zassert_equal(ARRAY_SIZE(a), 3, "Unexpected value:%zu", ARRAY_SIZE(a));
}

ZTEST(util, test_FOR_EACH_IDX_FIXED_ARG) {
	#undef FOO
	#define FOO(n, arg, fixed_arg) \
		uint8_t fixed_arg##n = arg

	FOR_EACH_IDX_FIXED_ARG(FOO, (;), a, 1, 2, 3);

	zassert_equal(a0, 1, "Unexpected value %d", a0);
	zassert_equal(a1, 2, "Unexpected value %d", a1);
	zassert_equal(a2, 3, "Unexpected value %d", a2);
}

ZTEST(util, test_IS_EMPTY) {
	#define test_IS_EMPTY_REAL_EMPTY
	#define test_IS_EMPTY_NOT_EMPTY XXX_DO_NOT_REPLACE_XXX
	zassert_true(IS_EMPTY(test_IS_EMPTY_REAL_EMPTY),
		     "Expected to be empty");
	zassert_false(IS_EMPTY(test_IS_EMPTY_NOT_EMPTY),
		      "Expected to be non-empty");
	zassert_false(IS_EMPTY("string"),
		      "Expected to be non-empty");
	zassert_false(IS_EMPTY(&test_IS_EMPTY),
		      "Expected to be non-empty");
}

ZTEST(util, test_IS_EQ) {
	zassert_true(IS_EQ(0, 0), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(1, 1), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(7, 7), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(0U, 0U), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(1U, 1U), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(7U, 7U), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(1, 1U), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(1U, 1), "Unexpected IS_EQ result");

	zassert_false(IS_EQ(0, 1), "Unexpected IS_EQ result");
	zassert_false(IS_EQ(1, 7), "Unexpected IS_EQ result");
	zassert_false(IS_EQ(7, 0), "Unexpected IS_EQ result");
}

ZTEST(util, test_LIST_DROP_EMPTY) {
	/*
	 * The real definition should be:
	 *  #define TEST_BROKEN_LIST ,Henry,,Dorsett,Case,
	 * but checkpatch complains, so below equivalent is defined.
	 */
	#define TEST_BROKEN_LIST EMPTY, Henry, EMPTY, Dorsett, Case,
	#define TEST_FIXED_LIST LIST_DROP_EMPTY(TEST_BROKEN_LIST)
	static const char *const arr[] = {
		FOR_EACH(STRINGIFY, (,), TEST_FIXED_LIST)
	};

	zassert_equal(ARRAY_SIZE(arr), 3, "Failed to cleanup list");
	zassert_str_equal(arr[0], "Henry", "Failed at 0");
	zassert_str_equal(arr[1], "Dorsett", "Failed at 1");
	zassert_str_equal(arr[2], "Case", "Failed at 0");
}

ZTEST(util, test_nested_FOR_EACH) {
	#define FOO_1(x) a##x = x
	#define FOO_2(x) int x

	FOR_EACH(FOO_2, (;), FOR_EACH(FOO_1, (,), 0, 1, 2));

	zassert_equal(a0, 0);
	zassert_equal(a1, 1);
	zassert_equal(a2, 2);
}

#define TWO 2 /* to showcase that GET_ARG_N and GET_ARGS_LESS_N also work with macros */

ZTEST(util, test_GET_ARG_N) {
	int a = GET_ARG_N(1, 10, 100, 1000);
	int b = GET_ARG_N(2, 10, 100, 1000);
	int c = GET_ARG_N(3, 10, 100, 1000);
	int d = GET_ARG_N(TWO, 10, 100, 1000);

	zassert_equal(a, 10);
	zassert_equal(b, 100);
	zassert_equal(c, 1000);
	zassert_equal(d, 100);
}

ZTEST(util, test_GET_ARGS_LESS_N) {
	uint8_t a[] = { GET_ARGS_LESS_N(0, 1, 2, 3) };
	uint8_t b[] = { GET_ARGS_LESS_N(1, 1, 2, 3) };
	uint8_t c[] = { GET_ARGS_LESS_N(2, 1, 2, 3) };
	uint8_t d[] = { GET_ARGS_LESS_N(TWO, 1, 2, 3) };

	zassert_equal(sizeof(a), 3);

	zassert_equal(sizeof(b), 2);
	zassert_equal(b[0], 2);
	zassert_equal(b[1], 3);

	zassert_equal(sizeof(c), 1);
	zassert_equal(c[0], 3);

	zassert_equal(sizeof(d), 1);
	zassert_equal(d[0], 3);
}

ZTEST(util, test_mixing_GET_ARG_and_FOR_EACH) {
	#undef TEST_MACRO
	#define TEST_MACRO(x) x,
	int i;

	i = GET_ARG_N(3, FOR_EACH(TEST_MACRO, (), 1, 2, 3, 4, 5));
	zassert_equal(i, 3);

	i = GET_ARG_N(2, 1, GET_ARGS_LESS_N(2, 1, 2, 3, 4, 5));
	zassert_equal(i, 3);

	#undef TEST_MACRO
	#undef TEST_MACRO2
	#define TEST_MACRO(x) GET_ARG_N(3, 1, 2, x),
	#define TEST_MACRO2(...) FOR_EACH(TEST_MACRO, (), __VA_ARGS__)
	int a[] = {
		LIST_DROP_EMPTY(TEST_MACRO2(1, 2, 3, 4)), 5
	};

	zassert_equal(ARRAY_SIZE(a), 5);
	zassert_equal(a[0], 1);
	zassert_equal(a[1], 2);
	zassert_equal(a[2], 3);
	zassert_equal(a[3], 4);
	zassert_equal(a[4], 5);
}

ZTEST(util, test_IS_ARRAY_ELEMENT)
{
	size_t i;
	size_t array[3];
	uint8_t *const alias = (uint8_t *)array;

	zassert_false(IS_ARRAY_ELEMENT(array, &array[-1]));
	zassert_false(IS_ARRAY_ELEMENT(array, &array[ARRAY_SIZE(array)]));
	zassert_false(IS_ARRAY_ELEMENT(array, &alias[1]));

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_true(IS_ARRAY_ELEMENT(array, &array[i]));
	}
}

ZTEST(util, test_ARRAY_INDEX)
{
	size_t i;
	size_t array[] = {0, 1, 2, 3};

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_equal(array[ARRAY_INDEX(array, &array[i])], i);
	}
}

ZTEST(util, test_ARRAY_FOR_EACH)
{
	size_t j = -1;
	size_t array[3];

	ARRAY_FOR_EACH(array, i) {
		j = i + 1;
	}

	zassert_equal(j, ARRAY_SIZE(array));
}

ZTEST(util, test_ARRAY_FOR_EACH_PTR)
{
	size_t j = 0;
	size_t array[3];
	size_t *ptr[3];

	ARRAY_FOR_EACH_PTR(array, p) {
		ptr[j] = p;
		++j;
	}

	zassert_equal(ptr[0], &array[0]);
	zassert_equal(ptr[1], &array[1]);
	zassert_equal(ptr[2], &array[2]);
}

ZTEST(util, test_PART_OF_ARRAY)
{
	size_t i;
	size_t array[3];
	uint8_t *const alias = (uint8_t *)array;

	ARG_UNUSED(i);
	ARG_UNUSED(alias);

	zassert_false(PART_OF_ARRAY(array, &array[-1]));
	zassert_false(PART_OF_ARRAY(array, &array[ARRAY_SIZE(array)]));

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_true(PART_OF_ARRAY(array, &array[i]));
	}

	zassert_true(PART_OF_ARRAY(array, &alias[1]));
}

ZTEST(util, test_ARRAY_INDEX_FLOOR)
{
	size_t i;
	size_t array[] = {0, 1, 2, 3};
	uint8_t *const alias = (uint8_t *)array;

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_equal(array[ARRAY_INDEX_FLOOR(array, &array[i])], i);
	}

	zassert_equal(array[ARRAY_INDEX_FLOOR(array, &alias[1])], 0);
}

ZTEST(util, test_BIT_MASK)
{
	uint32_t bitmask0 = BIT_MASK(0);
	uint32_t bitmask1 = BIT_MASK(1);
	uint32_t bitmask2 = BIT_MASK(2);
	uint32_t bitmask31 = BIT_MASK(31);

	zassert_equal(0x00000000UL, bitmask0);
	zassert_equal(0x00000001UL, bitmask1);
	zassert_equal(0x00000003UL, bitmask2);
	zassert_equal(0x7ffffffFUL, bitmask31);
}

ZTEST(util, test_BIT_MASK64)
{
	uint64_t bitmask0 = BIT64_MASK(0);
	uint64_t bitmask1 = BIT64_MASK(1);
	uint64_t bitmask2 = BIT64_MASK(2);
	uint64_t bitmask63 = BIT64_MASK(63);

	zassert_equal(0x0000000000000000ULL, bitmask0);
	zassert_equal(0x0000000000000001ULL, bitmask1);
	zassert_equal(0x0000000000000003ULL, bitmask2);
	zassert_equal(0x7fffffffffffffffULL, bitmask63);
}

ZTEST(util, test_IS_BIT_MASK)
{
	uint32_t zero32 = 0UL;
	uint64_t zero64 = 0ULL;
	uint32_t bitmask1 = 0x00000001UL;
	uint32_t bitmask2 = 0x00000003UL;
	uint32_t bitmask31 = 0x7fffffffUL;
	uint32_t bitmask32 = 0xffffffffUL;
	uint64_t bitmask63 = 0x7fffffffffffffffULL;
	uint64_t bitmask64 = 0xffffffffffffffffULL;

	uint32_t not_bitmask32 = 0xfffffffeUL;
	uint64_t not_bitmask64 = 0xfffffffffffffffeULL;

	zassert_true(IS_BIT_MASK(zero32));
	zassert_true(IS_BIT_MASK(zero64));
	zassert_true(IS_BIT_MASK(bitmask1));
	zassert_true(IS_BIT_MASK(bitmask2));
	zassert_true(IS_BIT_MASK(bitmask31));
	zassert_true(IS_BIT_MASK(bitmask32));
	zassert_true(IS_BIT_MASK(bitmask63));
	zassert_true(IS_BIT_MASK(bitmask64));
	zassert_false(IS_BIT_MASK(not_bitmask32));
	zassert_false(IS_BIT_MASK(not_bitmask64));

	zassert_true(IS_BIT_MASK(0));
	zassert_true(IS_BIT_MASK(0x00000001UL));
	zassert_true(IS_BIT_MASK(0x00000003UL));
	zassert_true(IS_BIT_MASK(0x7fffffffUL));
	zassert_true(IS_BIT_MASK(0xffffffffUL));
	zassert_true(IS_BIT_MASK(0x7fffffffffffffffUL));
	zassert_true(IS_BIT_MASK(0xffffffffffffffffUL));
	zassert_false(IS_BIT_MASK(0xfffffffeUL));
	zassert_false(IS_BIT_MASK(0xfffffffffffffffeULL));
	zassert_false(IS_BIT_MASK(0x00000002UL));
	zassert_false(IS_BIT_MASK(0x8000000000000000ULL));
}

ZTEST(util, test_IS_SHIFTED_BIT_MASK)
{
	uint32_t bitmask32_shift1 = 0xfffffffeUL;
	uint32_t bitmask32_shift31 = 0x80000000UL;
	uint64_t bitmask64_shift1 =  0xfffffffffffffffeULL;
	uint64_t bitmask64_shift63 = 0x8000000000000000ULL;

	zassert_true(IS_SHIFTED_BIT_MASK(bitmask32_shift1, 1));
	zassert_true(IS_SHIFTED_BIT_MASK(bitmask32_shift31, 31));
	zassert_true(IS_SHIFTED_BIT_MASK(bitmask64_shift1, 1));
	zassert_true(IS_SHIFTED_BIT_MASK(bitmask64_shift63, 63));

	zassert_true(IS_SHIFTED_BIT_MASK(0xfffffffeUL, 1));
	zassert_true(IS_SHIFTED_BIT_MASK(0xfffffffffffffffeULL, 1));
	zassert_true(IS_SHIFTED_BIT_MASK(0x80000000UL, 31));
	zassert_true(IS_SHIFTED_BIT_MASK(0x8000000000000000ULL, 63));
}

ZTEST(util, test_DIV_ROUND_UP)
{
	zassert_equal(DIV_ROUND_UP(0, 1), 0);
	zassert_equal(DIV_ROUND_UP(1, 2), 1);
	zassert_equal(DIV_ROUND_UP(3, 2), 2);
}

ZTEST(util, test_DIV_ROUND_CLOSEST)
{
	zassert_equal(DIV_ROUND_CLOSEST(0, 1), 0);
	/* 5 / 2 = 2.5 -> 3 */
	zassert_equal(DIV_ROUND_CLOSEST(5, 2), 3);
	zassert_equal(DIV_ROUND_CLOSEST(5, -2), -3);
	zassert_equal(DIV_ROUND_CLOSEST(-5, 2), -3);
	zassert_equal(DIV_ROUND_CLOSEST(-5, -2), 3);
	/* 7 / 3 = 2.(3) -> 2 */
	zassert_equal(DIV_ROUND_CLOSEST(7, 3), 2);
	zassert_equal(DIV_ROUND_CLOSEST(-7, 3), -2);
}

ZTEST(util, test_IF_DISABLED)
{
	#define test_IF_DISABLED_FLAG_A 0
	#define test_IF_DISABLED_FLAG_B 1

	IF_DISABLED(test_IF_DISABLED_FLAG_A, (goto skipped_a;))
	/* location should be skipped if IF_DISABLED macro is correct. */
	zassert_false(true, "location A should be skipped");
skipped_a:
	IF_DISABLED(test_IF_DISABLED_FLAG_B, (zassert_false(true, "");))

	IF_DISABLED(test_IF_DISABLED_FLAG_C, (goto skipped_c;))
	/* location should be skipped if IF_DISABLED macro is correct. */
	zassert_false(true, "location C should be skipped");
skipped_c:

	zassert_true(true, "");

	#undef test_IF_DISABLED_FLAG_A
	#undef test_IF_DISABLED_FLAG_B
}

ZTEST(util, test_bytecpy)
{
	/* Test basic byte-by-byte copying */
	uint8_t src1[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
	uint8_t dst1[16] = {0};
	uint8_t expected1[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
				 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

	bytecpy(dst1, src1, sizeof(src1));
	zassert_mem_equal(dst1, expected1, sizeof(expected1), "Basic byte-by-byte copy failed");

	/* Test copying with different sizes */
	uint8_t src2[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
	uint8_t dst2[8] = {0};
	uint8_t expected2[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

	bytecpy(dst2, src2, sizeof(src2));
	zassert_mem_equal(dst2, expected2, sizeof(expected2), "Copy with different size failed");

	/* Test copying with zero size */
	uint8_t src3[4] = {0x11, 0x22, 0x33, 0x44};
	uint8_t dst3[4] = {0xAA, 0xBB, 0xCC, 0xDD};
	uint8_t expected3[4] = {0xAA, 0xBB, 0xCC, 0xDD}; /* Should remain unchanged */

	bytecpy(dst3, src3, 0);
	zassert_mem_equal(dst3, expected3, sizeof(expected3),
			  "Zero size copy should not modify destination");

	/* Test copying with overlapping memory regions */
	uint8_t buf[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint8_t expected4[8] = {0x11, 0x22, 0x33, 0x44, 0x11, 0x22, 0x33, 0x44};

	bytecpy(&buf[4], buf, 4); /* Copy first 4 bytes to last 4 bytes */
	zassert_mem_equal(buf, expected4, sizeof(expected4), "Overlapping memory copy failed");
}

ZTEST(util, test_byteswp)
{
	uint8_t a1 = 0xAAU;
	uint8_t b1 = 0x55U;
	uint32_t a2 = 0x12345678U;
	uint32_t b2 = 0xABCDEF00U;

	byteswp(&a1, &b1, sizeof(a1));
	zassert_equal(a1, 0x55U, "Failed to swap single bytes");
	zassert_equal(b1, 0xAAU, "Failed to swap single bytes");
	byteswp(&a1, &b1, 0);
	zassert_equal(a1, 0x55U, "Zero size swap should not modify values");
	zassert_equal(b1, 0xAAU, "Zero size swap should not modify values");

	byteswp(&a2, &b2, sizeof(a2));
	zassert_equal(a2, 0xABCDEF00U, "Failed to swap multiple bytes");
	zassert_equal(b2, 0x12345678U, "Failed to swap multiple bytes");
}

ZTEST(util, test_mem_xor_n)
{
	const size_t max_len = 128;
	uint8_t expected_result[max_len];
	uint8_t src1[max_len];
	uint8_t src2[max_len];
	uint8_t dst[max_len];

	memset(expected_result, 0, sizeof(expected_result));
	memset(src1, 0, sizeof(src1));
	memset(src2, 0, sizeof(src2));
	memset(dst, 0, sizeof(dst));

	for (size_t i = 0U; i < max_len; i++) {
		const size_t len = i;

		for (size_t j = 0U; j < len; j++) {
			src1[j] = 0x33;
			src2[j] = 0x0F;
			expected_result[j] = 0x3C;
		}

		mem_xor_n(dst, src1, src2, len);
		zassert_mem_equal(expected_result, dst, len);
	}
}

ZTEST(util, test_mem_xor_32)
{
	uint8_t expected_result[4];
	uint8_t src1[4];
	uint8_t src2[4];
	uint8_t dst[4];

	memset(expected_result, 0, sizeof(expected_result));
	memset(src1, 0, sizeof(src1));
	memset(src2, 0, sizeof(src2));
	memset(dst, 0, sizeof(dst));

	for (size_t i = 0U; i < 4; i++) {
		src1[i] = 0x43;
		src2[i] = 0x0F;
		expected_result[i] = 0x4C;
	}

	mem_xor_32(dst, src1, src2);
	zassert_mem_equal(expected_result, dst, 4);
}

ZTEST(util, test_mem_xor_128)
{
	uint8_t expected_result[16];
	uint8_t src1[16];
	uint8_t src2[16];
	uint8_t dst[16];

	memset(expected_result, 0, sizeof(expected_result));
	memset(src1, 0, sizeof(src1));
	memset(src2, 0, sizeof(src2));
	memset(dst, 0, sizeof(dst));

	for (size_t i = 0U; i < 16; i++) {
		src1[i] = 0x53;
		src2[i] = 0x0F;
		expected_result[i] = 0x5C;
	}

	mem_xor_128(dst, src1, src2);
	zassert_mem_equal(expected_result, dst, 16);
}

ZTEST(util, test_sys_count_bits)
{
	uint8_t zero = 0U;
	uint8_t u8 = 29U;
	uint16_t u16 = 29999U;
	uint32_t u32 = 2999999999U;
	uint64_t u64 = 123456789012345ULL;
	uint8_t u8_arr[] = {u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
			    u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8};

	zassert_equal(sys_count_bits(&zero, sizeof(zero)), 0);
	zassert_equal(sys_count_bits(&u8, sizeof(u8)), 4);
	zassert_equal(sys_count_bits(&u16, sizeof(u16)), 10);
	zassert_equal(sys_count_bits(&u32, sizeof(u32)), 20);
	zassert_equal(sys_count_bits(&u64, sizeof(u64)), 23);

	zassert_equal(sys_count_bits(u8_arr, sizeof(u8_arr)), 128);
	zassert_equal(sys_count_bits(&u8_arr[1], sizeof(u8_arr) - sizeof(u8_arr[0])), 124);
}

ZTEST(util, test_CONCAT)
{
#define _CAT_PART1 1
#define CAT_PART1 _CAT_PART1
#define _CAT_PART2 2
#define CAT_PART2 _CAT_PART2
#define _CAT_PART3 3
#define CAT_PART3 _CAT_PART3
#define _CAT_PART4 4
#define CAT_PART4 _CAT_PART4
#define _CAT_PART5 5
#define CAT_PART5 _CAT_PART5
#define _CAT_PART6 6
#define CAT_PART6 _CAT_PART6
#define _CAT_PART7 7
#define CAT_PART7 _CAT_PART7
#define _CAT_PART8 8
#define CAT_PART8 _CAT_PART8

	zassert_equal(CONCAT(CAT_PART1), 1);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2), 12);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3), 123);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3, CAT_PART4), 1234);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3, CAT_PART4, CAT_PART5), 12345);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3, CAT_PART4, CAT_PART5, CAT_PART6),
			123456);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3, CAT_PART4,
			     CAT_PART5, CAT_PART6, CAT_PART7),
			1234567);
	zassert_equal(CONCAT(CAT_PART1, CAT_PART2, CAT_PART3, CAT_PART4,
			     CAT_PART5, CAT_PART6, CAT_PART7, CAT_PART8),
			12345678);

	zassert_equal(CONCAT(CAT_PART1, CONCAT(CAT_PART2, CAT_PART3)), 123);
}

ZTEST(util, test_SIZEOF_FIELD)
{
	struct test_t {
		uint32_t a;
		uint8_t b;
		uint8_t c[17];
		int16_t d;
	};

	BUILD_ASSERT(SIZEOF_FIELD(struct test_t, a) == 4, "The a member is 4-byte wide.");
	BUILD_ASSERT(SIZEOF_FIELD(struct test_t, b) == 1, "The b member is 1-byte wide.");
	BUILD_ASSERT(SIZEOF_FIELD(struct test_t, c) == 17, "The c member is 17-byte wide.");
	BUILD_ASSERT(SIZEOF_FIELD(struct test_t, d) == 2, "The d member is 2-byte wide.");
}

ZTEST(util, test_utf8_trunc_truncated)
{
	struct {
		char input[20];
		char expected[20];
	} tests[] = {
		{"ééé", "éé"},                    /* 2-byte UTF-8 characters */
		{"€€€", "€€"},                    /* 3-byte UTF-8 characters */
		{"𠜎𠜎𠜎", "𠜎𠜎"},                 /* 4-byte UTF-8 characters */
		{"Hello 世界!🌍", "Hello 世界!"},   /* mixed UTF-8 characters */
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i].input[strlen(tests[i].input) - 1] = '\0';
		utf8_trunc(tests[i].input);
		zassert_str_equal(tests[i].input, tests[i].expected, "Failed to truncate");
	}
}

ZTEST(util, test_utf8_trunc_not_truncated)
{
	struct {
		char input[20];
		char expected[20];
	} tests[] = {
		{"abc", "abc"},                    /* 1-byte ASCII characters */
		{"ééé", "ééé"},                    /* 2-byte UTF-8 characters */
		{"€€€", "€€€"},                    /* 3-byte UTF-8 characters */
		{"𠜎𠜎𠜎", "𠜎𠜎𠜎"},                /* 4-byte UTF-8 characters */
		{"Hello 世界!🌍", "Hello 世界!🌍"},  /* mixed UTF-8 characters */
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		utf8_trunc(tests[i].input);
		zassert_str_equal(tests[i].input, tests[i].expected, "No-op truncation failed");
	}
}

ZTEST(util, test_utf8_trunc_zero_length)
{
	/* Attempt to truncate a valid UTF8 string and verify no changed */
	char test_str[] = "";
	char expected_result[] = "";

	utf8_trunc(test_str);

	zassert_str_equal(test_str, expected_result, "Failed to truncate");
}

ZTEST(util, test_utf8_lcpy_truncated)
{
	/* dest_str size is based on storing 2 * € plus the null terminator plus an extra space to
	 * verify that it's truncated properly
	 */
	char dest_str[strlen("€") * 2 + 1 + 1];
	char test_str[] = "€€€";
	char expected_result[] = "€€";

	utf8_lcpy(dest_str, test_str, sizeof((dest_str)));

	zassert_str_equal(dest_str, expected_result, "Failed to copy");
}

ZTEST(util, test_utf8_lcpy_not_truncated)
{
	/* dest_str size is based on storing 3 * € plus the null terminator  */
	char dest_str[strlen("€") * 3 + 1];
	char test_str[] = "€€€";
	char expected_result[] = "€€€";

	utf8_lcpy(dest_str, test_str, sizeof((dest_str)));

	zassert_str_equal(dest_str, expected_result, "Failed to truncate");
}

ZTEST(util, test_utf8_lcpy_zero_length_copy)
{
	/* dest_str size is based on the null terminator */
	char dest_str[1];
	char test_str[] = "";
	char expected_result[] = "";

	utf8_lcpy(dest_str, test_str, sizeof((dest_str)));

	zassert_str_equal(dest_str, expected_result, "Failed to truncate");
}

ZTEST(util, test_utf8_lcpy_zero_length_dest)
{
	char dest_str[] = "A";
	char test_str[] = "";
	char expected_result[] = "A"; /* expect no changes to dest_str */

	utf8_lcpy(dest_str, test_str, 0);

	zassert_str_equal(dest_str, expected_result, "Failed to truncate");
}

ZTEST(util, test_utf8_lcpy_null_termination)
{
	char dest_str[] = "DEADBEEF";
	char test_str[] = "DEAD";
	char expected_result[] = "DEAD";

	utf8_lcpy(dest_str, test_str, sizeof(dest_str));

	zassert_str_equal(dest_str, expected_result, "Failed to truncate");
}

ZTEST(util, test_utf8_count_chars_ASCII)
{
	const char *test_str = "I have 15 char.";
	int count = utf8_count_chars(test_str);

	zassert_equal(count, 15, "Failed to count ASCII");
}

ZTEST(util, test_utf8_count_chars_non_ASCII)
{
	const char *test_str = "Hello دنیا!🌍";
	int count = utf8_count_chars(test_str);

	zassert_equal(count, 12, "Failed to count non-ASCII");
}

ZTEST(util, test_utf8_count_chars_invalid_utf)
{
	const char test_str[] = { (char)0x80, 0x00 };
	int count = utf8_count_chars(test_str);
	int expected_result = -EINVAL;

	zassert_equal(count, expected_result, "Failed to detect invalid UTF");
}

ZTEST(util, test_util_eq)
{
	uint8_t src1[16];
	uint8_t src2[16];

	bool mem_area_matching_1;
	bool mem_area_matching_2;

	memset(src1, 0, sizeof(src1));
	memset(src2, 0, sizeof(src2));

	for (size_t i = 0U; i < 16; i++) {
		src1[i] = 0xAB;
		src2[i] = 0xAB;
	}

	src1[15] = 0xCD;
	src2[15] = 0xEF;

	mem_area_matching_1 = util_eq(src1, sizeof(src1), src2, sizeof(src2));
	mem_area_matching_2 = util_eq(src1, sizeof(src1) - 1, src2, sizeof(src2) - 1);

	zassert_false(mem_area_matching_1);
	zassert_true(mem_area_matching_2);
}

ZTEST(util, test_util_memeq)
{
	uint8_t src1[16];
	uint8_t src2[16];
	uint8_t src3[16];

	bool mem_area_matching_1;
	bool mem_area_matching_2;

	memset(src1, 0, sizeof(src1));
	memset(src2, 0, sizeof(src2));

	for (size_t i = 0U; i < 16; i++) {
		src1[i] = 0xAB;
		src2[i] = 0xAB;
		src3[i] = 0xCD;
	}

	mem_area_matching_1 = util_memeq(src1, src2, sizeof(src1));
	mem_area_matching_2 = util_memeq(src1, src3, sizeof(src1));

	zassert_true(mem_area_matching_1);
	zassert_false(mem_area_matching_2);
}

static void test_single_bitmask_find_gap(uint32_t mask, size_t num_bits, size_t total_bits,
					 bool first_match, int exp_rv, int line)
{
	int rv;

	rv = bitmask_find_gap(mask, num_bits, total_bits, first_match);
	zassert_equal(rv, exp_rv, "%d Unexpected rv:%d (exp:%d)", line, rv, exp_rv);
}

ZTEST(util, test_bitmask_find_gap)
{
	test_single_bitmask_find_gap(0x0F0F070F, 6, 32, true, -1, __LINE__);
	test_single_bitmask_find_gap(0x0F0F070F, 5, 32, true, 11, __LINE__);
	test_single_bitmask_find_gap(0x030F070F, 5, 32, true, 26, __LINE__);
	test_single_bitmask_find_gap(0x030F070F, 5, 32, false, 11, __LINE__);
	test_single_bitmask_find_gap(0x0F0F070F, 5, 32, true, 11, __LINE__);
	test_single_bitmask_find_gap(0x030F070F, 5, 32, true, 26, __LINE__);
	test_single_bitmask_find_gap(0x030F070F, 5, 32, false, 11, __LINE__);
	test_single_bitmask_find_gap(0x0, 1, 32, true, 0, __LINE__);
	test_single_bitmask_find_gap(0x1F1F071F, 4, 32, true, 11, __LINE__);
	test_single_bitmask_find_gap(0x0000000F, 2, 6, false, 4, __LINE__);
}

ZTEST_SUITE(util, NULL, NULL, NULL, NULL, NULL);
