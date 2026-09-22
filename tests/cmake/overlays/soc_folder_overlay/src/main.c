/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifndef CONFIG_TEST_TYPE
#error "Invalid test configuration"
#endif

#ifdef CONFIG_SOC_FOLDER_TEST_INCLUDE_APP
#define INCLUDED_APP 1
#else
#define INCLUDED_APP 0
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

#if CONFIG_TEST_TYPE == 0
/* Default test */
ZTEST(soc_folder_overlay, test_default)
{
	zassert_false(INCLUDED_APP, "Did not expect app overlay to be present");
	zassert_false(INCLUDED_BOARD_SUFFIX, "Did not expect board suffix overlay to be present");

#ifdef CONFIG_BOARD_NATIVE_SIM_NATIVE_64
	zassert_false(INCLUDED_BOARD, "Did not expect board overlay to be present");
	zassert_true(INCLUDED_BOARD_QUALIFIERS, "Expected board qualifier overlay to be present");
#else
	zassert_true(INCLUDED_BOARD, "Expected board overlay to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier overlay to be present");
#endif

	zassert_true(INCLUDED_SOC, "Expect soc overlay to be present");
	zassert_false(INCLUDED_SOC_SUFFIX, "Did not expect soc suffix overlay to be present");
}
#elif CONFIG_TEST_TYPE == 1
/* File suffix test */
ZTEST(soc_folder_overlay, test_suffix)
{
	zassert_false(INCLUDED_APP, "Did not expect app overlay to be present");
	zassert_true(INCLUDED_BOARD_SUFFIX, "Expected board suffix overlay to be present");
	zassert_false(INCLUDED_BOARD, "Did not expect board overlay to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier overlay to be present");
	zassert_false(INCLUDED_SOC, "Did not expect soc overlay to be present");
	zassert_true(INCLUDED_SOC_SUFFIX, "Expected soc suffix overlay to be present");
}
#elif CONFIG_TEST_TYPE == 2
/* App overlay test */
ZTEST(soc_folder_overlay, test_app)
{
	zassert_true(INCLUDED_APP, "Expected app overlay to be present");
	zassert_false(INCLUDED_BOARD_SUFFIX, "Did not expect board suffix overlay to be present");
	zassert_false(INCLUDED_BOARD, "Did not expect board overlay to be present");
	zassert_false(INCLUDED_BOARD_QUALIFIERS,
		      "Did not expect board qualifier overlay to be present");
	zassert_false(INCLUDED_SOC, "Did not expect soc overlay to be present");
	zassert_false(INCLUDED_SOC_SUFFIX, "Did not epect soc suffix overlay to be present");
}
#else
#error "Invalid test type"
#endif

ZTEST_SUITE(soc_folder_overlay, NULL, NULL, NULL, NULL, NULL);
