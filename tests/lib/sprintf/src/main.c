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

#include <zephyr/ztest.h>
#include <stdio.h>
#include <stdarg.h>

/**
 *
 * @brief Test implementation-defined constants library
 * @defgroup libc_api
 * @ingroup all_tests
 * @{
 *
 */

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

#define IS_MINIMAL_LIBC_NANO (IS_ENABLED(CONFIG_MINIMAL_LIBC) \
	      && IS_ENABLED(CONFIG_CBPRINTF_NANO))

#define IS_MINIMAL_LIBC_NOFP (IS_ENABLED(CONFIG_MINIMAL_LIBC) \
	      && !IS_ENABLED(CONFIG_CBPRINTF_FP_SUPPORT))

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

#ifdef CONFIG_BIG_ENDIAN
union raw_double_u {
	double d;
	struct {
		uint32_t fraction;
		uint32_t exponent;
	};
};
#else
union raw_double_u {
	double d;
	struct {
		uint32_t exponent;
		uint32_t fraction;
	};
};
#endif

static int WriteFrmtd_vf(FILE *stream, char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = vfprintf(stream, format, args);
	va_end(args);

	return ret;
}

/**
 *
 * @brief Test sprintf with doubles
 *
 */

#ifdef CONFIG_STDOUT_CONSOLE
ZTEST(sprintf, test_sprintf_double)
{
	char buffer[400];
	union raw_double_u var;


	/* Conversion not supported with minimal_libc without
	 * CBPRINTF_FP_SUPPORT.
	 *
	 * Conversion not supported without FPU except on native POSIX.
	 */
	if (IS_MINIMAL_LIBC_NOFP
	    || !(IS_ENABLED(CONFIG_FPU)
		 || IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX))) {
		ztest_test_skip();
		return;
	}

	var.exponent = 0x00000000;
	var.fraction = 0x7ff00000; /* Bit pattern for +INF (double) */
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

	var.exponent = 0x00000000;
	var.fraction = 0xfff00000; /* Bit pattern for -INF (double) */
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

	var.exponent = 0x00000000;
	var.fraction = 0x7ff80000; /* Bit pattern for NaN (double) */
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

	var.exponent = 0x00000000;
	var.fraction = 0xfff80000; /* Bit pattern for -NaN (double) */
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
		     "sprintf(1234.56789) - incorrect output '%s'\n", buffer);

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

	/* 3.872E-121 expressed as " 0.0...387" */
	sprintf(buffer, "% .380f", var.d);
	zassert_true((strlen(buffer) == 383),
		     "sprintf(<large output>) - incorrect length %d\n",
		     strlen(buffer));
	zassert_equal(strncmp(&buffer[119], "00003872", 8), 0,
		      "sprintf(<large output>) - misplaced value\n");
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

	var.exponent = 0x00000001;
	var.fraction = 0x00000000; /* smallest denormal value */
	sprintf(buffer, "%g", var.d);
#ifdef CONFIG_PICOLIBC
	zassert_true((strcmp(buffer, "5e-324") == 0),
		     "sprintf(5e-324) - incorrect "
		     "output '%s'\n", buffer);
#else
	zassert_true((strcmp(buffer, "4.94066e-324") == 0),
		     "sprintf(4.94066e-324) - incorrect "
		     "output '%s'\n", buffer);
#endif
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

ZTEST(sprintf, test_vsnprintf)
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

ZTEST(sprintf, test_vsprintf)
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

ZTEST(sprintf, test_snprintf)
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

