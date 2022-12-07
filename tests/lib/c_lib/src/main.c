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

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/ztest.h>

#include <limits.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <zephyr/ztest_error_hook.h>
#ifdef CONFIG_PICOLIBC
#include <unistd.h>
#endif

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

/* Recent GCC's are issuing a warning for the truncated strncpy()
 * below (the static source string is longer than the locally-defined
 * destination array).  That's exactly the case we're testing, so turn
 * it off.
 */
#if defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

ZTEST_SUITE(test_c_lib, NULL, NULL, NULL, NULL, NULL);

/*
 * variables used during limits library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long long_max = LONG_MAX;
volatile long long_one = 1L;

/**
 *
 * @brief Test implementation-defined constants library
 * @defgroup libc_api
 * @ingroup all_tests
 * @{
 *
 */

ZTEST(test_c_lib, test_limits)
{

	zassert_true((long_max + long_one == LONG_MIN));
}

static ssize_t foobar(void)
{
	return -1;
}

ZTEST(test_c_lib, test_ssize_t)
{
	zassert_true(foobar() < 0);
}

/**
 *
 * @brief Test boolean types and values library
 *
 */
ZTEST(test_c_lib, test_stdbool)
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
ZTEST(test_c_lib, test_stddef)
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
ZTEST(test_c_lib, test_stdint)
{
	zassert_true((unsigned_int + unsigned_byte + 1u == 0U));

#if (UINT8_C(1) == 1)			\
	&& (INT8_C(-1) == -1)		\
	&& (UINT16_C(2) == 2)		\
	&& (INT16_C(-2) == -2)		\
	&& (UINT32_C(4) == 4)		\
	&& (INT32_C(-4) == -4)		\
	&& (UINT64_C(8) == 8)		\
	&& (INT64_C(-8) == -8)		\
	&& (UINTMAX_C(11) == 11)	\
	&& (INTMAX_C(-11) == -11)
	zassert_true(true);
#else
	zassert_true(false, "const int expr values ...");
#endif
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
ZTEST(test_c_lib, test_memset)
{
	int i, ret;
	const char set = 'a';
	int size = 0;

	memset(buffer, 0, 10);
	for (i = 0; i < 10; i++) {
		memset(buffer + i, set, size);
		memset(buffer + i, set, 1);
		ret = memcmp(buffer + i, &set, 1);
		zassert_true((ret == 0), "memset buffer a failed");
	}
}

/**
 *
 * @brief Test string length function
 *
 * @see strlen(), strnlen().
 *
 */
ZTEST(test_c_lib, test_strlen)
{
	(void)memset(buffer, '\0', BUFSIZE);
	(void)memset(buffer, 'b', 5); /* 5 is BUFSIZE / 2 */
	zassert_equal(strlen(buffer), 5, "strlen failed");

	zassert_equal(strnlen(buffer, 3), 3, "strnlen failed");
	zassert_equal(strnlen(buffer, BUFSIZE), 5, "strnlen failed");
}

/**
 *
 * @brief Test string compare function
 *
 * @see strcmp(), strncasecmp().
 *
 */
ZTEST(test_c_lib, test_strcmp)
{
	strcpy(buffer, "eeeee");
	char test = 0;

	zassert_true((strcmp(buffer, "fffff") < 0), "strcmp less ...");
	zassert_true((strcmp(buffer, "eeeee") == 0), "strcmp equal ...");
	zassert_true((strcmp(buffer, "ddddd") > 0), "strcmp greater ...");

	zassert_true((strncasecmp(buffer, "FFFFF", 3) < 0), "strncasecmp less ...");
	zassert_true((strncasecmp(buffer, "DDDDD", 3) > 0), "strncasecmp equal ...");
	zassert_true((strncasecmp(buffer, "EEEEE", 3) == 0), "strncasecmp greater ...");
	zassert_true((strncasecmp(&test, &test, 1) == 0), "strncasecmp failed");
}

/**
 *
 * @brief Test string N compare function
 *
 * @see strncmp().
 */
ZTEST(test_c_lib, test_strncmp)
{
	static const char pattern[] = "eeeeeeeeeeee";

	/* Note we don't want to count the final \0 that sizeof will */
	__ASSERT_NO_MSG(sizeof(pattern) - 1 > BUFSIZE);
	memcpy(buffer, pattern, BUFSIZE);

	zassert_true((strncmp(buffer, "fffff", 0) == 0), "strncmp 0");
	zassert_true((strncmp(buffer, "eeeff", 3) == 0), "strncmp 3");
	zassert_true((strncmp(buffer, "eeeff", 4) != 0), "strncmp 4");
	zassert_true((strncmp(buffer, "eeeeeeeeeeeff", BUFSIZE) == 0),
		     "strncmp 10");

	/* test compare the same strings */
	buffer[BUFSIZE - 1] = '\0';
	zassert_true((strncmp(buffer, buffer, BUFSIZE) == 0),
				 "strncmp 10 with \0");
}


/**
 *
 * @brief Test string copy function
 *
 * @see strcpy().
 */
ZTEST(test_c_lib, test_strcpy)
{
	(void)memset(buffer, '\0', BUFSIZE);
	strcpy(buffer, "10 chars!\0");

	zassert_true((strcmp(buffer, "10 chars!\0") == 0), "strcpy");
}

/**
 *
 * @brief Test string N copy function
 *
 * @see strncpy().
 */
ZTEST(test_c_lib, test_strncpy)
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
 * @see strchr().
 */
ZTEST(test_c_lib, test_strchr)
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
 * @see strspn(),strcspn().
 */
ZTEST(test_c_lib, test_strxspn)
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
 * @see memcmp()
 */
ZTEST(test_c_lib, test_memcmp)
{
	int ret;
	unsigned char m1[] = "a\0$def";
	unsigned char m2[] = "a\0$dhj";


	ret = memcmp(m1, m2, 4);
	zassert_true((ret == 0), "memcmp four characters failed");

	ret = memcmp(m1, m2, 5);
	zassert_true((ret != 0), "memcmp five characters failed");

	ret = memcmp(m1, m2, 0);
	zassert_true((ret == 0), "memcmp zero character failed");

	ret = memcmp(m1, m2, sizeof(m2));
	zassert_true((ret != 0), "memcmp 2 block of memory failed");
}

/**
 *
 * @brief Test binary search function
 *
 * @see bsearch()
 */
int cmp_func(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

ZTEST(test_c_lib, test_bsearch)
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
 * @see abs()
 */
ZTEST(test_c_lib, test_abs)
{
	int val = -5, value = 5;

	zassert_equal(abs(val), 5, "abs -5");
	zassert_equal(abs(value), 5, "abs 5");
}

/**
 *
 * @brief Test atoi function
 *
 * @see atoi()
 */
ZTEST(test_c_lib, test_atoi)
{
	zassert_equal(atoi("123"), 123, "atoi error");
	zassert_equal(atoi("2c5"), 2, "atoi error");
	zassert_equal(atoi("acd"), 0, "atoi error");
	zassert_equal(atoi(" "), 0, "atoi error");
	zassert_equal(atoi(""), 0, "atoi error");
	zassert_equal(atoi("3-4e"), 3, "atoi error");
	zassert_equal(atoi("8+1c"), 8, "atoi error");
	zassert_equal(atoi("+3"), 3, "atoi error");
	zassert_equal(atoi("-1"), -1, "atoi error");
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
ZTEST(test_c_lib, test_checktype)
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
		if (isalnum(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_alnum), 0, "isalnum error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isalpha(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_alpha), 0, "isalpha error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isdigit(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_digit), 0, "isdigit error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isgraph(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_graph), 0, "isgraph error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isprint(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_print), 0, "isprint error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isupper(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_upper), 0, "isupper error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isspace(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_space), 0, "isspace error");

	ptr = buf;
	for (int i = 0; i < 128; i++) {
		if (isxdigit(i)) {
			*ptr++ = i;
		}
	}
	*ptr = '\0';
	zassert_equal(strcmp(buf, exp_xdigit), 0, "isxdigit error");
}

/**
 * @brief Test memchr function
 *
 * @see memchr().
 */
ZTEST(test_c_lib, test_memchr)
{
	static const char str[] = "testfunction";

	/* verify the character inside the count scope */
	zassert_not_null(memchr(str, 'e', strlen(str)), "memchr serach e");
	zassert_not_null(memchr(str, '\0', strlen(str)+1), "memchr serach \0");

	/* verify when the count parm is zero */
	zassert_is_null(memchr(str, 't', 0), "memchr count 0 error");
	/* verify the wanted character outside the count scope */
	zassert_is_null(memchr(str, '\0', strlen(str)), "memchr scope error");
}

/**
 * @brief Test memcpy operation
 *
 * @see memcpy().
 */
ZTEST(test_c_lib, test_memcpy)
{
	/* make sure the buffer is word aligned */
	uintptr_t mem_dest[4] = {0};
	uintptr_t mem_src[4] = {0};
	unsigned char *mem_dest_tmp = NULL;
	unsigned char *mem_src_tmp = NULL;

	unsigned char *mem_dest_byte = (unsigned char *)mem_dest;
	unsigned char *mem_src_byte = (unsigned char *)mem_src;

	/* initialize source buffer in bytes */
	for (int i = 0; i < sizeof(mem_src); i++) {
		mem_src_byte[i] = i;
	}

	/* verify when dest in not word aligned */
	mem_dest_tmp = mem_dest_byte + 1;
	mem_src_tmp = mem_src_byte;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 10),
		mem_dest_tmp, "memcpy error");
	zassert_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");

	/* restore the environment */
	memset(mem_dest_byte, '\0', sizeof(mem_dest));
	/* verify when dest and src are all in not word aligned */
	mem_dest_tmp = mem_dest_byte + sizeof(uintptr_t) - 1;
	mem_src_tmp = mem_src_byte + sizeof(uintptr_t) - 1;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 10),
		mem_dest_tmp, "memcpy error");
	zassert_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");

	/* restore the environment */
	memset(mem_dest_byte, '\0', sizeof(mem_dest));
	/* verify when the copy count is zero, the copy will directly return */
	mem_dest_tmp = mem_dest_byte + sizeof(uintptr_t) - 1;
	mem_src_tmp = mem_src_byte + sizeof(uintptr_t) - 1;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 0),
		mem_dest_tmp, "memcpy error");
	zassert_not_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");
}

