/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifdef CONFIG_EXTENDED_VARIANT_BOARD_SETTING
#define EXTENDED_BOARD_A 1
#else
#define EXTENDED_BOARD_A 0
#endif

#ifdef CONFIG_EXTENDED_VARIANT_BOARD_ONE_SETTING_PROMPTLESS
#define EXTENDED_BOARD_ONE_B 1
#else
#define EXTENDED_BOARD_ONE_B 0
#endif

#ifdef CONFIG_EXTENDED_VARIANT_BOARD_TWO_SETTING_PROMPTLESS
#define EXTENDED_BOARD_TWO_C 1
#else
#define EXTENDED_BOARD_TWO_C 0
#endif

#ifdef CONFIG_EXTENDED_VARIANT_BOARD_SETTING_DEFCONFIG
#define EXTENDED_BOARD_D 1
#else
#define EXTENDED_BOARD_D 0
#endif

#ifdef CONFIG_BASE_BOARD_SETTING
#define BASE_BOARD_CONFIG 1
#else
#define BASE_BOARD_CONFIG 0
#endif

#ifdef CONFIG_SOC_MPS2_AN521_CPUTEST
#define EXTENDED_SOC 1
#else
#define EXTENDED_SOC 0
#endif

ZTEST_SUITE(soc_board_extend, NULL, NULL, NULL, NULL, NULL);

#if CONFIG_BOARD_NATIVE_SIM
ZTEST(soc_board_extend, test_native_sim_extend)
{
#if CONFIG_BOARD_NATIVE_SIM_NATIVE_ONE
	zassert_true(EXTENDED_BOARD_A, "Expected extended board to be set");
	zassert_true(EXTENDED_BOARD_ONE_B, "Expected extended board to be set");
	zassert_false(EXTENDED_BOARD_TWO_C, "Did not expect extended board two to be set");
	zassert_true(EXTENDED_BOARD_D, "Expected extended board to be set");
	zassert_false(BASE_BOARD_CONFIG, "Did not expect base board to be set");
	zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_native_one)));
	zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_native_two)));
	zassert_false(DT_NODE_EXISTS(DT_PATH(adc)));
#elif CONFIG_BOARD_NATIVE_SIM_NATIVE_64_TWO
	zassert_true(EXTENDED_BOARD_A, "Expected extended board to be set");
	zassert_false(EXTENDED_BOARD_ONE_B, "Did not expect extended board one to be set");
	zassert_true(EXTENDED_BOARD_TWO_C, "Expected extended board to be set");
	zassert_true(EXTENDED_BOARD_D, "Expected extended board to be set");
	zassert_false(BASE_BOARD_CONFIG, "Did not expect base board to be set");
	zassert_false(DT_NODE_EXISTS(DT_PATH(added_by_native_one)));
	zassert_true(DT_NODE_EXISTS(DT_PATH(added_by_native_two)));
	zassert_false(DT_NODE_EXISTS(DT_PATH(adc)));
#else
	zassert_true(false, "Did not expect to build for a base native_sim board");
#endif
#elif CONFIG_BOARD_MPS2
ZTEST(soc_board_extend, test_an521_soc_extend)
{
#if CONFIG_BOARD_MPS2_AN521_CPUTEST
	zassert_true(EXTENDED_SOC, "Expected extended SoC to be set");
#elif CONFIG_BOARD_MPS2
	zassert_true(false, "Did not expect to build for a base mps2 board");
#endif

#else
ZTEST(soc_board_extend, test_failure)
{
	zassert_true(false, "Did not expect to build for a regular board");
#endif
}