ZTEST(sprintf, test_sprintf_misc)
{
	int count;
	char buffer[100];

	/*******************/
	sprintf(buffer, "%p", (void *) DEADBEEF);
	zassert_false((strcmp(buffer, DEADBEEF_PTR_STR) != 0),
		      "sprintf(%%p).  Expected '%s', got '%s'", DEADBEEF_PTR_STR, buffer);
	/*******************/
	if (IS_MINIMAL_LIBC_NANO) {
		TC_PRINT(" MINIMAL_LIBC+CPBPRINTF skipped tests\n");
	} else {
#ifndef CONFIG_PICOLIBC
		sprintf(buffer, "test data %n test data", &count);
		zassert_false((count != 10),
			      "sprintf(%%n).  Expected count to be %d, not %d",
			      10, count);

		zassert_false((strcmp(buffer, "test data  test data") != 0),
			      "sprintf(%%p).  Expected '%s', got '%s'",
			      "test data  test data", buffer);
#else
		/*
		 * Picolibc doesn't include %n support as it makes format string
		 * bugs a more serious security issue
		 */
		(void) count;
#endif


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
	}

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
ZTEST(sprintf, test_sprintf_integer)
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

	/* no upper-case hex support */
	if (!IS_MINIMAL_LIBC_NANO) {
		zassert_true((strcmp(buffer, DEADBEEF_UHEX_STR) == 0),
			     "sprintf(%%X).  Expected '%s', got '%s'\n",
			     DEADBEEF_UHEX_STR, buffer);
	}

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

	/* MINIMAL_LIBC+NANO doesn't support the following tests */
	if (IS_MINIMAL_LIBC_NANO) {
		TC_PRINT(" MINIMAL_LIBC+CBPRINTF_NANO skipped tests\n");
		return;
	}

	/*******************/
	len = sprintf(buffer, "%#o", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_OCTAL_ALT_STR)),
		     "sprintf(%%#o).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_OCTAL_ALT_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_OCTAL_ALT_STR) == 0),
		     "sprintf(%%#o).  Expected '%s', got '%s'\n",
		     DEADBEEF_OCTAL_ALT_STR, buffer);

	/*******************/
	len = sprintf(buffer, "%o", DEADBEEF);
	zassert_true((len == strlen(DEADBEEF_OCTAL_STR)),
		     "sprintf(%%#o).  Expected %zu bytes written, not %d\n",
		     strlen(DEADBEEF_OCTAL_STR), len);

	zassert_true((strcmp(buffer, DEADBEEF_OCTAL_STR) == 0),
		     "sprintf(%%o).  Expected '%s', got '%s'\n",
		     DEADBEEF_OCTAL_STR, buffer);

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

ZTEST(sprintf, test_sprintf_string)
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
 * @brief Test print function
 *
 * @see printf().
 *
 */
ZTEST(sprintf, test_print)
{
	int ret;

	ret = printf("%d\n", 3);
	zassert_equal(ret, 2, "printf failed!");

	ret = printf("");
	zassert_equal(ret, 0, "printf failed!");
}

/**
 *
 * @brief Test fprintf function
 *
 * @see fprintf().
 *
 */
ZTEST(sprintf, test_fprintf)
{
	int ret, i = 3;

	ret = fprintf(stdout, "%d\n", i);
	zassert_equal(ret, 2, "fprintf failed!");

	ret = fprintf(stdout, "");
	zassert_equal(ret, 0, "fprintf failed!");

#if !defined(CONFIG_PICOLIBC) && !defined(CONFIG_ARMCLANG_STD_LIBC)	/* this is UB */
	ret = fprintf(NULL, "%d", i);
	zassert_equal(ret, EOF, "fprintf failed!");
#endif
}


/**
 *
 * @brief Test vfprintf function
 *
 */

ZTEST(sprintf, test_vfprintf)
{
	int ret;

	ret = WriteFrmtd_vf(stdout, "This %0-d\n", 3);
	zassert_equal(ret, 7, "vfprintf \"This 3\" failed");

	ret = WriteFrmtd_vf(stdout,  "%9d\n", 3);
	zassert_equal(ret, 10, "vfprintf \"3\" failed");

	ret = WriteFrmtd_vf(stdout, "");
	zassert_equal(ret, 0, "vfprintf \"\" failed");

	ret = WriteFrmtd_vf(stdout, "/%%/%c/\n", 'a');
	zassert_equal(ret, 6, "vfprintf \'a\' failed");

	ret = WriteFrmtd_vf(stdout,  "11\n");
	zassert_equal(ret, 3, "vfprintf \"11\" failed");

#ifndef CONFIG_PICOLIBC	/* this is UB */
	ret = WriteFrmtd_vf(NULL,  "This %d", 3);
	zassert_equal(ret, EOF, "vfprintf \"This 3\" failed");
#endif
}

