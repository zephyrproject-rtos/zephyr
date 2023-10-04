/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <string.h>

/**
 * @brief Test of u8_to_dec
 *
 * This test verifies conversion of various input values.
 *
 */
void run_u8_to_dec(void)
{
	char text[4];
	uint8_t len;

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

void run_COND_CODE_1(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
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

void run_COND_CODE_0(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
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

#undef ZERO
#undef SEVEN
#undef A_BUILD_ERROR
#define ZERO 0
#define SEVEN 7
#define A_BUILD_ERROR (this would be a build error if you used || or &&)

void run_UTIL_OR(void)
{
	zassert_equal(UTIL_OR(SEVEN, A_BUILD_ERROR), 7);
	zassert_equal(UTIL_OR(7, 0), 7);
	zassert_equal(UTIL_OR(SEVEN, ZERO), 7);
	zassert_equal(UTIL_OR(0, 7), 7);
	zassert_equal(UTIL_OR(ZERO, SEVEN), 7);
	zassert_equal(UTIL_OR(0, 0), 0);
	zassert_equal(UTIL_OR(ZERO, ZERO), 0);
}

void run_UTIL_AND(void)
{
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

void run_IF_ENABLED(void)
{
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

void run_LISTIFY(void)
{
	int ab0 = 1;
	int ab1 = 1;
#define A_PTR(x, name0, name1) &UTIL_CAT(UTIL_CAT(name0, name1), x)

	int *a[] = { LISTIFY(2, A_PTR, (,), a, b) };

	zassert_equal(ARRAY_SIZE(a), 2);
	zassert_equal(a[0], &ab0);
	zassert_equal(a[1], &ab1);
}

void run_MACRO_MAP_CAT(void)
{
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

/* Test checks if @ref Z_MAX, @ref Z_MIN and @ref Z_CLAMP return correct result
 * and perform single evaluation of input arguments.
 */
void run_z_max_z_min_z_clamp(void)
{
	zassert_equal(Z_MAX(inc_func(true), 0), 1, "Unexpected macro result");
	/* Z_MAX should have call inc_func only once */
	zassert_equal(inc_func(false), 2, "Unexpected return value");

	zassert_equal(Z_MIN(inc_func(false), 2), 2, "Unexpected macro result");
	/* Z_MIN should have call inc_func only once */
	zassert_equal(inc_func(false), 4, "Unexpected return value");

	zassert_equal(Z_CLAMP(inc_func(false), 1, 3), 3, "Unexpected macro result");
	/* Z_CLAMP should have call inc_func only once */
	zassert_equal(inc_func(false), 6, "Unexpected return value");

	zassert_equal(Z_CLAMP(inc_func(false), 10, 15), 10,
		      "Unexpected macro result");
	/* Z_CLAMP should have call inc_func only once */
	zassert_equal(inc_func(false), 8, "Unexpected return value");
}

void run_CLAMP(void)
{
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

void run_IN_RANGE(void)
{
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

void run_FOR_EACH(void)
{
	#define FOR_EACH_MACRO_TEST(arg) *buf++ = arg

	uint8_t array[3] = {0};
	uint8_t *buf = array;

	FOR_EACH(FOR_EACH_MACRO_TEST, (;), 1, 2, 3);

	zassert_equal(array[0], 1, "Unexpected value %d", array[0]);
	zassert_equal(array[1], 2, "Unexpected value %d", array[1]);
	zassert_equal(array[2], 3, "Unexpected value %d", array[2]);
}

void run_FOR_EACH_NONEMPTY_TERM(void)
{
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

void run_FOR_EACH_FIXED_ARG(void)
{
	uint32_t sum = 0;

	FOR_EACH_FIXED_ARG(fsum, (;), &sum, 1, 2, 3);

	zassert_equal(sum, 6, "Unexpected value %d", sum);
}

void run_FOR_EACH_IDX(void)
{
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

void run_FOR_EACH_IDX_FIXED_ARG(void)
{
	#undef FOO
	#define FOO(n, arg, fixed_arg) \
		uint8_t fixed_arg##n = arg

	FOR_EACH_IDX_FIXED_ARG(FOO, (;), a, 1, 2, 3);

	zassert_equal(a0, 1, "Unexpected value %d", a0);
	zassert_equal(a1, 2, "Unexpected value %d", a1);
	zassert_equal(a2, 3, "Unexpected value %d", a2);
}

void run_IS_EMPTY(void)
{
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

void run_IS_EQ(void)
{
	zassert_true(IS_EQ(0, 0), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(1, 1), "Unexpected IS_EQ result");
	zassert_true(IS_EQ(7, 7), "Unexpected IS_EQ result");

	zassert_false(IS_EQ(0, 1), "Unexpected IS_EQ result");
	zassert_false(IS_EQ(1, 7), "Unexpected IS_EQ result");
	zassert_false(IS_EQ(7, 0), "Unexpected IS_EQ result");
}

void run_LIST_DROP_EMPTY(void)
{
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
	zassert_equal(strcmp(arr[0], "Henry"), 0, "Failed at 0");
	zassert_equal(strcmp(arr[1], "Dorsett"), 0, "Failed at 1");
	zassert_equal(strcmp(arr[2], "Case"), 0, "Failed at 0");
}

void run_nested_FOR_EACH(void)
{
	#define FOO_1(x) a##x = x
	#define FOO_2(x) int x

	FOR_EACH(FOO_2, (;), FOR_EACH(FOO_1, (,), 0, 1, 2));

	zassert_equal(a0, 0);
	zassert_equal(a1, 1);
	zassert_equal(a2, 2);
}

void run_GET_ARG_N(void)
{
	int a = GET_ARG_N(1, 10, 100, 1000);
	int b = GET_ARG_N(2, 10, 100, 1000);
	int c = GET_ARG_N(3, 10, 100, 1000);

	zassert_equal(a, 10);
	zassert_equal(b, 100);
	zassert_equal(c, 1000);
}

void run_GET_ARGS_LESS_N(void)
{
	uint8_t a[] = { GET_ARGS_LESS_N(0, 1, 2, 3) };
	uint8_t b[] = { GET_ARGS_LESS_N(1, 1, 2, 3) };
	uint8_t c[] = { GET_ARGS_LESS_N(2, 1, 2, 3) };

	zassert_equal(sizeof(a), 3);

	zassert_equal(sizeof(b), 2);
	zassert_equal(b[0], 2);
	zassert_equal(b[1], 3);

	zassert_equal(sizeof(c), 1);
	zassert_equal(c[0], 3);
}

void run_mixing_GET_ARG_and_FOR_EACH(void)
{
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

void run_IS_ARRAY_ELEMENT(void)
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

void run_ARRAY_INDEX(void)
{
	size_t i;
	size_t array[] = {0, 1, 2, 3};

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_equal(array[ARRAY_INDEX(array, &array[i])], i);
	}

	/* ARRAY_INDEX(array, &alias[1]) asserts with CONFIG_ASSERT=y */
}

void run_PART_OF_ARRAY(void)
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

void run_ARRAY_INDEX_FLOOR(void)
{
	size_t i;
	size_t array[] = {0, 1, 2, 3};
	uint8_t *const alias = (uint8_t *)array;

	for (i = 0; i < ARRAY_SIZE(array); ++i) {
		zassert_equal(array[ARRAY_INDEX_FLOOR(array, &array[i])], i);
	}

	zassert_equal(array[ARRAY_INDEX_FLOOR(array, &alias[1])], 0);
}

void run_BIT_MASK(void)
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

void run_BIT_MASK64(void)
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

void run_IS_BIT_MASK(void)
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

void run_IS_SHIFTED_BIT_MASK(void)
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

void run_DIV_ROUND_UP(void)
{
	zassert_equal(DIV_ROUND_UP(0, 1), 0);
	zassert_equal(DIV_ROUND_UP(1, 2), 1);
	zassert_equal(DIV_ROUND_UP(3, 2), 2);
}
