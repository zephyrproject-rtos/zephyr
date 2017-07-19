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

#include <tc_util.h>
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
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int sprintf_double_test(void)
{
	char buffer[100];
	union raw_double_u var;
	int  status = TC_PASS;

	var.u1 = 0x00000000;
	var.u2 = 0x7ff00000;    /* Bit pattern for +INF (double) */
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "+INF") != 0) {
		TC_ERROR("sprintf(+INF) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	var.u1 = 0x00000000;
	var.u2 = 0xfff00000;    /* Bit pattern for -INF (double) */
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "-INF") != 0) {
		TC_ERROR("sprintf(-INF) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	var.u1 = 0x00000000;
	var.u2 = 0x7ff80000;    /* Bit pattern for NaN (double) */
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "NaN") != 0) {
		TC_ERROR("sprintf(NaN) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	var.d = 1.0;
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "1.000000") != 0) {
		TC_ERROR("sprintf(1.0) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%+f", var.d);
	if (strcmp(buffer, "+1.000000") != 0) {
		TC_ERROR("sprintf(+1.0) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%.2f", var.d);
	if (strcmp(buffer, "1.00") != 0) {
		TC_ERROR("sprintf(1.00) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%.*f", 11, var.d);
	if (strcmp(buffer, "1.00000000000") != 0) {
		TC_ERROR("sprintf(1.00000000000) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%12f", var.d);
	if (strcmp(buffer, "    1.000000") != 0) {
		TC_ERROR("sprintf(    1.000000) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%-12f", var.d);
	if (strcmp(buffer, "1.000000    ") != 0) {
		TC_ERROR("sprintf(1.000000    ) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%012f", var.d);
	if (strcmp(buffer, "00001.000000") != 0) {
		TC_ERROR("sprintf(00001.000000) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	var.d = -1.0;
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "-1.000000") != 0) {
		TC_ERROR("sprintf(-1.0) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	var.d = 1234.56789;
	sprintf(buffer, "%f", var.d);
	if (strcmp(buffer, "1234.567890") != 0) {
		TC_ERROR("sprintf(-1.0) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	var.d = 1234.0;
	sprintf(buffer, "%e", var.d);
	if (strcmp(buffer, "1.234000e+003") != 0) {
		TC_ERROR("sprintf(1.234000e+003) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%E", var.d);
	if (strcmp(buffer, "1.234000E+003") != 0) {
		TC_ERROR("sprintf(1.234000E+003) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	var.d = 0.1234;
	sprintf(buffer, "%e", var.d);
	if (strcmp(buffer, "1.234000e-001") != 0) {
		TC_ERROR("sprintf(1.234000e-001) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%E", var.d);
	if (strcmp(buffer, "1.234000E-001") != 0) {
		TC_ERROR("sprintf(1.234000E-001) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	var.d = 1234000000.0;
	sprintf(buffer, "%g", var.d);
	if (strcmp(buffer, "1.234e+009") != 0) {
		TC_ERROR("sprintf(1.234e+009) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%G", var.d);
	if (strcmp(buffer, "1.234E+009") != 0) {
		TC_ERROR("sprintf(1.234E+009) - incorrect "
			"output '%s'\n", buffer);
		status = TC_FAIL;
	}


	return status;
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
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int vsnprintf_test(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];

	/*******************/
	buffer[0] = '\0';
	len = tvsnprintf(buffer, 0, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("vsnprintf(%%x).  Expected return value %zu, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "") != 0) {
		TC_ERROR("vsnprintf(%%x).  Expected '%s', got '%s'\n",
				 "", buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = tvsnprintf(buffer, 4, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("vsnprintf(%%x).  Expected return value %zu, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "dea") != 0) {
		TC_ERROR("vsnprintf(%%x).  Expected '%s', got '%s'\n",
				 "dea", buffer);
		status = TC_FAIL;
	}

	return status;
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
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int vsprintf_test(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];

	/*******************/
	len = tvsprintf(buffer, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("sprintf(%%x).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_LHEX_STR) != 0) {
		TC_ERROR("sprintf(%%x).  Expected '%s', got '%s'\n",
				 DEADBEEF_LHEX_STR, buffer);
		status = TC_FAIL;
	}

	return status;
}

/**
 *
 * @brief Test the snprintf() routine
 *
 * This routine does not aim to test the same underlying functionality as
 * sprintfTest().  Instead it tries to limit it to functionality specific to
 * snprintf().
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int snprintf_test(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];


	/*******************/
	buffer[0] = '\0';
	len = snprintf(buffer, 0, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("snprintf(%%x).  Expected return value %zu, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "") != 0) {
		TC_ERROR("snprintf(%%x).  Expected '%s', got '%s'\n",
				 "", buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = snprintf(buffer, 4, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("snprintf(%%x).  Expected return value %zu, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "dea") != 0) {
		TC_ERROR("snprintf(%%x).  Expected '%s', got '%s'\n",
				 "dea", buffer);
		status = TC_FAIL;
	}

	return status;
}

/**
 *
 * @brief Test the sprintf() routine with miscellaneous specifiers
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int sprintf_misc_test(void)
{
	int  status = TC_PASS;
	int  count;
	char buffer[100];

	/*******************/
	sprintf(buffer, "%p", (void *) DEADBEEF);
	if (strcmp(buffer, DEADBEEF_PTR_STR) != 0) {
		TC_ERROR("sprintf(%%p).  Expected '%s', got '%s'\n",
				 DEADBEEF_PTR_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "test data %n test data", &count);
	if (count != 10) {
		TC_ERROR("sprintf(%%n).  Expected count to be %d, not %d\n",
				 10, count);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "test data  test data") != 0) {
		TC_ERROR("sprintf(%%p).  Expected '%s', got '%s'\n",
				 "test data  test data", buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "%*d", 10, 1234);
	if (strcmp(buffer, "      1234") != 0) {
		TC_ERROR("sprintf(%%p).  Expected '%s', got '%s'\n",
				 "      1234", buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "%*d", -10, 1234);
	if (strcmp(buffer, "1234      ") != 0) {
		TC_ERROR("sprintf(%%p).  Expected '%s', got '%s'\n",
				 "1234      ", buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "% d", 1234);
	if (strcmp(buffer, " 1234") != 0) {
		TC_ERROR("sprintf(%% d). Expected '%s', got '%s'\n",
				 " 1234", buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "%hx", (unsigned short)1234);
	if (strcmp("4d2", buffer) != 0) {
		TC_ERROR("sprintf(%%hx).  Expected '4d2', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	sprintf(buffer, "%lx", 1234ul);
	if (strcmp("4d2", buffer) != 0) {
		TC_ERROR("sprintf(%%lx).  Expected '4d2', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	return status;
}

/**
 *
 * @brief Test the sprintf() routine with integers
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int sprintf_integer_test(void)
{
	int  status = TC_PASS;
	int  len;
	char buffer[100];

	/*******************/

	/* Note: prints hex numbers in 8 characters */
	len = sprintf(buffer, "%x", 0x11);
	if (len != 2) {
		TC_ERROR("sprintf(%%x). "
			"Expected 2 bytes written, not %d\n", len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "11") != 0) {
		TC_ERROR("sprintf(%%x). "
			"Expected '%s', got '%s'\n", "11", buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("sprintf(%%x).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_LHEX_STR) != 0) {
		TC_ERROR("sprintf(%%x).  Expected '%s', got '%s'\n",
				 DEADBEEF_LHEX_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%X", DEADBEEF);
	if (len != strlen(DEADBEEF_UHEX_STR)) {
		TC_ERROR("sprintf(%%X).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_UHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_UHEX_STR) != 0) {
		TC_ERROR("sprintf(%%X).  Expected '%s', got '%s'\n",
				 DEADBEEF_UHEX_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%u", DEADBEEF);
	if (len != strlen(DEADBEEF_UNSIGNED_STR)) {
		TC_ERROR("sprintf(%%u).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_UNSIGNED_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_UNSIGNED_STR) != 0) {
		TC_ERROR("sprintf(%%u).  Expected '%s', got '%s'\n",
				 DEADBEEF_UNSIGNED_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%d", (int) DEADBEEF);
	if (len != strlen(DEADBEEF_SIGNED_STR)) {
		TC_ERROR("sprintf(%%d).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_SIGNED_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_SIGNED_STR) != 0) {
		TC_ERROR("sprintf(%%d).  Expected '%s', got '%s'\n",
				 DEADBEEF_SIGNED_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%o", DEADBEEF);
	if (len != strlen(DEADBEEF_OCTAL_STR)) {
		TC_ERROR("sprintf(%%o).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_OCTAL_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_OCTAL_STR) != 0) {
		TC_ERROR("sprintf(%%o).  Expected '%s', got '%s'\n",
				 DEADBEEF_OCTAL_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%#o", DEADBEEF);
	if (len != strlen(DEADBEEF_OCTAL_ALT_STR)) {
		TC_ERROR("sprintf(%%#o).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_OCTAL_ALT_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_OCTAL_ALT_STR) != 0) {
		TC_ERROR("sprintf(%%#o).  Expected '%s', got '%s'\n",
				 DEADBEEF_OCTAL_ALT_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%#x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_ALT_STR)) {
		TC_ERROR("sprintf(%%#x).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_LHEX_ALT_STR), len);
		status = TC_FAIL;
	}
	if (strcmp(buffer, DEADBEEF_LHEX_ALT_STR) != 0) {
		TC_ERROR("sprintf(%%#x).  Expected '%s', got '%s'\n",
				 DEADBEEF_LHEX_ALT_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%#X", DEADBEEF);
	if (len != strlen(DEADBEEF_UHEX_ALT_STR)) {
		TC_ERROR("sprintf(%%#X).  Expected %zu bytes written, not %d\n",
				 strlen(DEADBEEF_UHEX_ALT_STR), len);
		status = TC_FAIL;
	}
	if (strcmp(buffer, DEADBEEF_UHEX_ALT_STR) != 0) {
		TC_ERROR("sprintf(%%#X).  Expected '%s', got '%s'\n",
				 DEADBEEF_UHEX_ALT_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/

	len = sprintf(buffer, "%+d", 1);
	if (len != 2) {
		TC_ERROR("sprintf(%%+d).  Expected %d bytes written, not %d\n",
				 2, len);
		status = TC_FAIL;
	}
	if (strcmp(buffer, "+1") != 0) {
		TC_ERROR("sprintf(%%+d). Expected '+1', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	return status;
}

/**
 *
 * @brief Test sprintf with strings
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int sprintf_stringtest(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[400];

	sprintf(buffer, "%%");
	if (strcmp(buffer, "%") != 0) {
		TC_ERROR("sprintf(%%).  Expected '%%', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%c", 't');
	if (strcmp(buffer, "t") != 0) {
		TC_ERROR("sprintf(%%c).  Expected 't', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%s", "short string");
	if (strcmp(buffer, "short string") != 0) {
		TC_ERROR("sprintf(%%s). "
			"Expected 'short string', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	len = sprintf(buffer, "%s", REALLY_LONG_STRING);
#ifndef CONFIG_NEWLIB_LIBC
	if (len != PRINTF_MAX_STRING_LENGTH) {
		TC_ERROR("Internals changed. "
			"Max string length no longer %d got %d\n",
				 PRINTF_MAX_STRING_LENGTH, len);
		status = TC_FAIL;
	}
#endif
	if (strncmp(buffer, REALLY_LONG_STRING,
			PRINTF_MAX_STRING_LENGTH) != 0) {
		TC_ERROR("First %d characters of REALLY_LONG_STRING "
			"not printed!\n",
				 PRINTF_MAX_STRING_LENGTH);
		status = TC_FAIL;
	}

	return status;
}

/**
 *
 * @brief Test entry point
 *
 * @return N/A
 */

void main(void)
{
	int  status = TC_PASS;

	TC_START("Test sprintf APIs\n");

	PRINT_LINE;

	TC_PRINT("Testing sprintf() with integers ....\n");
	if (sprintf_integer_test() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing snprintf() ....\n");
	if (snprintf_test() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing vsprintf() ....\n");
	if (vsprintf_test() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing vsnprintf() ....\n");
	if (vsnprintf_test() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing sprintf() with strings ....\n");
	if (sprintf_stringtest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing sprintf() with misc options ....\n");
	if (sprintf_misc_test() != TC_PASS) {
		status = TC_FAIL;
	}

#ifdef CONFIG_FLOAT
	TC_PRINT("Testing sprintf() with doubles ....\n");
	if (sprintf_double_test() != TC_PASS) {
		status = TC_FAIL;
	}
#endif /* CONFIG_FLOAT */

	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
