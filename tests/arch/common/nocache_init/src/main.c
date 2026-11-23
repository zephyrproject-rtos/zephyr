/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Andrei-Edward Popa
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/linker/section_tags.h>
#include <zephyr/ztest.h>

ZTEST(nocache_init, test_initial_values)
{
	static volatile uint32_t initialized __nocache_load = 0x12345678U;
	static volatile uint32_t zeroed __nocache;

	zassert_equal(initialized, 0x12345678U, "Uncached initializer was overwritten");
	zassert_equal(zeroed, 0U, "Uncached BSS was not zeroed");
}

ZTEST_SUITE(nocache_init, NULL, NULL, NULL, NULL, NULL);
