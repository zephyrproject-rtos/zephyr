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
void test_mb_api(void)
{
	arch_mb();
	arch_rmb();
	arch_wmb();
}