/**
 *
 * @brief Test vprintf function
 *
 */

static int WriteFrmtd_v(char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = vprintf(format, args);
	va_end(args);

	return ret;
}

ZTEST(sprintf, test_vprintf)
{
	int ret;

	ret = WriteFrmtd_v("This %d\n", 3);
	zassert_equal(ret, 7, "vprintf \"This 3\" failed");

	ret = WriteFrmtd_v("%9d\n", 3);
	zassert_equal(ret, 10, "vprintf \"3\" failed");

	ret = WriteFrmtd_v("");
	zassert_equal(ret, 0, "vprintf \"3\" failed");

	ret = WriteFrmtd_v("/%%/%c/\n", 'a');
	zassert_equal(ret, 6, "vprintf \'a\' failed");

	ret = WriteFrmtd_v("11\n");
	zassert_equal(ret, 3, "vprintf \"11\" failed");
}

/**
 *
 * @brief Test put function
 *
 * @see fputs(), puts(), fputc(), putc().
 */
ZTEST(sprintf, test_put)
{
	int ret;

	ret = fputs("This 3\n", stdout);
	zassert_equal(ret, 0, "fputs \"This 3\" failed");

	ret = fputs("This 3\n", stderr);
	zassert_equal(ret, 0, "fputs \"This 3\" failed");

#if !defined(CONFIG_PICOLIBC) && !defined(CONFIG_ARMCLANG_STD_LIBC)	/* this is UB */
	ret = fputs("This 3", NULL);
	zassert_equal(ret, EOF, "fputs \"This 3\" failed");
#endif

	ret = puts("This 3");
	zassert_equal(ret, 0, "puts \"This 3\" failed");

	ret = fputc('T', stdout);
	zassert_equal(ret, 84, "fputc \'T\' failed");

#if !defined(CONFIG_PICOLIBC) && !defined(CONFIG_ARMCLANG_STD_LIBC)	/* this is UB */
	ret = fputc('T', NULL);
	zassert_equal(ret, EOF, "fputc \'T\' failed");
#endif

	ret = putc('T', stdout);
	zassert_equal(ret, 84, "putc \'T\' failed");

#if !defined(CONFIG_PICOLIBC) && !defined(CONFIG_ARMCLANG_STD_LIBC)	/* this is UB */
	ret = putc('T', NULL);
	zassert_equal(ret, EOF, "putc \'T\' failed");
#endif

	ret = fputc('T', stderr);
	zassert_equal(ret, 84, "fputc \'T\' failed");

	ret = fputc('T', stdin);
	zassert_equal(ret, EOF, "fputc to stdin");
}

/**
 *
 * @brief Test fwrite function
 *
 */
ZTEST(sprintf, test_fwrite)
{
	int ret;

	ret = fwrite("This 3", 0, 0, stdout);
	zassert_equal(ret, 0, "fwrite failed!");

	ret = fwrite("This 3", 0, 4, stdout);
	zassert_equal(ret, 0, "fwrite failed!");

	ret = fwrite("This 3", 4, 4, stdout);
	zassert_equal(ret, 4, "fwrite failed!");

	ret = fwrite("This 3", 4, 4, stdin);
	zassert_equal(ret, 0, "fwrite failed!");
}

/**
 *
 * @brief Test stdout_hook_default() function
 *
 * @details When CONFIG_STDOUT_CONSOLE=n the default
 * stdout hook function _stdout_hook_default() returns EOF.
 */

#else
ZTEST(sprintf, test_EOF)
{
	int ret;

	ret = fputc('T', stdout);
	zassert_equal(ret, EOF, "fputc \'T\' failed");

	ret = fputs("This 3", stdout);
	zassert_equal(ret, EOF, "fputs \"This 3\" failed");

	ret = puts("This 3");
	zassert_equal(ret, EOF, "puts \"This 3\" failed");

	ret = WriteFrmtd_vf(stdout, "This %d", 3);
	zassert_equal(ret, EOF, "vfprintf \"3\" failed");
}
#endif

/**
 * @}
 */

/**
 *
 * @brief Test entry point
 *
 */

ZTEST_SUITE(sprintf, NULL, NULL, NULL, NULL, NULL);
