/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifdef CONFIG_SOC_FOLDER_TEST_STRING
#define STRING_OUTPUT CONFIG_SOC_FOLDER_TEST_STRING
#else
#error "Invalid test configuration"
#endif

#ifndef CONFIG_TEST_TYPE
#error "Invalid test configuration"
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_BOARD
#define INCLUDED_BOARD 1
#else
#define INCLUDED_BOARD 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_BOARD_SUFFIX
#define INCLUDED_BOARD_SUFFIX 1
#else
#define INCLUDED_BOARD_SUFFIX 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_BOARD_QUALIFIERS
#define INCLUDED_BOARD_QUALIFIERS 1
#else
#define INCLUDED_BOARD_QUALIFIERS 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_BOARD_OTHER
#define INCLUDED_BOARD_OTHER 1
#else
#define INCLUDED_BOARD_OTHER 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_SOC
#define INCLUDED_SOC 1
#else
#define INCLUDED_SOC 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_SOC_SUFFIX
#define INCLUDED_SOC_SUFFIX 1
#else
#define INCLUDED_SOC_SUFFIX 0
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_SOC_OTHER
#define INCLUDED_SOC_OTHER 1
#else
#define INCLUDED_SOC_OTHER 0
#endif

#if CONFIG_TEST_TYPE == 0
/* Default test */
ZTEST(soc_folder_kconfig, test_default)
{
	zassert_false(INCLUDED_BOARD_SUFFIX, "Did not expect board suffix config to be present");

#ifdef CONFIG_BOARD_NATIVE_SIM_NATIVE_64
	zassert_false(INCLUDED_BOARD, "Did not expect board config to be present");
	zassert_true(INCLUDED_BOARD_QUALIFIERS, "Expected board qualifier config to be present");
	zassert_mem_equal(STRING_OUTPUT, "five", strlen("five"), "Expected string to match");
#else
	zassert_true(INCLUDED_BOARD, "Expected board config to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier config to be present");
	zassert_mem_equal(STRING_OUTPUT, "two", strlen("two"), "Expected string to match");
#endif

	zassert_false(INCLUDED_BOARD_OTHER, "Did not expect board other config to be present");
	zassert_true(INCLUDED_SOC, "Expect soc config to be present");
	zassert_false(INCLUDED_SOC_SUFFIX, "Did not expect soc suffix config to be present");
	zassert_false(INCLUDED_SOC_OTHER, "Did not expect soc other config to be present");
}
#elif CONFIG_TEST_TYPE == 1
/* File suffix test */
ZTEST(soc_folder_kconfig, test_suffix)
{
	zassert_true(INCLUDED_BOARD_SUFFIX, "Expected board suffix config to be present");

	zassert_false(INCLUDED_BOARD, "Did not expect board config to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier config to be present");
	zassert_mem_equal(STRING_OUTPUT, "four", strlen("four"), "Expected string to match");
	zassert_false(INCLUDED_BOARD_OTHER, "Did not expect board other config to be present");
	zassert_false(INCLUDED_SOC, "Did not expect soc config to be present");
	zassert_true(INCLUDED_SOC_SUFFIX, "Expected soc suffix config to be present");
	zassert_false(INCLUDED_SOC_OTHER, "Did not expect soc other config to be present");
}
#elif CONFIG_TEST_TYPE == 2
/* Conf file test */
ZTEST(soc_folder_kconfig, test_conf)
{
	zassert_false(INCLUDED_BOARD_SUFFIX, "Did not expect board suffix config to be present");

#ifdef CONFIG_BOARD_NATIVE_SIM_NATIVE_64
	zassert_false(INCLUDED_BOARD, "Did not expect board config to be present");
	zassert_true(INCLUDED_BOARD_QUALIFIERS,
		     "Expected board qualifier config to be present");
#else
	zassert_true(INCLUDED_BOARD, "Expected board config to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		     "Did not expect board qualifier config to be present");
#endif

	zassert_mem_equal(STRING_OUTPUT, "three", strlen("three"), "Expected string to match");

	zassert_true(INCLUDED_BOARD_OTHER, "Expected board other config to be present");
	zassert_true(INCLUDED_SOC, "Expected soc config to be present");
	zassert_false(INCLUDED_SOC_SUFFIX, "Did not expect soc suffix config to be present");
	zassert_false(INCLUDED_SOC_OTHER, "Did not expect soc other config to be present");
}
#elif CONFIG_TEST_TYPE == 3
/* File suffix and conf file test */
ZTEST(soc_folder_kconfig, test_suffix_conf)
{
	zassert_true(INCLUDED_BOARD_SUFFIX, "Expected board suffix config to be present");
	zassert_false(INCLUDED_BOARD, "Did not expect board config to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier config to be present");
	zassert_mem_equal(STRING_OUTPUT, "three", strlen("three"), "Expected string to match");
	zassert_true(INCLUDED_BOARD_OTHER, "Expected board other config to be present");
	zassert_false(INCLUDED_SOC, "Did not expect soc config to be present");
	zassert_true(INCLUDED_SOC_SUFFIX, "Expected soc suffix config to be present");
	zassert_false(INCLUDED_SOC_OTHER, "Did not expect soc other config to be present");
}
#else
#error "Invalid test type"
#endif

ZTEST_SUITE(soc_folder_kconfig, NULL, NULL, NULL, NULL, NULL);
