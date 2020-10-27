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

void test_COND_CODE_1(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_1(1, (uint32_t x0 = 1;), (uint32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_1(NOT_EXISTING_DEFINE, (uint32_t x1 = 1;), (uint32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_1(TEST_DEFINE_1, (uint32_t x2 = 1;), (uint32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_1(2, (uint32_t x3 = 1;), (uint32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

void test_COND_CODE_0(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_0(0, (uint32_t x0 = 1;), (uint32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_0(NOT_EXISTING_DEFINE, (uint32_t x1 = 1;), (uint32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_0(TEST_DEFINE_0, (uint32_t x2 = 1;), (uint32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_0(2, (uint32_t x3 = 1;), (uint32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

#undef ZERO
#undef SEVEN
#undef A_BUILD_ERROR
#define ZERO 0
#define SEVEN 7
#define A_BUILD_ERROR (this would be a build error if you used || or &&)

void test_UTIL_OR(void)
{
	zassert_equal(UTIL_OR(SEVEN, A_BUILD_ERROR), 7, NULL);
	zassert_equal(UTIL_OR(7, 0), 7, NULL);
	zassert_equal(UTIL_OR(SEVEN, ZERO), 7, NULL);
	zassert_equal(UTIL_OR(0, 7), 7, NULL);
	zassert_equal(UTIL_OR(ZERO, SEVEN), 7, NULL);
	zassert_equal(UTIL_OR(0, 0), 0, NULL);
	zassert_equal(UTIL_OR(ZERO, ZERO), 0, NULL);
}

void test_UTIL_AND(void)
{
	zassert_equal(UTIL_AND(ZERO, A_BUILD_ERROR), 0, NULL);
	zassert_equal(UTIL_AND(7, 0), 0, NULL);
	zassert_equal(UTIL_AND(SEVEN, ZERO), 0, NULL);
	zassert_equal(UTIL_AND(0, 7), 0, NULL);
	zassert_equal(UTIL_AND(ZERO, SEVEN), 0, NULL);
	zassert_equal(UTIL_AND(0, 0), 0, NULL);
	zassert_equal(UTIL_AND(ZERO, ZERO), 0, NULL);
	zassert_equal(UTIL_AND(7, 7), 7, NULL);
	zassert_equal(UTIL_AND(7, SEVEN), 7, NULL);
	zassert_equal(UTIL_AND(SEVEN, 7), 7, NULL);
	zassert_equal(UTIL_AND(SEVEN, SEVEN), 7, NULL);
}

void test_IF_ENABLED(void)
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

void test_UTIL_LISTIFY(void)
{
	int i = 0;

#define INC(x, _)		\
	do {			\
		i += x;		\
	} while (0);

#define DEFINE(x, y) int a##x = x * y;
#define MARK_UNUSED(x, _) ARG_UNUSED(a##x);

	UTIL_LISTIFY(4, DEFINE, 2)
	UTIL_LISTIFY(4, MARK_UNUSED, _)

	UTIL_LISTIFY(4, INC, _)

	zassert_equal(i, 0 + 1 + 2 + 3, NULL);
	zassert_equal(a0, 0, NULL);
	zassert_equal(a1, 2, NULL);
	zassert_equal(a2, 4, NULL);
	zassert_equal(a3, 6, NULL);
}

void test_MACRO_MAP_CAT(void)
{
	int item_a_item_b_item_c_ = 1;

#undef FOO
#define FOO(x) item_##x##_
	zassert_equal(MACRO_MAP_CAT(FOO, a, b, c), 1, "MACRO_MAP_CAT");
#undef FOO
}

static int inc_func(void)
{
	static int a = 1;

	return a++;
}

/* Test checks if @ref Z_MAX, @ref Z_MIN and @ref Z_CLAMP return correct result
 * and perform single evaluation of input arguments.
 */
static void test_z_max_z_min_z_clamp(void)
{
	zassert_equal(Z_MAX(inc_func(), 0), 1, "Unexpected macro result");
	/* Z_MAX should have call inc_func only once */
	zassert_equal(inc_func(), 2, "Unexpected return value");

	zassert_equal(Z_MIN(inc_func(), 2), 2, "Unexpected macro result");
	/* Z_MIN should have call inc_func only once */
	zassert_equal(inc_func(), 4, "Unexpected return value");

	zassert_equal(Z_CLAMP(inc_func(), 1, 3), 3, "Unexpected macro result");
	/* Z_CLAMP should have call inc_func only once */
	zassert_equal(inc_func(), 6, "Unexpected return value");

	zassert_equal(Z_CLAMP(inc_func(), 10, 15), 10,
		      "Unexpected macro result");
	/* Z_CLAMP should have call inc_func only once */
	zassert_equal(inc_func(), 8, "Unexpected return value");
}

static void test_CLAMP(void)
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

static void test_FOR_EACH(void)
{
	#define FOR_EACH_MACRO_TEST(arg) *buf++ = arg

	uint8_t array[3] = {0};
	uint8_t *buf = array;

	FOR_EACH(FOR_EACH_MACRO_TEST, (;), 1, 2, 3);

	zassert_equal(array[0], 1, "Unexpected value %d", array[0]);
	zassert_equal(array[1], 2, "Unexpected value %d", array[1]);
	zassert_equal(array[2], 3, "Unexpected value %d", array[2]);
}

static void test_FOR_EACH_NONEMPTY_TERM(void)
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

static void test_FOR_EACH_FIXED_ARG(void)
{
	uint32_t sum = 0;

	FOR_EACH_FIXED_ARG(fsum, (;), &sum, 1, 2, 3);

	zassert_equal(sum, 6, "Unexpected value %d", sum);
}

static void test_FOR_EACH_IDX(void)
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

	zassert_equal(ARRAY_SIZE(a), 3, "Unexpected value:%d", ARRAY_SIZE(a));
}

static void test_FOR_EACH_IDX_FIXED_ARG(void)
{
	#undef FOO
	#define FOO(n, arg, fixed_arg) \
		uint8_t fixed_arg##n = arg

	FOR_EACH_IDX_FIXED_ARG(FOO, (;), a, 1, 2, 3);

	zassert_equal(a0, 1, "Unexpected value %d", a0);
	zassert_equal(a1, 2, "Unexpected value %d", a1);
	zassert_equal(a2, 3, "Unexpected value %d", a2);
}

static void test_IS_EMPTY(void)
{
	#define test_IS_EMPTY_REAL_EMPTY
	#define test_IS_EMPTY_NOT_EMPTY XXX_DO_NOT_REPLACE_XXX
	zassert_true(IS_EMPTY(test_IS_EMPTY_REAL_EMPTY),
		     "Expected to be empty");
	zassert_false(IS_EMPTY(test_IS_EMPTY_NOT_EMPTY),
		      "Expected to be non-empty");
}

static void test_LIST_DROP_EMPTY(void)
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

static void test_nested_FOR_EACH(void)
{
	#define FOO_1(x) a##x = x
	#define FOO_2(x) int x

	FOR_EACH(FOO_2, (;), FOR_EACH(FOO_1, (,), 0, 1, 2));

	zassert_equal(a0, 0, NULL);
	zassert_equal(a1, 1, NULL);
	zassert_equal(a2, 2, NULL);
}

static void test_GET_ARG_N(void)
{
	int a = GET_ARG_N(1, 10, 100, 1000);
	int b = GET_ARG_N(2, 10, 100, 1000);
	int c = GET_ARG_N(3, 10, 100, 1000);

	zassert_equal(a, 10, NULL);
	zassert_equal(b, 100, NULL);
	zassert_equal(c, 1000, NULL);
}

static void test_GET_ARGS_LESS_N(void)
{
	uint8_t a[] = { GET_ARGS_LESS_N(0, 1, 2, 3) };
	uint8_t b[] = { GET_ARGS_LESS_N(1, 1, 2, 3) };
	uint8_t c[] = { GET_ARGS_LESS_N(2, 1, 2, 3) };

	zassert_equal(sizeof(a), 3, NULL);

	zassert_equal(sizeof(b), 2, NULL);
	zassert_equal(b[0], 2, NULL);
	zassert_equal(b[1], 3, NULL);

	zassert_equal(sizeof(c), 1, NULL);
	zassert_equal(c[0], 3, NULL);
}

static void test_mixing_GET_ARG_and_FOR_EACH(void)
{
	#undef TEST_MACRO
	#define TEST_MACRO(x) x,
	int i;

	i = GET_ARG_N(3, FOR_EACH(TEST_MACRO, (), 1, 2, 3, 4, 5));
	zassert_equal(i, 3, NULL);

	i = GET_ARG_N(2, 1, GET_ARGS_LESS_N(2, 1, 2, 3, 4, 5));
	zassert_equal(i, 3, NULL);

	#undef TEST_MACRO
	#undef TEST_MACRO2
	#define TEST_MACRO(x) GET_ARG_N(3, 1, 2, x),
	#define TEST_MACRO2(...) FOR_EACH(TEST_MACRO, (), __VA_ARGS__)
	int a[] = {
		LIST_DROP_EMPTY(TEST_MACRO2(1, 2, 3, 4)), 5
	};

	zassert_equal(ARRAY_SIZE(a), 5, NULL);
	zassert_equal(a[0], 1, NULL);
	zassert_equal(a[1], 2, NULL);
	zassert_equal(a[2], 3, NULL);
	zassert_equal(a[3], 4, NULL);
	zassert_equal(a[4], 5, NULL);
}

#if __cplusplus
extern "C" void test_cxx(void);
void test_cxx(void)
#else
void test_cc(void)
#endif
{
	ztest_test_suite(test_lib_sys_util_tests,
			 ztest_unit_test(test_u8_to_dec),
			 ztest_unit_test(test_COND_CODE_1),
			 ztest_unit_test(test_COND_CODE_0),
			 ztest_unit_test(test_UTIL_OR),
			 ztest_unit_test(test_UTIL_AND),
			 ztest_unit_test(test_IF_ENABLED),
			 ztest_unit_test(test_UTIL_LISTIFY),
			 ztest_unit_test(test_MACRO_MAP_CAT),
			 ztest_unit_test(test_z_max_z_min_z_clamp),
			 ztest_unit_test(test_CLAMP),
			 ztest_unit_test(test_FOR_EACH),
			 ztest_unit_test(test_FOR_EACH_NONEMPTY_TERM),
			 ztest_unit_test(test_FOR_EACH_FIXED_ARG),
			 ztest_unit_test(test_FOR_EACH_IDX),
			 ztest_unit_test(test_FOR_EACH_IDX_FIXED_ARG),
			 ztest_unit_test(test_IS_EMPTY),
			 ztest_unit_test(test_LIST_DROP_EMPTY),
			 ztest_unit_test(test_nested_FOR_EACH),
			 ztest_unit_test(test_GET_ARG_N),
			 ztest_unit_test(test_GET_ARGS_LESS_N),
			 ztest_unit_test(test_mixing_GET_ARG_and_FOR_EACH)
	);

	ztest_run_test_suite(test_lib_sys_util_tests);
}
