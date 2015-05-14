/* test_sprintf.c - test various sprintf functionality */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module contains the code for testing sprintf() functionality.
*/

/* includes */

#include <tc_util.h>
#include <stdio.h>

/* defines */

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
		"11111111111111111111111111111111111111111111111111111111111111111" \
		"22222222222222222222222222222222222222222222222222222222222222222" \
		"33333333333333333333333333333333333333333333333333333333333333333" \
		"44444444444444444444444444444444444444444444444444444444444444444" \
		"55555555555555555555555555555555555555555555555555555555555555555" \
		"66666666666666666666666666666666666666666666666666666666666666666"

#define PRINTF_MAX_STRING_LENGTH   200

/* typedefs */

typedef union {
	double  d;
	struct {
		uint32_t  u1;    /* This part contains the exponent */
		uint32_t  u2;    /* This part contains the fraction */
	};
} raw_double_u;

#ifdef CONFIG_FLOAT
/*******************************************************************************
*
* sprintfDoubleTest - test sprintf with doubles
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int sprintfDoubleTest(void)
{
	char buffer[100];
	raw_double_u var;
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
		TC_ERROR("sprintf(1.00000000000) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%12f", var.d);
	if (strcmp(buffer, "    1.000000") != 0) {
		TC_ERROR("sprintf(    1.000000) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%-12f", var.d);
	if (strcmp(buffer, "1.000000    ") != 0) {
		TC_ERROR("sprintf(1.000000    ) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%012f", var.d);
	if (strcmp(buffer, "00001.000000") != 0) {
		TC_ERROR("sprintf(00001.000000) - incorrect output '%s'\n", buffer);
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
		TC_ERROR("sprintf(1.234000e+003) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%E", var.d);
	if (strcmp(buffer, "1.234000E+003") != 0) {
		TC_ERROR("sprintf(1.234000E+003) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	var.d = 0.1234;
	sprintf(buffer, "%e", var.d);
	if (strcmp(buffer, "1.234000e-001") != 0) {
		TC_ERROR("sprintf(1.234000e-001) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%E", var.d);
	if (strcmp(buffer, "1.234000E-001") != 0) {
		TC_ERROR("sprintf(1.234000E-001) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/
	var.d = 1234000000.0;
	sprintf(buffer, "%g", var.d);
	if (strcmp(buffer, "1.234e+009") != 0) {
		TC_ERROR("sprintf(1.234e+009) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	sprintf(buffer, "%G", var.d);
	if (strcmp(buffer, "1.234E+009") != 0) {
		TC_ERROR("sprintf(1.234E+009) - incorrect output '%s'\n", buffer);
		status = TC_FAIL;
	}

	/*******************/

	/*
	 * VxMicro does not support size modifiers such as L.  As such, attempts to
	 * print a long double will NOT result in intuitively expected output.
	 * Until such time as they are supported (if ever) the output will remain
	 * counter-intuitive.  If they ever are supported then this test will have
	 * to be updated.
	 */

	sprintf(buffer, "%Lf", (long double) 1234567.0);
	if (strcmp("-0.000000", buffer) != 0) {
		TC_ERROR("sprintf(%%Lf).  Expected '-0.000000', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	return status;
}
#endif /* CONFIG_FLOAT */

/*******************************************************************************
*
* tvsnprintf - a test wrapper for vsnprintf()
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

/*******************************************************************************
*
* vsnprintfTest - test the vsprintf() routine
*
* This routine does not aim to test the same underlying functionality as
* sprintfTest().  Instead it tries to limit it to functionality specific to
* vsnprintf().  Instead of calling vsnprintf() directly, it invokes the wrapper
* routine tvsnprintf().
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int vsnprintfTest(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];

	/*
	 * VxMicro may be handling the string size in a non-standard manner.
	 * If a negative value is supplied for the string size, VxMicro converts
	 * it to 0x7fffffff--maximum integer size.  Since there is insufficient
	 * memory to test a string of that length, we just check that the string
	 * was fully written so that we can exercise the code path.
	 */
	buffer[0] = '\0';
	len = tvsnprintf(buffer, (size_t)(-4), "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("vsnprintf(%%x).  Expected return value %d, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_LHEX_STR) != 0) {
		TC_ERROR("vsnprintf(%%x).  Expected '%s', got '%s'\n",
				 DEADBEEF_LHEX_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	buffer[0] = '\0';
	len = tvsnprintf(buffer, 0, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("vsnprintf(%%x).  Expected return value %d, not %d\n",
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
		TC_ERROR("vsnprintf(%%x).  Expected return value %d, not %d\n",
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

/*******************************************************************************
*
* tvsprintf - a test wrapper for vsprintf()
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

/*******************************************************************************
*
* vsprintfTest - test the vsprintf() routine
*
* This routine does not aim to test the same underlying functionality as
* sprintfTest().  Instead it tries to limit it to functionality specific to
* vsprintf().
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int vsprintfTest(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];

	/*******************/
	len = tvsprintf(buffer, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("sprintf(%%x).  Expected %d bytes written, not %d\n",
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

/*******************************************************************************
*
* snprintfTest - test the snprintf() routine
*
* This routine does not aim to test the same underlying functionality as
* sprintfTest().  Instead it tries to limit it to functionality specific to
* snprintf().
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int snprintfTest(void)
{
	int  len;
	int  status = TC_PASS;
	char buffer[100];

	/*
	 * VxMicro may be handling the string size in a non-standard manner.
	 * If a negative value is supplied for the string size, VxMicro converts
	 * it to 0x7fffffff--maximum integer size.  Since there is insufficient
	 * memory to test a string of that length, we just check that the string
	 * was fully written so that we can exercise the code path.
	 */
	buffer[0] = '\0';
	len = snprintf(buffer, (size_t)(-4), "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("snprintf(%%x).  Expected return value %d, not %d\n",
				 strlen(DEADBEEF_LHEX_STR), len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, DEADBEEF_LHEX_STR) != 0) {
		TC_ERROR("snprintf(%%x).  Expected '%s', got '%s'\n",
				 DEADBEEF_LHEX_STR, buffer);
		status = TC_FAIL;
	}

	/*******************/
	buffer[0] = '\0';
	len = snprintf(buffer, 0, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("snprintf(%%x).  Expected return value %d, not %d\n",
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
		TC_ERROR("snprintf(%%x).  Expected return value %d, not %d\n",
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

/*******************************************************************************
*
* sprintfMiscTest - test the sprintf() routine with miscellaneous specifiers
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int sprintfMiscTest(void)
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
	sprintf(buffer, "%hx", 1234);
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

/*******************************************************************************
*
* sprintfIntegerTest - test the sprintf() routine with integers
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int sprintfIntegerTest(void)
{
	int  status = TC_PASS;
	int  len;
	char buffer[100];

	/*******************/

	/* Note: VxMicro prints hex numbers in 8 characters */
	len = sprintf(buffer, "%x", 0x11);
	if (len != 2) {
		TC_ERROR("sprintf(%%x).  Expected 2 bytes written, not %d\n", len);
		status = TC_FAIL;
	}

	if (strcmp(buffer, "11") != 0) {
		TC_ERROR("sprintf(%%x).  Expected '%s', got '%s'\n", "11", buffer);
		status = TC_FAIL;
	}

	/*******************/
	len = sprintf(buffer, "%x", DEADBEEF);
	if (len != strlen(DEADBEEF_LHEX_STR)) {
		TC_ERROR("sprintf(%%x).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%X).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%u).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%d).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%o).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%#o).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%#x).  Expected %d bytes written, not %d\n",
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
		TC_ERROR("sprintf(%%#X).  Expected %d bytes written, not %d\n",
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

/*******************************************************************************
*
* sprintfStringTest - test sprintf with strings
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int sprintfStringTest(void)
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
		TC_ERROR("sprintf(%%s).  Expected 'short string', got '%s'\n", buffer);
		status = TC_FAIL;
	}

	len = sprintf(buffer, "%s", REALLY_LONG_STRING);
	if (len != PRINTF_MAX_STRING_LENGTH) {
		TC_ERROR("Internals changed.  Max string length no longer %d\n",
				 PRINTF_MAX_STRING_LENGTH);
		status = TC_FAIL;
	}
	if (strncmp(buffer, REALLY_LONG_STRING, PRINTF_MAX_STRING_LENGTH) != 0) {
		TC_ERROR("First %d characters of REALLY_LONG_STRING not printed!\n",
				 PRINTF_MAX_STRING_LENGTH);
		status = TC_FAIL;
	}

	return status;
}

/*******************************************************************************
*
* RegressionTask - test entry point
*
* RETURNS: N/A
*/

void RegressionTask(void)
{
	int  status = TC_PASS;

	TC_START("Test Microkernel sprintf APIs\n");

	PRINT_LINE;

	TC_PRINT("Testing sprintf() with integers ....\n");
	if (sprintfIntegerTest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing snprintf() ....\n");
	if (snprintfTest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing vsprintf() ....\n");
	if (vsprintfTest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing vsnprintf() ....\n");
	if (vsnprintfTest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing sprintf() with strings ....\n");
	if (sprintfStringTest() != TC_PASS) {
		status = TC_FAIL;
	}

	TC_PRINT("Testing sprintf() with misc options ....\n");
	if (sprintfMiscTest() != TC_PASS) {
		status = TC_FAIL;
	}

#ifdef CONFIG_FLOAT
	TC_PRINT("Testing sprintf() with doubles ....\n");
	if (sprintfDoubleTest() != TC_PASS) {
		status = TC_FAIL;
	}
#endif /* CONFIG_FLOAT */

	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