/**
 * @brief Test memmove operation
 *
 * @see memmove().
 */
ZTEST(test_c_lib, test_memmove)
{
	char move_buffer[6] = "12123";
	char move_new[6] = {0};
	static const char move_overlap[6] = "12121";

	/* verify <src> buffer overlaps with the start of the <dest> buffer */
	zassert_equal(memmove(move_buffer + 2, move_buffer, 3), move_buffer + 2,
		     "memmove error");
	zassert_equal(memcmp(move_overlap, move_buffer, sizeof(move_buffer)), 0,
		     "memmove failed");

	/* verify the buffer is not overlap, then forward-copy */
	zassert_equal(memmove(move_new, move_buffer, sizeof(move_buffer)), move_new,
		     "memmove error");
	zassert_equal(memcmp(move_new, move_buffer, sizeof(move_buffer)), 0,
		     "memmove failed");
}

/**
 *
 * @brief test str operate functions
 *
 * @see strcat(), strcspn(), strncat().
 *
 */
ZTEST(test_c_lib, test_str_operate)
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
	zassert_not_null(strncat(str1, str3, 2), "strncat failed");
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
	zassert_not_null(strncat(str1, str3, 1), "strncat failed");
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
	zassert_equal(strcmp(ncat, "ddeeaa"), 0, "strncat failed");

	zassert_is_null(strrchr(ncat, 'z'),
		       "strrchr not found this word. failed");
	ptr = strrchr(ncat, 'e');
	zassert_equal(strcmp(ptr, "eaa"), 0, "strrchr failed");

	zassert_is_null(strstr(str1, "ayz"), "strstr aabbccd with ayz failed");
	zassert_not_null(strstr(str1, str2), "strstr aabbccd with b succeed");
	zassert_not_null(strstr(str1, "bb"), "strstr aabbccd with bb succeed");
	zassert_not_null(strstr(str1, ""), "strstr aabbccd with \0 failed");
}

