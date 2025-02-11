/*
 * Copyright (c) 2022 CSIRO
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(test_configdefault, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_configdefault, test_expectedvalues)
{
	zassert_true(IS_ENABLED(CONFIG_DEP_Y), "");
	zassert_false(IS_ENABLED(CONFIG_DEP_N), "");

	zassert_true(IS_ENABLED(CONFIG_SYM_Y_1), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_2), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_3), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_4), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_5), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_6), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_7), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_8), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_9), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_10), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_11), "");
	zassert_true(IS_ENABLED(CONFIG_SYM_Y_12), "");

	zassert_false(IS_ENABLED(CONFIG_SYM_N_1), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_2), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_3), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_4), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_5), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_6), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_7), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_8), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_9), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_10), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_11), "");
	zassert_false(IS_ENABLED(CONFIG_SYM_N_12), "");

	zassert_false(IS_ENABLED(CONFIG_SYM_INT_UNDEF), "");
	zassert_equal(1, CONFIG_SYM_INT_1, "");
	zassert_equal(2, CONFIG_SYM_INT_2, "");
	zassert_equal(3, CONFIG_SYM_INT_3, "");
	zassert_equal(4, CONFIG_SYM_INT_4, "");

	zassert_equal(0x20, CONFIG_SYM_HEX_20, "");
	zassert_mem_equal("TEST", CONFIG_SYM_STRING, strlen("TEST"), "");
}
