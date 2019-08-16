/* test_sprintf.c - test various sprintf functionality */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This module contains the code for testing sprintf() functionality.
 */

#include <ztest.h>
#include <stdio.h>
#include <stdarg.h>

#define DEADBEEF  0xdeadbeef

#define DEADBEEF_LHEX_ALT_STR  "0xdeadbeef"
#define DEADBEEF_UHEX_ALT_STR  "0XDEADBEEF"
#define DEADBEEF_LHEX_STR      "deadbeef"
#define DEADBEEF_UHEX_STR      "DEADBEEF"
#define DEADBEEF_UNSIGNED_STR  "3735928559"
#define DEADBEEF_SIGNED_STR    "-559038737"
#define DEADBEEF_OCTAL_STR     "33653337357"
#define DEADBEEF_OCTAL_ALT_STR "033653337357"
#define DEADBEEF_PTR_STR       "0xdeadbeef"

/*
 * A really long string (330 characters + NULL).
 * The underlying sprintf() architecture will truncate it.
 */
#define REALLY_LONG_STRING		     \
	"1111111111111111111111111111111111" \
	"1111111111111111111111111111111"    \
	"22222222222222222222222222222222"   \
	"222222222222222222222222222222222"  \
	"333333333333333333333333333333333"  \
	"33333333333333333333333333333333"   \
	"44444444444444444444444444444444"   \
	"444444444444444444444444444444444"  \
	"555555555555555555555555555555555"  \
	"55555555555555555555555555555555"   \
	"66666666666666666666666666666666"   \
	"666666666666666666666666666666666"

union raw_double_u {
	double d;
	struct {
		u32_t u1;       /* This part contains the exponent */
		u32_t u2;       /* This part contains the fraction */
	};
};

/**
 *
 * @brief Test sprintf with doubles
 *
 */

