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
 * A really long string (330 characters + NUL).
 * The underlying sprintf() architecture will truncate it.
 */
#define REALLY_LONG_STRING \
		"1111111111111111111111111111111111" \
		"1111111111111111111111111111111" \
		"22222222222222222222222222222222" \
		"222222222222222222222222222222222" \
		"333333333333333333333333333333333" \
		"33333333333333333333333333333333" \
		"44444444444444444444444444444444" \
		"444444444444444444444444444444444" \
		"555555555555555555555555555555555" \
		"55555555555555555555555555555555" \
		"66666666666666666666666666666666" \
		"666666666666666666666666666666666"

#define PRINTF_MAX_STRING_LENGTH   200

union raw_double_u {
	double  d;
	struct {
		u32_t  u1;    /* This part contains the exponent */
		u32_t  u2;    /* This part contains the fraction */
	};
};

#ifdef CONFIG_FLOAT
/**
 *
 * @brief Test sprintf with doubles
 *
 */

void sprintf_double_test(void)
{
	char buffer[100];
	union raw_double_u var;

	var.u1 = 0x00000000;
	var.u2 = 0x7ff00000;    /* Bit pattern for +INF (double) */
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "+INF"), 0, NULL);

	var.u1 = 0x00000000;
	var.u2 = 0xfff00000;    /* Bit pattern for -INF (double) */
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "-INF"), 0, NULL);

	var.u1 = 0x00000000;
	var.u2 = 0x7ff80000;    /* Bit pattern for NaN (double) */
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "NaN"), 0, NULL);

	var.d = 1.0;
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "1.000000"), 0, "incorrect output");

	sprintf(buffer, "%+f", var.d);
	zassert_equal(strcmp(buffer, "+1.000000"), 0,
			"sprintf(+1.0) - incorrect output");

	sprintf(buffer, "%.2f", var.d);
	zassert_equal(strcmp(buffer, "1.00"), 0,
		"sprintf(1.00) - incorrect output");

	sprintf(buffer, "%.*f", 11, var.d);
	zassert_equal(strcmp(buffer, "1.00000000000"), 0,
		"sprintf(1.00000000000) - incorrect output");

	sprintf(buffer, "%12f", var.d);
	zassert_equal(strcmp(buffer, "1.000000"), 0,
		"sprintf(    1.000000) - incorrect output");

	sprintf(buffer, "%-12f", var.d);
	zassert_equal(strcmp(buffer, "1.000000"), 0,
		"sprintf(1.000000    ) - incorrect output");

	sprintf(buffer, "%012f", var.d);
	zassert_equal(strcmp(buffer, "00001.000000"), 0,
		"sprintf(00001.000000) - incorrect output");

	var.d = -1.0;
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "-1.000000"), 0,
		"sprintf(-1.0) - incorrect output");

	var.d = 1234.56789;
	sprintf(buffer, "%f", var.d);
	zassert_equal(strcmp(buffer, "1234.567890"), 0,
		"sprintf(-1.0) - incorrect output");

	/*******************/
	var.d = 1234.0;
	sprintf(buffer, "%e", var.d);
	zassert_equal(strcmp(buffer, "1.234000e+003"), 0,
		"sprintf(1.234000e+003) - incorrect output");

	sprintf(buffer, "%E", var.d);
	zassert_equal(strcmp(buffer, "1.234000E+003"), 0,
		"sprintf(1.234000E+003) - incorrect output");

	/*******************/
	var.d = 0.1234;
	sprintf(buffer, "%e", var.d);
	zassert_equal(strcmp(buffer, "1.234000e-001"), 0,
		"sprintf(1.234000e-001) - incorrect output");

	sprintf(buffer, "%E", var.d);
	zassert_equal(strcmp(buffer, "1.234000E-001"), 0,
		"sprintf(1.234000E-001) - incorrect output");

	/*******************/
	var.d = 1234000000.0;
	sprintf(buffer, "%g", var.d);
	zassert_equal(strcmp(buffer, "1.234e+009"), 0,
		"sprintf(1.234e+009) - incorrect output");

	sprintf(buffer, "%G", var.d);
	zassert_equal(strcmp(buffer, "1.234E+009"), 0,
		"sprintf(1.234E+009) - incorrect output");

}
#endif /* CONFIG_FLOAT */

/**
 *
 * @brief A test wrapper for vsnprintf()
 */

int tvsnprintf(char *s, size_t len, const char *format, ...)
{
	va_list  vargs;
	int      r;

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

void vsnprintf_test(void)
{
	int  len;
	char buffer[100];

	/*******************/
	buffer[0] = '\0';
	len = tvsnprintf(buffer, 0, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR), NULL);
	zassert_equal(strcmp(buffer, ""), 0, "");

	/*******************/
	len = tvsnprintf(buffer, 4, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR), NULL);
	zassert_equal(strcmp(buffer, "dea"), 0, NULL);
}

/**
 *
 * @brief A test wrapper for vsprintf()
 */

