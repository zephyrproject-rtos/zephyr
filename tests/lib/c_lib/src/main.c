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

	ztest_true((long_max + long_one == LONG_MIN), NULL);
}

static ssize_t foobar(void)
{
	return -1;
}

void test_ssize_t(void)
{
	ztest_true(foobar() < 0, NULL);
}

/**
 *
 * @brief Test boolean types and values library
 *
 */

void test_stdbool(void)
{

	ztest_true((true == 1), "true value");
	ztest_true((false == 0), "false value");
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
	ztest_true((size_of_long_variable == 8), "sizeof");
#else
	ztest_true((size_of_long_variable == 4), "sizeof");
#endif
}

/*
 * variables used during stdint library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile u8_t unsigned_byte = 0xff;
volatile u32_t unsigned_int = 0xffffff00;

/**
 *
 * @brief Test integer types library
 *
 */

void test_stdint(void)
{
	ztest_true((unsigned_int + unsigned_byte + 1u == 0U), NULL);

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
	ztest_true((buffer[0] == 'a'), "memset");
	ztest_true((buffer[BUFSIZE - 1] == 'a'), "memset");
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
	ztest_equal(strlen(buffer), 5, "strlen");
}

/**
 *
 * @brief Test string compare function
 *
 */

void test_strcmp(void)
{
	strcpy(buffer, "eeeee");

	ztest_true((strcmp(buffer, "fffff") < 0), "strcmp less ...");
	ztest_true((strcmp(buffer, "eeeee") == 0), "strcmp equal ...");
	ztest_true((strcmp(buffer, "ddddd") > 0), "strcmp greater ...");
}

/**
 *
 * @brief Test string N compare function
 *
 */

void test_strncmp(void)
{
	const char pattern[] = "eeeeeeeeeeee";

	/* Note we don't want to count the final \0 that sizeof will */
	__ASSERT_NO_MSG(sizeof(pattern) - 1 > BUFSIZE);
	memcpy(buffer, pattern, BUFSIZE);

	ztest_true((strncmp(buffer, "fffff", 0) == 0), "strncmp 0");
	ztest_true((strncmp(buffer, "eeeff", 3) == 0), "strncmp 3");
	ztest_true((strncmp(buffer, "eeeeeeeeeeeff", BUFSIZE) == 0),
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

	ztest_true((strcmp(buffer, "10 chars!\0") == 0), "strcpy");
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
	ztest_true((ret == 0), "strncpy");

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

	ztest_not_null(rs, "strchr");


	ret = strncmp(rs, "10", 2);
	ztest_true((ret == 0), "strchr");

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

	ztest_true(strspn("", empty) == 0U, "strspn empty empty");
	ztest_true(strcspn("", empty) == 0U, "strcspn empty empty");

	ztest_true(strspn("abde", cset) == 2U, "strspn match");
	ztest_true(strcspn("abde", cset) == 0U, "strcspn nomatch");

	ztest_true(strspn("da", cset) == 0U, "strspn nomatch");
	ztest_true(strcspn("da", cset) == 1U, "strcspn match");

	ztest_true(strspn("abac", cset) == 4U, "strspn all");
	ztest_true(strcspn("defg", cset) == 4U, "strcspn all");
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
	ztest_true((ret == 0), "memcmp 4");

	ret = memcmp(m1, m2, 5);
	ztest_true((ret != 0), "memcmp 5");
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
	ztest_is_null(result, "bsearch -key not found");

	key = 60;
	result = (int *)bsearch(&key, arr, size, sizeof(int), cmp_func);
	ztest_not_null(result, "bsearch -key found");
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
			 ztest_unit_test(test_bsearch)
			 );
	ztest_run_test_suite(test_c_lib);
}
