/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
#include <misc/__assert.h>
#include <ztest.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>

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

void limits_test(void)
{

	zassert_true((long_max + long_one == LONG_MIN), NULL);
}

/**
 *
 * @brief Test boolean types and values library
 *
 */

void stdbool_test(void)
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

void stddef_test(void)
{

	zassert_true((size_of_long_variable == 4), "sizeof");
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

void stdint_test(void)
{
	zassert_true((unsigned_int + unsigned_byte + 1u == 0), NULL);

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

void memset_test(void)
{
	memset(buffer, 'a', BUFSIZE);
	zassert_true((buffer[0] == 'a'), "memset");
	zassert_true((buffer[BUFSIZE - 1] == 'a'), "memset");
}

/**
 *
 * @brief Test string length function
 *
 */

void strlen_test(void)
{
	memset(buffer, '\0', BUFSIZE);
	memset(buffer, 'b', 5); /* 5 is BUFSIZE / 2 */
	zassert_equal(strlen(buffer), 5, "strlen");
}

/**
 *
 * @brief Test string compare function
 *
 */

void strcmp_test(void)
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

void strncmp_test(void)
{
	const char pattern[] = "eeeeeeeeeeee";

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

void strcpy_test(void)
{
	memset(buffer, '\0', BUFSIZE);
	strcpy(buffer, "10 chars!\0");

	zassert_true((strcmp(buffer, "10 chars!\0") == 0), "strcpy");
}

/**
 *
 * @brief Test string N copy function
 *
 */

void strncpy_test(void)
{
	int ret;

	memset(buffer, '\0', BUFSIZE);
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

void strchr_test(void)
{
	char *rs = NULL;
	int ret;

	memset(buffer, '\0', BUFSIZE);
	strncpy(buffer, "Copy 10", BUFSIZE);

	rs = strchr(buffer, '1');

	zassert_not_null(rs, "strchr");


	ret = strncmp(rs, "10", 2);
	zassert_true((ret == 0), "strchr");

}

/**
 *
 * @brief Test memory comparison function
 *
 */

void memcmp_test(void)
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
 * @brief Test string operations library
 */

void string_test(void)
{

	memset_test();
	strlen_test();
	strcmp_test();
	strcpy_test();
	strncpy_test();
	strncmp_test();
	strchr_test();
	memcmp_test();
}