/**
 *
 * @brief test strtol function
 *
 * @detail   in 32bit system:
 *	when base is 10, [-2147483648..2147483647]
 *		   in 64bit system:
 *	when base is 10,
 *    [-9,223,372,036,854,775,808..9,223,372,036,854,775,807]
 *
 * @see strtol().
 *
 */
ZTEST(test_c_lib, test_strtol)
{
	static const char buf1[] = "+10379aegi";
	static const char buf2[] = "   -10379aegi";
	static const char buf3[] = "-010379aegi";
	static const char buf4[] = "0x10379aegi";
	static const char buf5[] = "0X10379aegi";
	static const char buf6[] = "01037aegi";
	static const char buf7[] = "1037aegi";
	static const char buf8[] = "++1037aegi";
	static const char buf9[] = "A1037aegi";
	static const char buf10[] = "a1037aegi";
	static const char str_normal[] = "-1011 This stopped it";
	static const char str_abnormal[] = "ABCDEFGH";
	char *stop = NULL;
	long ret;

	/* test function strtol() */
	ret = strtol(buf3, NULL, 8);
	zassert_equal(ret, -543, "strtol base = 8 failed");
	ret = strtol(buf1, NULL, 10);
	zassert_equal(ret, 10379, "strtol base = 10 failed");
	ret = strtol(buf2, NULL, 10);
	zassert_equal(ret, -10379, "strtol base = 10 failed");
	ret = strtol(buf4, NULL, 16);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtol(buf4, NULL, 0);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtol(buf5, NULL, 0);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtol(buf6, NULL, 0);
	zassert_equal(ret, 543, "strtol base = 8 failed");
	ret = strtol(buf7, NULL, 0);
	zassert_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtol(buf8, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtol(buf9, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtol(buf10, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");

	ret = strtol(str_normal, &stop, 10);
	zassert_equal(ret, -1011, "strtol base = 10 failed");
	zassert_true((strcmp(stop, " This stopped it") == 0),
		"strtol get stop failed");

	ret = strtol(str_abnormal, &stop, 0);
	zassert_equal(ret, 0, "strtol base = 0 failed");
	zassert_true((strcmp(stop, "ABCDEFGH") == 0),
		"strtol get stop failed");

#if LONG_MAX > 2147483647
	char border1[] = "-9223372036854775809";
	char border2[] = "+9223372036854775808";
	char border3[] = "+9223372036854775806";
	char border4[] = "922337203685477580000000";

	ret = strtol(border1, NULL, 10);
	zassert_equal(ret, LONG_MIN, "strtol base = 10 failed");
	ret = strtol(border2, NULL, 10);
	zassert_equal(ret, LONG_MAX, "strtol base = 10 failed");
	ret = strtol(border3, NULL, 10);
	zassert_equal(ret, 9223372036854775806, "strtol base = 10 failed");
	ret = strtol(border4, NULL, 10);
	zassert_equal(ret, LONG_MAX, "strtol base = 10 failed");
#else
	char border1[] = "-2147483649";
	char border2[] = "+2147483648";
	char border3[] = "+2147483646";
	char border4[] = "214748364000000";

	ret = strtol(border1, NULL, 10);
	zassert_equal(ret, LONG_MIN, "strtol base = 10 failed");
	ret = strtol(border2, NULL, 10);
	zassert_equal(ret, LONG_MAX, "strtol base = 10 failed");
	ret = strtol(border3, NULL, 10);
	zassert_equal(ret, 2147483646, "strtol base = 10 failed");
	ret = strtol(border4, NULL, 10);
	zassert_equal(ret, LONG_MAX, "strtol base = 10 failed");
#endif
}

/**
 *
 * @brief test strtoul function
 *
 * @see strtoul().
 *
 */
ZTEST(test_c_lib, test_strtoul)
{
	static const char buf1[] = "+10379aegi";
	static const char buf2[] = "   -10379aegi";
	static const char buf3[] = "-010379aegi";
	static const char buf4[] = "0x10379aegi";
	static const char buf5[] = "0X10379aegi";
	static const char buf6[] = "01037aegi";
	static const char buf7[] = "1037aegi";
	static const char buf8[] = "++1037aegi";
	static const char buf9[] = "A1037aegi";
	static const char buf10[] = "a1037aegi";
	static const char str_normal[] = "-1011 This stopped it";
	static const char str_abnormal[] = "ABCDEFGH";
	char *stop = NULL;
	long ret;

	/* test function strtol() */
	ret = strtoul(buf3, NULL, 8);
	zassert_equal(ret, -543, "strtol base = 8 failed");
	ret = strtoul(buf1, NULL, 10);
	zassert_equal(ret, 10379, "strtol base = 10 failed");
	ret = strtoul(buf2, NULL, 10);
	zassert_equal(ret, -10379, "strtol base = 10 failed");
	ret = strtoul(buf4, NULL, 16);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtoul(buf4, NULL, 0);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtoul(buf5, NULL, 0);
	zassert_equal(ret, 17004974, "strtol base = 16 failed");
	ret = strtoul(buf6, NULL, 0);
	zassert_equal(ret, 543, "strtol base = 8 failed");
	ret = strtoul(buf7, NULL, 0);
	zassert_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtoul(buf8, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtoul(buf9, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");
	ret = strtoul(buf10, NULL, 10);
	zassert_not_equal(ret, 1037, "strtol base = 10 failed");

	ret = strtoul(str_normal, &stop, 10);
	zassert_equal(ret, -1011, "strtol base = 10 failed");
	zassert_true((strcmp(stop, " This stopped it") == 0),
		"strtol get stop failed");

	ret = strtoul(str_abnormal, &stop, 0);
	zassert_equal(ret, 0, "strtol base = 0 failed");
	zassert_true((strcmp(stop, "ABCDEFGH") == 0),
		"strtol get stop failed");

#if LONG_MAX > 2147483647
	char border1[] = "18446744073709551615";
	char border2[] = "-18446744073709551615000";
	char border3[] = "18446744073709551619";

	ret = strtoul(border1, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");
	ret = strtoul(border2, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");
	ret = strtoul(border3, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");

#else
	char border1[] = "+4294967295";
	char border2[] = "-4294967295000";
	char border3[] = "+4294967299";

	ret = strtoul(border1, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");
	ret = strtoul(border2, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");
	ret = strtoul(border3, NULL, 10);
	zassert_equal(ret, ULONG_MAX, "strtol base = 10 failed");
#endif
}

/**
 *
 * @brief test strtoll function
 *
 * @see strtoll().
 *
 */
void test_strtoll(void)
{
	static const char buf1[] = "+10379aegi";
	static const char buf2[] = "   -10379aegi";
	static const char buf3[] = "-010379aegi";
	static const char buf4[] = "0x10379aegi";
	static const char buf5[] = "0X10379aegi";
	static const char buf6[] = "01037aegi";
	static const char buf7[] = "1037aegi";
	static const char buf8[] = "++1037aegi";
	static const char buf9[] = "A1037aegi";
	static const char buf10[] = "a1037aegi";
	static const char str_normal[] = "-1011 This stopped it";
	static const char str_abnormal[] = "ABCDEFGH";
	char *stop = NULL;
	long long ret;

	/* test function strtoll() */
	ret = strtoll(buf3, NULL, 8);
	zassert_equal(ret, -543, "strtoll base = 8 failed");
	ret = strtoll(buf1, NULL, 10);
	zassert_equal(ret, 10379, "strtoll base = 10 failed");
	ret = strtoll(buf2, NULL, 10);
	zassert_equal(ret, -10379, "strtoll base = 10 failed");
	ret = strtoll(buf4, NULL, 16);
	zassert_equal(ret, 17004974, "strtoll base = 16 failed");
	ret = strtoll(buf4, NULL, 0);
	zassert_equal(ret, 17004974, "strtoll base = 16 failed");
	ret = strtoll(buf5, NULL, 0);
	zassert_equal(ret, 17004974, "strtoll base = 16 failed");
	ret = strtoll(buf6, NULL, 0);
	zassert_equal(ret, 543, "strtoll base = 8 failed");
	ret = strtoll(buf7, NULL, 0);
	zassert_equal(ret, 1037, "strtoll base = 10 failed");
	ret = strtoll(buf8, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoll base = 10 failed");
	ret = strtoll(buf9, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoll base = 10 failed");
	ret = strtoll(buf10, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoll base = 10 failed");

	ret = strtoll(str_normal, &stop, 10);
	zassert_equal(ret, -1011, "strtoll base = 10 failed");
	zassert_true((strcmp(stop, " This stopped it") == 0), "strtoll get stop failed");

	ret = strtoll(str_abnormal, &stop, 0);
	zassert_equal(ret, 0, "strtoll base = 0 failed");
	zassert_true((strcmp(stop, "ABCDEFGH") == 0), "strtoll get stop failed");

	char border1[] = "-9223372036854775808";
	char border2[] = "+9223372036854775807";
	char border3[] = "+9223372036854775806";
	char border4[] = "922337203685477580000000";
	char border5[] = "0x0000000000000000000000000000000000001";
	char border6[] = "10000000000000000000000000000000000001";
	char border7[] = "-10000000000000000000000000000000000001";

	ret = strtoll(border1, NULL, 10);
	zassert_equal(ret, LLONG_MIN, "strtoll base = 10 failed");
	ret = strtoll(border2, NULL, 10);
	zassert_equal(ret, LLONG_MAX, "strtoll base = 10 failed");
	ret = strtoll(border3, NULL, 10);
	zassert_equal(ret, 9223372036854775806, "strtoll base = 10 failed");
	ret = strtoll(border4, NULL, 10);
	zassert_equal(ret, LLONG_MAX, "strtoll base = 10 failed");
	ret = strtoull(border5, NULL, 16);
	zassert_equal(ret, 1, "strtoull base = 16 failed, %s != 0x%x", border5, ret);
	ret = strtoull(border6, NULL, 10);
	zassert_equal(errno, ERANGE, "strtoull base = 10 failed, %s != %lld", border6, ret);
	ret = strtoull(border7, NULL, 10);
	zassert_equal(errno, ERANGE, "strtoull base = 10 failed, %s != %lld", border7, ret);
}

/**
 *
 * @brief test strtoull function
 *
 * @see strtoull().
 *
 */
void test_strtoull(void)
{
	static const char buf1[] = "+10379aegi";
	static const char buf2[] = "   -10379aegi";
	static const char buf3[] = "-010379aegi";
	static const char buf4[] = "0x10379aegi";
	static const char buf5[] = "0X10379aegi";
	static const char buf6[] = "01037aegi";
	static const char buf7[] = "1037aegi";
	static const char buf8[] = "++1037aegi";
	static const char buf9[] = "A1037aegi";
	static const char buf10[] = "a1037aegi";
	static const char str_normal[] = "-1011 This stopped it";
	static const char str_abnormal[] = "ABCDEFGH";
	char *stop = NULL;
	unsigned long long ret;

	/* test function strtoull() */
	ret = strtoull(buf3, NULL, 8);
	zassert_equal(ret, -543, "strtoull base = 8 failed");
	ret = strtoull(buf1, NULL, 10);
	zassert_equal(ret, 10379, "strtoull base = 10 failed");
	ret = strtoull(buf2, NULL, 10);
	zassert_equal(ret, -10379, "strtoull base = 10 failed");
	ret = strtoull(buf4, NULL, 16);
	zassert_equal(ret, 17004974, "strtoull base = 16 failed");
	ret = strtoull(buf4, NULL, 0);
	zassert_equal(ret, 17004974, "strtoull base = 16 failed");
	ret = strtoull(buf5, NULL, 0);
	zassert_equal(ret, 17004974, "strtoull base = 16 failed");
	ret = strtoull(buf6, NULL, 0);
	zassert_equal(ret, 543, "strtoull base = 8 failed");
	ret = strtoull(buf7, NULL, 0);
	zassert_equal(ret, 1037, "strtoull base = 10 failed");
	ret = strtoull(buf8, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoull base = 10 failed");
	ret = strtoull(buf9, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoull base = 10 failed");
	ret = strtoull(buf10, NULL, 10);
	zassert_not_equal(ret, 1037, "strtoull base = 10 failed");

	ret = strtoull(str_normal, &stop, 10);
	zassert_equal(ret, -1011, "strtoull base = 10 failed");
	zassert_true((strcmp(stop, " This stopped it") == 0), "strtoull get stop failed");

	ret = strtoull(str_abnormal, &stop, 0);
	zassert_equal(ret, 0, "strtoull base = 0 failed");
	zassert_true((strcmp(stop, "ABCDEFGH") == 0), "strtoull get stop failed");

	char border1[] = "+18446744073709551615";
	char border2[] = "-18446744073709551615000";
	char border3[] = "+18446744073709551619";
	char border4[] = "0x0000000000000000000000000000000000001";
	char border5[] = "10000000000000000000000000000000000001";
	char border6[] = "-10000000000000000000000000000000000001";

	ret = strtoull(border1, NULL, 10);
	zassert_equal(ret, ULLONG_MAX, "strtoull base = 10 failed");
	ret = strtoull(border2, NULL, 10);
	zassert_equal(ret, ULLONG_MAX, "strtoull base = 10 failed");
	ret = strtoull(border3, NULL, 10);
	zassert_equal(ret, ULLONG_MAX, "strtoull base = 10 failed");
	ret = strtoull(border4, NULL, 16);
	zassert_equal(ret, 1, "strtoull base = 16 failed, %s != 0x%x", border4, ret);
	ret = strtoull(border5, NULL, 10);
	zassert_equal(errno, ERANGE, "strtoull base = 10 failed, %s != %lld", border5, ret);
	ret = strtoull(border6, NULL, 10);
	zassert_equal(errno, ERANGE, "strtoull base = 10 failed, %s != %lld", border6, ret);
}

/**
 *
 * @brief test convert function
 *
 */
ZTEST(test_c_lib, test_tolower_toupper)
{
	static const char test[] = "Az09Za{#!";
	static const char toup[] = "AZ09ZA{#!";
	static const char tolw[] = "az09za{#!";
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

ZTEST(test_c_lib, test_strtok_r)
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

/**
 *
 * @brief Test time function
 *
 * @see gmtime(),gmtime_r().
 */
ZTEST(test_c_lib, test_time)
{
	time_t tests1 = 0;
	time_t tests2 = -5;
	time_t tests3 = -214748364800;
	time_t tests4 = 951868800;

	struct tm tp;

	zassert_not_null(gmtime(&tests1), "gmtime failed");
	zassert_not_null(gmtime(&tests2), "gmtime failed");

	tp.tm_wday = -5;
	zassert_not_null(gmtime_r(&tests3, &tp), "gmtime_r failed");
	zassert_not_null(gmtime_r(&tests4, &tp), "gmtime_r failed");
}

/**
 *
 * @brief Test rand function
 *
 */
ZTEST(test_c_lib, test_rand)
{
#ifndef CONFIG_PICOLIBC
	int a;

	a = rand();
	/* The default seed is 1 */
	zassert_equal(a, 1103527590, "rand failed");
#else
	ztest_test_skip();
#endif
}

/**
 *
 * @brief Test srand function
 *
 */
ZTEST(test_c_lib, test_srand)
{
#ifndef CONFIG_PICOLIBC
	int a;

	srand(0);
	a = rand();
	zassert_equal(a, 12345, "srand with seed 0 failed");

	srand(1);
	a = rand();
	zassert_equal(a, 1103527590, "srand with seed 1 failed");

	srand(10);
	a = rand();
	zassert_equal(a, 297746555, "srand with seed 10 failed");

	srand(UINT_MAX - 1);
	a = rand();
	zassert_equal(a, 2087949151, "srand with seed UINT_MAX - 1 failed");

	srand(UINT_MAX);
	a = rand();
	zassert_equal(a, 1043980748, "srand with seed UINT_MAX failed");
#else
	ztest_test_skip();
#endif
}

/**
 *
 * @brief Test rand function for reproducibility
 *
 */
ZTEST(test_c_lib, test_rand_reproducibility)
{
#ifndef CONFIG_PICOLIBC
	int a;
	int b;
	int c;

	srand(0);
	a = rand();
	zassert_equal(a, 12345, "srand with seed 0 failed");
	srand(0);
	b = rand();
	zassert_equal(b, 12345, "srand with seed 0 failed (2nd)");
	srand(0);
	c = rand();
	zassert_equal(c, 12345, "srand with seed 0 failed (3rd)");

	srand(1);
	a = rand();
	zassert_equal(a, 1103527590, "srand with seed 1 failed");
	srand(1);
	b = rand();
	zassert_equal(b, 1103527590, "srand with seed 1 failed (2nd)");
	srand(1);
	c = rand();
	zassert_equal(c, 1103527590, "srand with seed 1 failed (3rd)");

	srand(10);
	a = rand();
	zassert_equal(a, 297746555, "srand with seed 10 failed");
	srand(10);
	b = rand();
	zassert_equal(b, 297746555, "srand with seed 10 failed (2nd)");
	srand(10);
	c = rand();
	zassert_equal(c, 297746555, "srand with seed 10 failed (3rd)");

	srand(UINT_MAX - 1);
	a = rand();
	zassert_equal(a, 2087949151, "srand with seed UINT_MAX - 1 failed");
	srand(UINT_MAX - 1);
	b = rand();
	zassert_equal(b, 2087949151, "srand with seed UINT_MAX - 1 failed (2nd)");
	srand(UINT_MAX - 1);
	c = rand();
	zassert_equal(c, 2087949151, "srand with seed UINT_MAX - 1 failed (3rd)");

	srand(UINT_MAX);
	a = rand();
	zassert_equal(a, 1043980748, "srand with seed UINT_MAX failed");
	srand(UINT_MAX);
	b = rand();
	zassert_equal(b, 1043980748, "srand with seed UINT_MAX failed (2nd)");
	srand(UINT_MAX);
	c = rand();
	zassert_equal(c, 1043980748, "srand with seed UINT_MAX failed (3rd)");
#else
	ztest_test_skip();
#endif
}

/**
 *
 * @brief test abort functions
 *
 * @see abort().
 */
ZTEST(test_c_lib, test_abort)
{
	int a = 0;

	ztest_set_fault_valid(true);
	abort();
	zassert_equal(a, 0, "abort failed");
}

/**
 *
 * @brief test _exit functions
 *
 */
static void exit_program(void *p1, void *p2, void *p3)
{
	_exit(1);
}

ZTEST(test_c_lib, test_exit)
{
	int a = 0;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE, exit_program,
					NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));
	k_thread_abort(tid);
	zassert_equal(a, 0, "exit failed");
}
