/* libraries.c - test access to the minimal C libraries */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module verifies that the various minimal C libraries can be used.

IMPORTANT: The module only ensures that each supported library is present,
and that a bare minimum of its functionality is operating correctly. It does
NOT guarantee that ALL standards-defined functionality is present, nor does
it guarantee that ALL functionality provided is working correctly.
 */

#include <zephyr.h>
#include <misc/__assert.h>
#include <tc_util.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * variables used during limits library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long longMax = LONG_MAX;
volatile long longOne = 1L;

/**
 *
 * @brief Test implementation-defined constants library
 *
 * @return TC_PASS or TC_FAIL
 */

int limitsTest(void)
{
	TC_PRINT("Testing limits.h library ...\n");

	if (longMax + longOne != LONG_MIN) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test boolean types and values library
 *
 * @return TC_PASS or TC_FAIL
 */

int stdboolTest(void)
{
	TC_PRINT("Testing stdbool.h library ...\n");

	if ((true != 1) || (false != 0)) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/*
 * variables used during stddef library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long longVariable;
volatile size_t sizeOfLongVariable = sizeof(longVariable);

/**
 *
 * @brief Test standard type definitions library
 *
 * @return TC_PASS or TC_FAIL
 */

int stddefTest(void)
{
	TC_PRINT("Testing stddef.h library ...\n");

	if (sizeOfLongVariable != 4) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/*
 * variables used during stdint library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile uint8_t unsignedByte = 0xff;
volatile uint32_t unsignedInt = 0xffffff00;

/**
 *
 * @brief Test integer types library
 *
 * @return TC_PASS or TC_FAIL
 */

int stdintTest(void)
{
	TC_PRINT("Testing stdint.h library ...\n");

	if (unsignedInt + unsignedByte + 1u != 0) {
		return TC_FAIL;
	}

	return TC_PASS;
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
 * @return TC_PASS or TC_FAIL
 */

int memset_test(void)
{
	TC_PRINT("\tmemset ...\t");

	memset(buffer, 'a', BUFSIZE);
	if (buffer[0] != 'a' || buffer[BUFSIZE-1] != 'a') {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test string length function
 *
 * @return TC_PASS or TC_FAIL
 */

int strlen_test(void)
{
	TC_PRINT("\tstrlen ...\t");

	memset(buffer, '\0', BUFSIZE);
	memset(buffer, 'b', 5); /* 5 is BUFSIZE / 2 */
	if (strlen(buffer) != 5) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test string compare function
 *
 * @return TC_PASS or TC_FAIL
 */

int strcmp_test(void)
{
	strcpy(buffer, "eeeee");

	TC_PRINT("\tstrcmp less ...\t");
	if (strcmp(buffer, "fffff") >= 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	TC_PRINT("\tstrcmp equal ...\t");
	if (strcmp(buffer, "eeeee") != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	TC_PRINT("\tstrcmp greater ...\t");
	if (strcmp(buffer, "ddddd") <= 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	return TC_PASS;
}

/**
 *
 * @brief Test string N compare function
 *
 * @return TC_PASS or TC_FAIL
 */

int strncmp_test(void)
{
	const char pattern[] = "eeeeeeeeeeee";

	/* Note we don't want to count the final \0 that sizeof will */
	__ASSERT_NO_MSG(sizeof(pattern) - 1 > BUFSIZE);
	memcpy(buffer, pattern, BUFSIZE);

	TC_PRINT("\tstrncmp 0 ...\t");
	if (strncmp(buffer, "fffff", 0) != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	TC_PRINT("\tstrncmp 3 ...\t");
	if (strncmp(buffer, "eeeff", 3) != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	TC_PRINT("\tstrncmp 10 ...\t");
	if (strncmp(buffer, "eeeeeeeeeeeff", BUFSIZE) != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT("passed\n");
	}

	return TC_PASS;
}


/**
 *
 * @brief Test string copy function
 *
 * @return TC_PASS or TC_FAIL
 */

int strcpy_test(void)
{
	TC_PRINT("\tstrcpy ...\t");

	memset(buffer, '\0', BUFSIZE);
	strcpy(buffer, "10 chars!\0");

	if (strcmp(buffer, "10 chars!\0") != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test string N copy function
 *
 * @return TC_PASS or TC_FAIL
 */

int strncpy_test(void)
{
	TC_PRINT("\tstrncpy ...\t");

	memset(buffer, '\0', BUFSIZE);
	strncpy(buffer, "This is over 10 characters", BUFSIZE);

	/* Purposely different values */
	if (strncmp(buffer, "This is over 20 characters", BUFSIZE) != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test string scanning function
 *
 * @return TC_PASS or TC_FAIL
 */

int strchr_test(void)
{
	char *rs = NULL;
	TC_PRINT("\tstrchr ...\t");

	memset(buffer, '\0', BUFSIZE);
	strncpy(buffer, "Copy 10", BUFSIZE);

	rs = strchr(buffer, '1');

	if (!rs) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	if (strncmp(rs, "10", 2) != 0) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test memory comparison function
 *
 * @return TC_PASS or TC_FAIL
 */

int memcmp_test(void)
{
	unsigned char m1[5] = { 1, 2, 3, 4, 5 };
	unsigned char m2[5] = { 1, 2, 3, 4, 6 };

	TC_PRINT("\tmemcmp ...\t");

	if (memcmp(m1, m2, 4)) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	if (!memcmp(m1, m2, 5)) {
		TC_PRINT("failed\n");
		return TC_FAIL;
	}

	TC_PRINT("passed\n");
	return TC_PASS;
}

/**
 *
 * @brief Test string operations library
 * * @return TC_PASS or TC_FAIL
 */

int stringTest(void)
{
	TC_PRINT("Testing string.h library ...\n");

	if (memset_test() || strlen_test() || strcmp_test() || strcpy_test() ||
		strncpy_test() || strncmp_test() || strchr_test() ||
		memcmp_test()) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Main task in the test suite
 *
 * This is the entry point to the main task used by the standard libraries test
 * suite. It tests each library in turn until a failure is detected or all
 * libraries have been tested successfully.
 *
 * @return TC_PASS or TC_FAIL
 */

int RegressionTask(void)
{
	TC_PRINT("Validating access to supported libraries\n");

	if (limitsTest() || stdboolTest() || stddefTest() ||
		stdintTest() || stringTest()) {
		TC_PRINT("Library validation failed\n");
		return TC_FAIL;
	}

	TC_PRINT("Validation complete\n");
	return TC_PASS;
}