void test_sprintf_double(void)
{
	char buffer[400];
	union raw_double_u var;

#ifndef CONFIG_FLOAT
	ztest_test_skip();
	return;
#endif

	var.u1 = 0x00000000;
	var.u2 = 0x7ff00000;    /* Bit pattern for +INF (double) */
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "inf") == 0),
		     "sprintf(inf) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "INF") == 0),
		     "sprintf(INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "inf") == 0),
		     "sprintf(inf) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%F", var.d);
	zassert_true((strcmp(buffer, "INF") == 0),
		     "sprintf(INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "inf") == 0),
		     "sprintf(inf) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%G", var.d);
	zassert_true((strcmp(buffer, "INF") == 0),
		     "sprintf(INF) - incorrect output '%s'\n", buffer);

	var.u1 = 0x00000000;
	var.u2 = 0xfff00000;    /* Bit pattern for -INF (double) */
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "-inf") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "-INF") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "-inf") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%F", var.d);
	zassert_true((strcmp(buffer, "-INF") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "-inf") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%G", var.d);
	zassert_true((strcmp(buffer, "-INF") == 0),
		     "sprintf(-INF) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%010f", var.d);
	zassert_true((strcmp(buffer, "      -inf") == 0),
		     "sprintf(      +inf) - incorrect output '%s'\n", buffer);

	var.u1 = 0x00000000;
	var.u2 = 0x7ff80000;    /* Bit pattern for NaN (double) */
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "nan") == 0),
		     "sprintf(nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "NAN") == 0),
		     "sprintf(NAN) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "nan") == 0),
		     "sprintf(nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%F", var.d);
	zassert_true((strcmp(buffer, "NAN") == 0),
		     "sprintf(NAN) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "nan") == 0),
		     "sprintf(nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%G", var.d);
	zassert_true((strcmp(buffer, "NAN") == 0),
		     "sprintf(NAN) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%+8.5e", var.d);
	zassert_true((strcmp(buffer, "    +nan") == 0),
		     "sprintf(    +nan) - incorrect output '%s'\n", buffer);

	var.u1 = 0x00000000;
	var.u2 = 0xfff80000;    /* Bit pattern for -NaN (double) */
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "-nan") == 0),
		     "sprintf(-nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "-NAN") == 0),
		     "sprintf(-NAN) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "-nan") == 0),
		     "sprintf(-nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%F", var.d);
	zassert_true((strcmp(buffer, "-NAN") == 0),
		     "sprintf(-NAN) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "-nan") == 0),
		     "sprintf(-nan) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%G", var.d);
	zassert_true((strcmp(buffer, "-NAN") == 0),
		     "sprintf(-NAN) - incorrect output '%s'\n", buffer);

	var.d = 1.0;
	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "1.000000") == 0),
		     "sprintf(1.0) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%+f", var.d);
	zassert_true((strcmp(buffer, "+1.000000") == 0),
		     "sprintf(+1.0) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%.2f", var.d);
	zassert_true((strcmp(buffer, "1.00") == 0),
		     "sprintf(1.00) - incorrect output '%s'\n", buffer);

	sprintf(buffer, "%.*f", 11, var.d);
	zassert_true((strcmp(buffer, "1.00000000000") == 0),
		     "sprintf(1.00000000000) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%12f", var.d);
	zassert_true((strcmp(buffer, "    1.000000") == 0),
		     "sprintf(    1.000000) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%-12f", var.d);
	zassert_true((strcmp(buffer, "1.000000    ") == 0),
		     "sprintf(1.000000    ) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%012f", var.d);
	zassert_true((strcmp(buffer, "00001.000000") == 0),
		     "sprintf(00001.000000) - incorrect "
		     "output '%s'\n", buffer);

	var.d = -1.0;
	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "-1.000000") == 0),
		     "sprintf(-1.0) - incorrect output '%s'\n", buffer);

	var.d = 1234.56789;
	sprintf(buffer, "%f", var.d);
	zassert_true((strcmp(buffer, "1234.567890") == 0),
		     "sprintf(-1.0) - incorrect output '%s'\n", buffer);

	/*
	 * With very large precision, the output differs significantly in
	 * terms of string even if not in terms of actual value depending
	 * on the library used and FPU implementation. However the length
	 * and decimal position should remain identical.
	 */
	var.d = 0x1p800;
	sprintf(buffer, "%.140f", var.d);
	zassert_true((strlen(buffer) == 382),
		     "sprintf(<large output>) - incorrect length %d\n",
		     strlen(buffer));
	buffer[10] = 0;  /* log facility doesn't support %.10s */
	zassert_true((strcmp(buffer, "6668014432") == 0),
		     "sprintf(<large output>) - starts with \"%s\" "
		     "expected \"6668014432\"\n", buffer);
	zassert_true((buffer[241] == '.'),
		      "sprintf(<large output>) - expected '.' got '%c'\n",
		      buffer[241]);

	var.d = 0x1p-400;
	sprintf(buffer, "% .380f", var.d);
	zassert_true((strlen(buffer) == 383),
		     "sprintf(<large output>) - incorrect length %d\n",
		     strlen(buffer));
	buffer[10] = 0;  /* log facility doesn't support %.10s */
	zassert_true((strcmp(buffer, " 0.0000000") == 0),
		     "sprintf(<large output>) - starts with \"%s\" "
		     "expected \" 0.0000000\"\n", buffer);
	buffer[119 + 10] = 0;  /* log facility doesn't support %.10s */
	zassert_true((strcmp(buffer + 119, "0000387259") == 0),
		      "sprintf(<large output>) - got \"%s\" "
		      "while expecting \"0000387259\"\n", buffer + 119);

	/*******************/
	var.d = 1234.0;
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "1.234000e+03") == 0),
		     "sprintf(1.234000e+03) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "1.234000E+03") == 0),
		     "sprintf(1.234000E+03) - incorrect "
		     "output '%s'\n", buffer);

	/*******************/
	var.d = 0.1234;
	sprintf(buffer, "%e", var.d);
	zassert_true((strcmp(buffer, "1.234000e-01") == 0),
		     "sprintf(1.234000e-01) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%E", var.d);
	zassert_true((strcmp(buffer, "1.234000E-01") == 0),
		     "sprintf(1.234000E-01) - incorrect "
		     "output '%s'\n", buffer);

	/*******************/
	var.d = 1234000000.0;
	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "1.234e+09") == 0),
		     "sprintf(1.234e+09) - incorrect "
		     "output '%s'\n", buffer);

	sprintf(buffer, "%G", var.d);
	zassert_true((strcmp(buffer, "1.234E+09") == 0),
		     "sprintf(1.234E+09) - incorrect "
		     "output '%s'\n", buffer);

	var.d = 150.0;
	sprintf(buffer, "%#.3g", var.d);
	zassert_true((strcmp(buffer, "150.") == 0),
		     "sprintf(150.) - incorrect "
		     "output '%s'\n", buffer);

	var.d = 150.1;
	sprintf(buffer, "%.2g", var.d);
	zassert_true((strcmp(buffer, "1.5e+02") == 0),
		     "sprintf(1.5e+02) - incorrect "
		     "output '%s'\n", buffer);

	var.d = 150.567;
	sprintf(buffer, "%.3g", var.d);
	zassert_true((strcmp(buffer, "151") == 0),
		     "sprintf(151) - incorrect "
		     "output '%s'\n", buffer);

	var.d = 15e-5;
	sprintf(buffer, "%#.3g", var.d);
	zassert_true((strcmp(buffer, "0.000150") == 0),
		     "sprintf(0.000150) - incorrect "
		     "output '%s'\n", buffer);

	var.d = 1505e-7;
	sprintf(buffer, "%.4g", var.d);
	zassert_true((strcmp(buffer, "0.0001505") == 0),
		     "sprintf(0.0001505) - incorrect "
		     "output '%s'\n", buffer);

	var.u1 = 0x00000001;
	var.u2 = 0x00000000;    /* smallest denormal value */
	sprintf(buffer, "%g", var.d);
	zassert_true((strcmp(buffer, "4.94066e-324") == 0),
		     "sprintf(4.94066e-324) - incorrect "
		     "output '%s'\n", buffer);
}

