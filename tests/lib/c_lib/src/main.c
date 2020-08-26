/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file test access to the minimal C libraries
 *
 * This module verifies that the various minimal C libraries can be used.
 *
 * IMPORTANT: The module only ensures that each supported library is present,
 * and that a bare minimum of its functionality is operating correctly. It does
 * NOT guarantee that ALL standards-defined functionality is present, nor does
 * it guarantee that ALL functionality provided is working correctly.
 */

#include <zephyr.h>
#include <sys/__assert.h>
#include <ztest.h>

#include <limits.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/* Recent GCC's are issuing a warning for the truncated strncpy()
 * below (the static source string is longer than the locally-defined
 * destination array).  That's exactly the case we're testing, so turn
 * it off.
 */
#if defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

/*
 * variables used during limits library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long long_max = LONG_MAX;
volatile long long_one = 1L;

/**
 *
 * @brief Test implementation-defined constants library
 *
 */

void test_limits(void)
{

	zassert_true((long_max + long_one == LONG_MIN), NULL);
}

static ssize_t foobar(void)
{
	return -1;
}

void test_ssize_t(void)
{
	zassert_true(foobar() < 0, NULL);
}

/**
 *
 * @brief Test boolean types and values library
 *
 */

void test_stdbool(void)
{

	zassert_true((true == 1), "true value");
	zassert_true((false == 0), "false value");
}

/*
 * variables used during stddef library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long long_variable;
volatile size_t size_of_long_variable = sizeof(long_variable);

/**
 *
 * @brief Test standard type definitions library
 *
 */

void test_stddef(void)
{
#ifdef CONFIG_64BIT
	zassert_true((size_of_long_variable == 8), "sizeof");
#else
	zassert_true((size_of_long_variable == 4), "sizeof");
#endif
}

/*
 * variables used during stdint library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile uint8_t unsigned_byte = 0xff;
volatile uint32_t unsigned_int = 0xffffff00;

/**
 *
 * @brief Test integer types library
 *
 */

void test_stdint(void)
{
	zassert_true((unsigned_int + unsigned_byte + 1u == 0U), NULL);

}

/*
 * variables used during string library testing
 */

#define BUFSIZE 10

char buffer[BUFSIZE];

/**
 *
 * @brief Test string memset
 *
 */

void test_memset(void)
{
	(void)memset(buffer, 'a', BUFSIZE);
	zassert_true((buffer[0] == 'a'), "memset");
	zassert_true((buffer[BUFSIZE - 1] == 'a'), "memset");
}

/**
 *
 * @brief Test string length function
 *
 */

void test_strlen(void)
{
	(void)memset(buffer, '\0', BUFSIZE);
	(void)memset(buffer, 'b', 5); /* 5 is BUFSIZE / 2 */
	zassert_equal(strlen(buffer), 5, "strlen");
}

/**
 *
 * @brief Test string compare function
 *
 */

void test_strcmp(void)
{
	strcpy(buffer, "eeeee");

	zassert_true((strcmp(buffer, "fffff") < 0), "strcmp less ...");
	zassert_true((strcmp(buffer, "eeeee") == 0), "strcmp equal ...");
	zassert_true((strcmp(buffer, "ddddd") > 0), "strcmp greater ...");
}

/**
 *
 * @brief Test string N compare function
 *
 */

void test_strncmp(void)
{
	static const char pattern[] = "eeeeeeeeeeee";

	/* Note we don't want to count the final \0 that sizeof will */
	__ASSERT_NO_MSG(sizeof(pattern) - 1 > BUFSIZE);
	memcpy(buffer, pattern, BUFSIZE);

	zassert_true((strncmp(buffer, "fffff", 0) == 0), "strncmp 0");
	zassert_true((strncmp(buffer, "eeeff", 3) == 0), "strncmp 3");
	zassert_true((strncmp(buffer, "eeeeeeeeeeeff", BUFSIZE) == 0),
		     "strncmp 10");
}


/**
 *
 * @brief Test string copy function
 *
 */

void test_strcpy(void)
{
	(void)memset(buffer, '\0', BUFSIZE);
	strcpy(buffer, "10 chars!\0");

	zassert_true((strcmp(buffer, "10 chars!\0") == 0), "strcpy");
}

/**
 *
 * @brief Test string N copy function
 *
 */

void test_strncpy(void)
{
	int ret;

	(void)memset(buffer, '\0', BUFSIZE);
	strncpy(buffer, "This is over 10 characters", BUFSIZE);

	/* Purposely different values */
	ret = strncmp(buffer, "This is over 20 characters", BUFSIZE);
	zassert_true((ret == 0), "strncpy");

}

/**
 *
 * @brief Test string scanning function
 *
 */

void test_strchr(void)
{
	char *rs = NULL;
	int ret;

	(void)memset(buffer, '\0', BUFSIZE);
	strncpy(buffer, "Copy 10", BUFSIZE);

	rs = strchr(buffer, '1');

	zassert_not_null(rs, "strchr");


	ret = strncmp(rs, "10", 2);
	zassert_true((ret == 0), "strchr");

}

/**
 *
 * @brief Test string prefix match functions
 *
 */

