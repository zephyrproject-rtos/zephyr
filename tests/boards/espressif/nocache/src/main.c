/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/linker/linker-defs.h>
#include <riscv/csr.h>

#include "soc/soc.h"
#include "cache.h"

#define TEST_PATTERN_A 0xA5A5A5A5U

/*
 * A cached alias of the same physical SRAM is obtained by subtracting the
 * non-cacheable offset from the __nocache address. The nocache section itself
 * is placed at the normal cached address and made non-cacheable by a PMA
 * entry, so the "cached alias" here is the uncached SRAM window, which lets us
 * observe the same physical bytes through a path that always bypasses the
 * cache. Comparing the two views proves the __nocache region is coherent
 * without any explicit cache maintenance.
 */
#define TO_UNCACHED_ALIAS(p) ((volatile uint32_t *)((uintptr_t)(p) + SOC_NON_CACHEABLE_OFFSET_SRAM))

static uint32_t __nocache nocache_word;
static volatile uint32_t __nocache nocache_buf[8];

/*
 * Force the linked .nocache section to a non-power-of-two length so the PMA
 * NAPOT rounding in nocache_region_init() is exercised. Without this the
 * section could happen to land on a power-of-two boundary and hide a rounding
 * bug that leaves part of the region cached.
 */
static volatile uint8_t __nocache nocache_odd[320];

/*
 * The PMA config CSR for the nocache entry must carry the non-cacheable
 * attribute. Reading it back is a direct, race-free proof that
 * nocache_region_init() applied the entry the driver relies on.
 */
ZTEST(nocache, test_pma_entry_applied)
{
	uint32_t cfg = RV_READ_CSR(CSR_PMACFG0 + NOCACHE_PMA_ENTRY);

	zassert_true(cfg & PMA_EN, "PMA entry %d not enabled (cfg=0x%08x)", NOCACHE_PMA_ENTRY, cfg);
	zassert_true(cfg & PMA_NONCACHEABLE, "PMA entry %d missing NONCACHEABLE (cfg=0x%08x)",
		     NOCACHE_PMA_ENTRY, cfg);
	zassert_equal(cfg & PMA_NAPOT, PMA_NAPOT, "PMA entry %d not in NAPOT mode (cfg=0x%08x)",
		      NOCACHE_PMA_ENTRY, cfg);
}

/*
 * The nocache symbols must describe a non-empty, cache-line aligned region
 * that lives in the DMA-reachable SRAM window (below SOC_DMA_HIGH), otherwise
 * a DMA engine could not reach buffers placed there.
 */
ZTEST(nocache, test_region_dma_reachable)
{
	uintptr_t start = (uintptr_t)_nocache_ram_start;
	uintptr_t end = (uintptr_t)_nocache_ram_end;
	size_t line = sys_cache_data_line_size_get();

	zassert_true(end > start, "empty nocache region");
	zassert_true(start >= SOC_DMA_LOW && end <= SOC_DMA_HIGH,
		     "nocache region [0x%lx, 0x%lx) outside DMA window", start, end);
	zassert_equal(start & (line - 1), 0, "nocache start not cache-line aligned");

	zassert_true((uintptr_t)&nocache_word >= start && (uintptr_t)&nocache_word < end,
		     "__nocache variable not in the nocache region");
}

/*
 * A store to the nocache region must reach RAM without an explicit flush. We
 * write a pattern, then invalidate the cache line: on a normal cached line the
 * dirty store would be discarded by the invalidate and the read-back would be
 * wrong, but a non-cacheable region has no dirty line to lose, so the value
 * survives. This isolates the cacheability of the region itself, independent of
 * any second address view.
 */
ZTEST(nocache, test_store_survives_invalidate)
{
	nocache_word = TEST_PATTERN_A;
	compiler_barrier();

	/* Drop any cache line covering the word without writing it back. */
	sys_cache_data_invd_range((void *)&nocache_word, sizeof(nocache_word));

	zassert_equal(nocache_word, TEST_PATTERN_A,
		      "store to nocache lost after invalidate (0x%08x) - region is cached",
		      nocache_word);
}

/*
 * The same coherency guarantee across a small buffer, checking every word so a
 * single mis-mapped cache line would be caught.
 */
ZTEST(nocache, test_buffer_coherency)
{
	volatile uint32_t *alias = TO_UNCACHED_ALIAS(nocache_buf);

	for (int i = 0; i < ARRAY_SIZE(nocache_buf); i++) {
		nocache_buf[i] = TEST_PATTERN_A ^ i;
	}
	compiler_barrier();

	for (int i = 0; i < ARRAY_SIZE(nocache_buf); i++) {
		zassert_equal(alias[i], TEST_PATTERN_A ^ i, "word %d incoherent: 0x%08x != 0x%08x",
			      i, alias[i], TEST_PATTERN_A ^ i);
	}
}

/*
 * Decode the NAPOT region the PMA entry actually describes and check it fully
 * contains the linked nocache section. A NAPOT address encodes the region size
 * as a run of low one-bits, so the region is [base, base + 2 * (lowbits + 1)).
 * This catches a rounding bug where the entry would cover a smaller or
 * misaligned window and leave part of the section cached.
 */
ZTEST(nocache, test_pma_region_covers_section)
{
	uintptr_t napot = RV_READ_CSR(CSR_PMAADDR0 + NOCACHE_PMA_ENTRY);
	uintptr_t start = (uintptr_t)_nocache_ram_start;
	uintptr_t end = (uintptr_t)_nocache_ram_end;
	/* NAPOT encodes the region as a run of low one-bits followed by a zero.
	 * The lowest zero bit gives half the region size (in address units), so
	 * the region is [base, base + 2 * lowzero) once shifted to bytes.
	 */
	uintptr_t lowzero = ~napot & (napot + 1);
	size_t region_size = (lowzero * 2) << PMA_SHIFT;
	uintptr_t region_base = (napot & ~(lowzero * 2 - 1)) << PMA_SHIFT;

	/* Touch the padding buffer so the linker keeps it and the section stays
	 * large enough to be non-power-of-two, exercising the NAPOT rounding.
	 */
	nocache_odd[0] = 1U;
	nocache_odd[ARRAY_SIZE(nocache_odd) - 1] = 1U;
	compiler_barrier();

	zassert_true(region_base <= start, "PMA base 0x%lx above section start 0x%lx", region_base,
		     start);
	zassert_true(region_base + region_size >= end,
		     "PMA region [0x%lx, 0x%lx) does not cover section end 0x%lx", region_base,
		     region_base + region_size, end);
}

ZTEST_SUITE(nocache, NULL, NULL, NULL, NULL, NULL);