/**
 * @brief A test wrapper for vsnprintf()
 */
int tvsnprintf(char *s, size_t len, const char *format, ...)
{
	va_list vargs;
	int r;

	va_start(vargs, format);
	r = vsnprintf(s, len, format, vargs);
	va_end(vargs);

	return r;
}

/**
 *
 * @brief Test the vsprintf() routine
 *
 * This routine does not aim to test the same underlying functionality as
 * sprintfTest().  Instead it tries to limit it to functionality specific to
 * vsnprintf().  Instead of calling vsnprintf() directly, it invokes the wrapper
 * routine tvsnprintf().
 *
 */

void test_vsnprintf(void)
{
	int len;
	char buffer[100];

	/*******************/
	buffer[0] = '\0';
	len = tvsnprintf(buffer, 0, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "vsnprintf(%%x).  Expected return value %zu, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);

	zassert_true((strcmp(buffer, "") == 0),
		     "vsnprintf(%%x).  Expected '%s', got '%s'\n",
		     "", buffer);

	/*******************/
	len = tvsnprintf(buffer, 4, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "vsnprintf(%%x).  Expected return value %zu, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);

	zassert_true((strcmp(buffer, "dea") == 0),
		     "vsnprintf(%%x).  Expected '%s', got '%s'\n",
		     "dea", buffer);

}

/**
 *
 * @brief A test wrapper for vsprintf()
 */

int tvsprintf(char *s, const char *format, ...)
{
	va_list vargs;
	int r;

	va_start(vargs, format);
	r = vsprintf(s, format, vargs);
	va_end(vargs);

	return r;
}

/**
 *
 * @brief Test the vsprintf() routine
 *
 * This routine does not aim to test the same underlying functionality as
 * sprintfTest().  Instead it tries to limit it to functionality specific to
 * vsprintf().
 *
 */

void test_vsprintf(void)
{
	int len;
	char buffer[100];

	/*******************/
	len = tvsprintf(buffer, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "sprintf(%%x).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_LHEX_STR) == 0),
		     "sprintf(%%x).  Expected '%s', got '%s'\n",
		     DEADBEEF_LHEX_STR, buffer);
}

/**
 *
 * @brief Test the snprintf() routine
 *
 * This routine does not aim to test the same underlying functionality as
 * sprintfTest().  Instead it tries to limit it to functionality specific to
 * snprintf().
 *
 */

void test_snprintf(void)
{
#if defined(__GNUC__) && __GNUC__ >= 7
	/*
	 * GCC 7 and newer are smart enough to realize that in the statements
	 * below, the output will not fit in 0 or 4 bytes, but that it requires
	 * 9.
	 * So it throws a warning in compile time. But in this case we are
	 * actually testing that snprintf's return value is what it should be
	 * while truncating the output. So let's suppress this warning here.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

	int len;
	char buffer[100];

	/*******************/
	buffer[0] = '\0';
	len = snprintf(buffer, 0, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "snprintf(%%x).  Expected return value %zu, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);

	zassert_true((strcmp(buffer, "") == 0),
		     "snprintf(%%x).  Expected '%s', got '%s'\n",
		     "", buffer);
	/*******************/
	len = snprintf(buffer, 4, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "snprintf(%%x).  Expected return value %zu, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);

	zassert_true((strcmp(buffer, "dea") == 0),
		     "snprintf(%%x).  Expected '%s', got '%s'\n",
		     "dea", buffer);

#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
}

/**
 *
 * @brief Test the sprintf() routine with miscellaneous specifiers
 *
 */

void test_sprintf_misc(void)
{
	int count;
	char buffer[100];

	/*******************/
	sprintf(buffer, "%p", (void *) DEADBEEF);
	zassert_false((strcmp(buffer, DEADBEEF_PTR_STR) != 0),
		      "sprintf(%%p).  Expected '%s', got '%s'", DEADBEEF_PTR_STR, buffer);
	/*******************/
	sprintf(buffer, "test data %n test data", &count);
	zassert_false((count != 10), "sprintf(%%n).  Expected count to be %d, not %d",
		      10, count);

	zassert_false((strcmp(buffer, "test data  test data") != 0),
		      "sprintf(%%p).  Expected '%s', got '%s'",
		      "test data  test data", buffer);

	/*******************/
	sprintf(buffer, "%*d", 10, 1234);
	zassert_true((strcmp(buffer, "      1234") == 0),
		     "sprintf(%%p).  Expected '%s', got '%s'",
		     "      1234", buffer);

	/*******************/
	sprintf(buffer, "%*d", -10, 1234);
	zassert_true((strcmp(buffer, "1234      ") == 0),
		     "sprintf(%%p).  Expected '%s', got '%s'",
		     "1234      ", buffer);

	/*******************/
	sprintf(buffer, "% d", 1234);
	zassert_true((strcmp(buffer, " 1234") == 0),
		     "sprintf(%% d). Expected '%s', got '%s'",
		     " 1234", buffer);

	/*******************/
	sprintf(buffer, "%hx", (unsigned short)1234);
	zassert_true((strcmp("4d2", buffer) == 0),
		     "sprintf(%%hx).  Expected '4d2', got '%s'", buffer);

	/*******************/
	sprintf(buffer, "%lx", 1234ul);
	zassert_true((strcmp("4d2", buffer) == 0),
		     "sprintf(%%lx).  Expected '4d2', got '%s'", buffer);

}

/**
 *
 * @brief Test the sprintf() routine with integers
 *
 */
void test_sprintf_integer(void)
{
	int len;
	char buffer[100];

	/*******************/

	/* Note: prints hex numbers in 8 characters */
	len = sprintf(buffer, "%x", 0x11);
	zassert_true((len == 2),
		     "sprintf(%%x). "
		     "Expected 2 bytes written, not %d", len);

	zassert_true((strcmp(buffer, "11") == 0),
		     "sprintf(%%x). "
		     "Expected '%s', got '%s'", "11", buffer);

	/*******************/
	len = sprintf(buffer, "%x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_STR)),
		     "sprintf(%%x).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_LHEX_STR), len);
	/*******************/
	zassert_true((strcmp(buffer, DEADBEEF_LHEX_STR) == 0),
		     "sprintf(%%x).  Expected '%s', got '%s'\n",
		     DEADBEEF_LHEX_STR, buffer);
	/*******************/
	len = sprintf(buffer, "%X", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_UHEX_STR)),
		     "sprintf(%%X).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_UHEX_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_UHEX_STR) == 0),
		     "sprintf(%%X).  Expected '%s', got '%s'\n",
		     DEADBEEF_UHEX_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%u", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_UNSIGNED_STR)),
		     "sprintf(%%u).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_UNSIGNED_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_UNSIGNED_STR) == 0),
		     "sprintf(%%u).  Expected '%s', got '%s'\n",
		     DEADBEEF_UNSIGNED_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%d", (int) DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_SIGNED_STR)),
		     "sprintf(%%d).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_SIGNED_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_SIGNED_STR) == 0),
		     "sprintf(%%d).  Expected '%s', got '%s'\n",
		     DEADBEEF_SIGNED_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%o", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_OCTAL_STR)),
		     "sprintf(%%o).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_OCTAL_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_OCTAL_STR) == 0),
		     "sprintf(%%o).  Expected '%s', got '%s'\n",
		     DEADBEEF_OCTAL_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%#o", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_OCTAL_ALT_STR)),
		     "sprintf(%%#o).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_OCTAL_ALT_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_OCTAL_ALT_STR) == 0),
		     "sprintf(%%#o).  Expected '%s', got '%s'\n",
		     DEADBEEF_OCTAL_ALT_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%#x", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_LHEX_ALT_STR)),
		     "sprintf(%%#x).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_LHEX_ALT_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_LHEX_ALT_STR) == 0),
		     "sprintf(%%#x).  Expected '%s', got '%s'\n",
		     DEADBEEF_LHEX_ALT_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%#X", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_UHEX_ALT_STR)),
		     "sprintf(%%#X).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_UHEX_ALT_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_UHEX_ALT_STR) == 0),
		     "sprintf(%%#X).  Expected '%s', got '%s'\n",
		     DEADBEEF_UHEX_ALT_STR, buffer);

	/*******************/

	len = sprintf(buffer, "%+d", 1);
	zassert_true((len == 2),
		     "sprintf(%%+d).  Expected %d bytes written, not %d\n",
		     2, len);

	zassert_true((strcmp(buffer, "+1") == 0),
		     "sprintf(%%+d). Expected '+1', got '%s'\n", buffer);

}

/**
 *
 * @brief Test sprintf with strings
 *
 */

void test_sprintf_string(void)
{
	char buffer[400];

	sprintf(buffer, "%%");
	zassert_true((strcmp(buffer, "%") == 0),
		     "sprintf(%%).  Expected '%%', got '%s'\n", buffer);

	sprintf(buffer, "%c", 't');
	zassert_true((strcmp(buffer, "t") == 0),
		     "sprintf(%%c).  Expected 't', got '%s'\n", buffer);

	sprintf(buffer, "%s", "short string");
	zassert_true((strcmp(buffer, "short string") == 0),
		     "sprintf(%%s). "
		     "Expected 'short string', got '%s'\n", buffer);

	sprintf(buffer, "%s", REALLY_LONG_STRING);
	zassert_true((strcmp(buffer, REALLY_LONG_STRING) == 0),
		     "sprintf(%%s) of REALLY_LONG_STRING doesn't match!\n");
}

/**
 *
 * @brief Test entry point
 *
 * @return N/A
 */

void test_main(void)
{
	ztest_test_suite(test_sprintf,
			 ztest_unit_test(test_sprintf_double),
			 ztest_unit_test(test_sprintf_integer),
			 ztest_unit_test(test_vsprintf),
			 ztest_unit_test(test_vsnprintf),
			 ztest_unit_test(test_sprintf_string),
			 ztest_unit_test(test_sprintf_misc));
	ztest_run_test_suite(test_sprintf);
}