void test_strxspn(void)
{
	const char *empty = "";
	const char *cset = "abc";

	zassert_true(strspn("", empty) == 0U, "strspn empty empty");
	zassert_true(strcspn("", empty) == 0U, "strcspn empty empty");

	zassert_true(strspn("abde", cset) == 2U, "strspn match");
	zassert_true(strcspn("abde", cset) == 0U, "strcspn nomatch");

	zassert_true(strspn("da", cset) == 0U, "strspn nomatch");
	zassert_true(strcspn("da", cset) == 1U, "strcspn match");

	zassert_true(strspn("abac", cset) == 4U, "strspn all");
	zassert_true(strcspn("defg", cset) == 4U, "strcspn all");
}

/**
 *
 * @brief Test memory comparison function
 *
 */

void test_memcmp(void)
{
	int ret;
	unsigned char m1[5] = { 1, 2, 3, 4, 5 };
	unsigned char m2[5] = { 1, 2, 3, 4, 6 };


	ret = memcmp(m1, m2, 4);
	zassert_true((ret == 0), "memcmp 4");

	ret = memcmp(m1, m2, 5);
	zassert_true((ret != 0), "memcmp 5");
}

/**
 *
 * @brief Test binary search function
 *
 */

int cmp_func(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

void test_bsearch(void)
{
	void *result = NULL;
	int arr[5] = { 2, 5, 20, 50, 60 };
	size_t size = ARRAY_SIZE(arr);
	int key = 30;

	result = (int *)bsearch(&key, arr, size, sizeof(int), cmp_func);
	zassert_is_null(result, "bsearch -key not found");

	key = 60;
	result = (int *)bsearch(&key, arr, size, sizeof(int), cmp_func);
	zassert_not_null(result, "bsearch -key found");
}

/**
 *
 * @brief Test abs function
 *
 */

void test_abs(void)
{
	int val = -5, value = 5;

	zassert_equal(abs(val), 5, "abs -5");
	zassert_equal(abs(value), 5, "abs 5");
}

/**
 *
 * @brief Test atoi function
 *
 */
void test_atoi(void)
{
	zassert_equal(atoi("123"), 123, "atoi error");
	zassert_equal(atoi("2c5"), 2, "atoi error");
	zassert_equal(atoi("acd"), 0, "atoi error");
	zassert_equal(atoi(" "), 0, "atoi error");
	zassert_equal(atoi(""), 0, "atoi error");
	zassert_equal(atoi("3-4e"), 3, "atoi error");
	zassert_equal(atoi("8+1c"), 8, "atoi error");
}

/**
 *
 * @brief Test value type
 *
 * @details This function check the char type,
 * and verify the return value.
 *
 * @see isalnum(), isalpha(), isdigit(), isgraph(),
 * isprint(), isspace(), isupper(), isxdigit().
 *
 */
void test_checktype(void)
{
	static const char exp_alnum[] =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static const char exp_alpha[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static const char exp_digit[] = "0123456789";
	static const char exp_graph[] =
		"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	static const char exp_print[] =
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	static const char exp_space[] = {"\x9\xa\xb\xc\xd\x20"};

	static const char exp_upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const char exp_xdigit[] = "0123456789ABCDEFabcdef";
	char buf[128];
	char *ptr = buf;

	for (int i = 0; i < 128; i++) {
		if (isalnum(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_alnum), 0, "isalnum error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isalpha(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_alpha), 0, "isalpha error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isdigit(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_digit), 0, "isdigit error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isgraph(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_graph), 0, "isgraph error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isprint(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_print), 0, "isprint error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isupper(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_upper), 0, "isupper error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isspace(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_space), 0, "isspace error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isxdigit(i))
			*ptr++ = i;
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_xdigit), 0, "isxdigit error");
}

/**
 *
 * @brief Test character operation
 *
 * @see memchr(), memcpy(), memmove().
 *
 */
void test_memstr(void)
{
	static const char str[] = "testfunction";
	int ch1 = 'e', ch2 = 'z';
	char arr[10], move_arr[6] = "12123";
	static const char num[6] = "12121";

	zassert_equal(memchr(str, ch1, strlen(str)), str + 1,
		     "memchr 'testfunction' 'e' ");
	zassert_equal(memchr(str, ch2, strlen(str)), NULL,
		     "memchr 'testfunction' 'z'");

	zassert_equal(memcpy(arr, num, sizeof(num)), arr, "memcpy error");
	zassert_equal(memcmp(num, arr, sizeof(num)), 0,
		     "memcpy failed");

	zassert_equal(memmove(move_arr + 2, move_arr, 3), move_arr + 2,
		     "memmove error");
	zassert_equal(memcmp(num, move_arr, 6), 0,
		     "memmove failed");
}

/**
 *
 * @brief test str operate functions
 *
 * @see strcat(), strcspn(), strncat().
 *
 */
void test_str_operate(void)
{
	char str1[10] = "aabbcc", ncat[10] = "ddee";
	char *str2 = "b";
	char *str3 = "d";
	int ret;
	char *ptr;

	zassert_not_null(strcat(str1, str3), "strcat false");
	zassert_equal(strcmp(str1, "aabbccd"), 0, "test strcat failed");

	ret = strcspn(str1, str2);
	zassert_equal(ret, 2, "strcspn failed");
	ret = strcspn(str1, str3);
	zassert_equal(ret, 6, "strcspn not found str");

	zassert_true(strncat(ncat, str1, 2), "strncat failed");
	zassert_equal(strcmp(ncat, "ddeeaa"), 0, "strncat failed");

	zassert_is_null(strrchr(ncat, 'z'),
		       "strrchr not found this word. failed");
	ptr = strrchr(ncat, 'e');
	zassert_equal(strcmp(ptr, "eaa"), 0, "strrchr failed");

	zassert_is_null(strstr(str1, "xyz"), "strstr aabbccd with xyz failed");
	zassert_not_null(strstr(str1, str2), "strstr aabbccd with b failed");
}

/**
 *
 * @brief test strtol function
 *
 */
void test_strtoxl(void)
{
	char buf1[20] = "10379aegi";
	char buf2[20] = "10379aegi";
	char buf3[20] = "010379aegi";
	char buf4[20] = "0x10379aegi";
	char *stop = NULL;
	long ret;
	unsigned long retul;

	ret = strtol(buf3, &stop, 8);
	zassert_equal(ret, 543, "strtol base = 8 failed");
	ret = strtol(buf1, &stop, 10);
	zassert_equal(ret, 10379, "strtol base = 10 failed");
	retul = strtoul(buf2, &stop, 10);
	zassert_equal(ret, 10379, "strtoul base = 10 failed");
	ret = strtol(buf4, &stop, 16);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
}

/**
 *
 * @brief test convert function
 *
 */
void test_tolower_toupper(void)
{
	static const char test[] = "Az09Za\t\n#!";
	static const char toup[] = "AZ09ZA\t\n#!";
	static const char tolw[] = "az09za\t\n#!";
	char up[11];
	char lw[11];
	int i = 0;

	for (; i < strlen(test); i++) {
		up[i] = toupper(test[i]);
		lw[i] = tolower(test[i]);
	}
	lw[i] = up[i] = '\0';

	zassert_equal(strcmp(up, toup), 0, "toupper error");
	zassert_equal(strcmp(lw, tolw), 0, "tolower error");
}

void test_strtok_r_do(char *str, char *sep, int tlen,
		      const char * const *toks, bool expect)
{
	int len = 0;
	char *state, *tok, buf[64+1] = {0};

	strncpy(buf, str, 64);

	tok = strtok_r(buf, sep, &state);
	while (tok && len < tlen) {
		if (strcmp(tok, toks[len]) != 0) {
			break;
		}
		tok = strtok_r(NULL, sep, &state);
		len++;
	}
	if (expect) {
		zassert_equal(len, tlen,
			      "strtok_r error '%s' / '%s'", str, sep);
	} else {
		zassert_not_equal(len, tlen,
				  "strtok_r error '%s' / '%s'", str, sep);
	}
}

void test_strtok_r(void)
{
	static const char * const tc01[] = { "1", "2", "3", "4", "5" };

	test_strtok_r_do("1,2,3,4,5",           ",",  5, tc01, true);
	test_strtok_r_do(",, 1 ,2  ,3   4,5  ", ", ", 5, tc01, true);
	test_strtok_r_do("1,,,2 3,,,4 5",       ", ", 5, tc01, true);
	test_strtok_r_do("1,2 3,,,4 5  ",       ", ", 5, tc01, true);
	test_strtok_r_do("0,1,,,2 3,,,4 5",     ", ", 5, tc01, false);
	test_strtok_r_do("1,,,2 3,,,4 5",       ",",  5, tc01, false);
	test_strtok_r_do("A,,,2,3,,,4 5",       ",",  5, tc01, false);
	test_strtok_r_do("1,,,2,3,,,",          ",",  5, tc01, false);
	test_strtok_r_do("1|2|3,4|5",           "| ", 5, tc01, false);
}

void test_main(void)
{
	ztest_test_suite(test_c_lib,
			 ztest_unit_test(test_limits),
			 ztest_unit_test(test_ssize_t),
			 ztest_unit_test(test_stdbool),
			 ztest_unit_test(test_stddef),
			 ztest_unit_test(test_stdint),
			 ztest_unit_test(test_memcmp),
			 ztest_unit_test(test_strchr),
			 ztest_unit_test(test_strcpy),
			 ztest_unit_test(test_strncpy),
			 ztest_unit_test(test_memset),
			 ztest_unit_test(test_strlen),
			 ztest_unit_test(test_strcmp),
			 ztest_unit_test(test_strxspn),
			 ztest_unit_test(test_bsearch),
			 ztest_unit_test(test_abs),
			 ztest_unit_test(test_atoi),
			 ztest_unit_test(test_checktype),
			 ztest_unit_test(test_memstr),
			 ztest_unit_test(test_str_operate),
			 ztest_unit_test(test_tolower_toupper),
			 ztest_unit_test(test_strtok_r)
			 );
	ztest_run_test_suite(test_c_lib);
}
