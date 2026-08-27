/*
 * Copyright (c) 2026 Rishav Chakraborty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

static volatile uint8_t nocache_buf __nocache;
static volatile uint8_t regular_buf;

ZTEST(arc_nocache, test_placement)
{
	uintptr_t start = (uintptr_t)&_nocache_ram_start;
	uintptr_t end = (uintptr_t)&_nocache_ram_end;
	uintptr_t nc = (uintptr_t)&nocache_buf;
	uintptr_t reg = (uintptr_t)&regular_buf;

	zassert_true(nc >= start && nc < end,
		     "nocache_buf at 0x%lx outside .nocache [0x%lx, 0x%lx)", nc, start, end);
	zassert_false(reg >= start && reg < end,
		      "regular_buf at 0x%lx unexpectedly inside .nocache", reg);
}

ZTEST(arc_nocache, test_section_invariants)
{
	uintptr_t size = (uintptr_t)&_nocache_ram_size;
	uintptr_t start = (uintptr_t)&_nocache_ram_start;

	zassert_true(size > 0 && (size & (size - 1)) == 0,
		     ".nocache size 0x%lx is not a power of two", size);
	zassert_equal(start & (size - 1), 0, ".nocache start 0x%lx not aligned to size 0x%lx",
		      start, size);
}

ZTEST(arc_nocache, test_cache_bypass)
{
	nocache_buf = 0xAA;
	regular_buf = 0xAA;

	volatile uint8_t _nc = nocache_buf;
	(void)_nc;
	volatile uint8_t _reg = regular_buf;
	(void)_reg;

	__asm__ __volatile__("stb.di %0, [%1]" ::"r"((uint8_t)0xBB), "r"(&nocache_buf) : "memory");
	__asm__ __volatile__("stb.di %0, [%1]" ::"r"((uint8_t)0xBB), "r"(&regular_buf) : "memory");
	zassert_equal(nocache_buf, 0xBB, "nocache region appears to be cached");
	zassert_equal(regular_buf, 0xAA, "regular region read returned uncached value");
}

ZTEST_SUITE(arc_nocache, NULL, NULL, NULL, NULL, NULL);
