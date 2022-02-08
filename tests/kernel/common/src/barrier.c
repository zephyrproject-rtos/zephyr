/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel.h>

/**
 * @brief Test to verify API functions
 *
 * @ingroup kernel_mem_barrier_tests
 *
 * @details
 * Test Objective:
 * - To verify if all memory barrier API are defined
 *   to catch build regression.
 *
 * Testing techniques:
 * - Call all memory barrier API functions.
 *
 */
ZTEST(mb_api, test_mb_api)
{
	z_full_mb();
	z_read_mb();
	z_write_mb();
}