int tvsprintf(char *s, const char *format, ...)
{
	va_list  vargs;
	int      r;

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

void vsprintf_test(void)
{
	int  len;
	char buffer[100];

	/*******************/
	len = tvsprintf(buffer, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR), NULL);
	zassert_equal(strcmp(buffer, DEADBEEF_LHEX_STR), 0, NULL);
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

void snprintf_test(void)
{
	int  len;
	char buffer[100];


	/*******************/
	buffer[0] = '\0';
	len = snprintf(buffer, 0, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR), NULL);
	zassert_equal(strcmp(buffer, ""), 0, "");

	/*******************/
	len = snprintf(buffer, 4, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR), NULL);
	zassert_equal(strcmp(buffer, "dea"), 0, NULL);
}

/**
 *
 * @brief Test the sprintf() routine with miscellaneous specifiers
 *
 */

void sprintf_misc_test(void)
{
	int  count;
	char buffer[100];

	/*******************/
	sprintf(buffer, "%p", (void *) DEADBEEF);
	zassert_equal(strcmp(buffer, DEADBEEF_PTR_STR), 0, NULL);

	/*******************/
	sprintf(buffer, "test data %n test data", &count);
	zassert_equal(count, 10, NULL);
	zassert_equal(strcmp(buffer, "test data  test data"), 0, NULL);

	/*******************/
	sprintf(buffer, "%*d", 10, 1234);
	zassert_equal(strcmp(buffer, "      1234"), 0, NULL);

	/*******************/
	sprintf(buffer, "%*d", -10, 1234);
	zassert_equal(strcmp(buffer, "1234      "), 0, NULL);

	/*******************/
	sprintf(buffer, "% d", 1234);
	zassert_equal(strcmp(buffer, " 1234"), 0, NULL);

	/*******************/
	sprintf(buffer, "%hx", 1234);
	zassert_equal(strcmp("4d2", buffer), 0, NULL);

	/*******************/
	sprintf(buffer, "%lx", 1234ul);
	zassert_equal(strcmp("4d2", buffer), 0, NULL);
}

/**
 *
 * @brief Test the sprintf() routine with integers
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

void sprintf_integer_test(void)
{
	int  len;
	char buffer[100];

	/*******************/

	/* Note: prints hex numbers in 8 characters */
	len = sprintf(buffer, "%x", 0x11);
	zassert_equal(len, 2,
		"Expected 2 bytes written");

	zassert_equal(strcmp(buffer, "11"), 0,
		" Expected 11");

	/*******************/
	len = sprintf(buffer, "%x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_STR),
		"Expected byte not written");

	zassert_equal(strcmp(buffer, DEADBEEF_LHEX_STR), 0,
		"Expected byte not written");

	/*******************/
	len = sprintf(buffer, "%X", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_UHEX_STR),
		"Expected byte not written");
	zassert_equal(strcmp(buffer, DEADBEEF_UHEX_STR), 0,
		"Expected byte not written");

	/*******************/
	len = sprintf(buffer, "%u", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_UNSIGNED_STR),
		"Expected byte not written");

	zassert_equal(strcmp(buffer, DEADBEEF_UNSIGNED_STR), 0,
		"Expected byte not written");

	/*******************/
	len = sprintf(buffer, "%d", (int) DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_SIGNED_STR),
		"Expected bytes not written");

	zassert_equal(strcmp(buffer, DEADBEEF_SIGNED_STR), 0, NULL);

	/*******************/
	len = sprintf(buffer, "%o", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_OCTAL_STR),
		"Expected bytes not written");
	zassert_equal(strcmp(buffer, DEADBEEF_OCTAL_STR), 0,
		"Expected bytes not written");

	/*******************/
	len = sprintf(buffer, "%#o", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_OCTAL_ALT_STR),
		"Expected bytes not written");
	zassert_equal(strcmp(buffer, DEADBEEF_OCTAL_ALT_STR), 0,
		"Expected bytes not written");

	/*******************/
	len = sprintf(buffer, "%#x", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_LHEX_ALT_STR),
		"Expected bytes not written");
	zassert_equal(strcmp(buffer, DEADBEEF_LHEX_ALT_STR), 0,
		"Expected bytes not written");

	/*******************/
	len = sprintf(buffer, "%#X", DEADBEEF);
	zassert_equal(len, strlen(DEADBEEF_UHEX_ALT_STR),
		"Expected bytes not written");
	zassert_equal(strcmp(buffer, DEADBEEF_UHEX_ALT_STR), 0,
		"Expected bytes not written");

	/*******************/

	len = sprintf(buffer, "%+d", 1);
	zassert_equal(len, 2,
			"Expected bytes not written");
	zassert_equal(strcmp(buffer, "+1"), 0,
			"Expected bytes not written");

}

/**
 *
 * @brief Test sprintf with strings
 *
 */

void sprintf_stringtest(void)
{
	int  len;
	char buffer[400];

	sprintf(buffer, "%%");
	zassert_equal(strcmp(buffer, "%"), 0, NULL);

	sprintf(buffer, "%c", 't');
	zassert_equal(strcmp(buffer, "t"), 0, NULL);

	sprintf(buffer, "%s", "short string");
	zassert_equal(strcmp(buffer, "short string"), 0,
			" It is expecting short string");

	len = sprintf(buffer, "%s", REALLY_LONG_STRING);
#ifndef CONFIG_NEWLIB_LIBC
	zassert_equal(len, PRINTF_MAX_STRING_LENGTH, NULL);
#endif
	zassert_equal(strncmp(buffer, REALLY_LONG_STRING,
			PRINTF_MAX_STRING_LENGTH), 0, NULL);
}
