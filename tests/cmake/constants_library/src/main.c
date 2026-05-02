/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/test_constants_a.h>
#include <zephyr/test_constants_b.h>
#include <zephyr/test_constants_c.h>
#include <zephyr/test_constants_d.h>

/*
 * Dependency graph:
 *   B: standalone
 *   A -> B
 *   C -> B
 *   D -> A, B
 */

ZTEST(constants_library, test_constants)
{
	zassert_equal(TEST_CONST_B_SIZEOF, 10);
	zassert_equal(TEST_CONST_A_SIZEOF, 11);  /* B + 1 */
	zassert_equal(TEST_CONST_C_SIZEOF, 110); /* B + 100 */
	zassert_equal(TEST_CONST_D_SIZEOF, 21);  /* A + B = 11 + 10 */
}

ZTEST_SUITE(constants_library, NULL, NULL, NULL, NULL, NULL);
