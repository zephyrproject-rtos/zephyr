/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>

/* CPU1 DLMU, cached (segment 9) and non-cached (segment B) */
#if defined(CONFIG_CPU_TC18)
#define DLMU_CPU1 0x90080000UL
#else
#define DLMU_CPU1 0x90010000UL
#endif
#define AREA_OFFSET 0x8000UL
#define NC_OFFSET   0x20000000UL

#define WORDS (4 * CONFIG_DCACHE_LINE_SIZE / sizeof(uint32_t))

static volatile uint32_t *const cached = (volatile uint32_t *)(DLMU_CPU1 + AREA_OFFSET);
static volatile uint32_t *const uncached =
	(volatile uint32_t *)(DLMU_CPU1 + AREA_OFFSET + NC_OFFSET);

static void fill(volatile uint32_t *p, uint32_t pattern)
{
	for (size_t i = 0; i < WORDS; i++) {
		p[i] = pattern + i;
	}
	barrier_dsync_fence_full();
}

static bool holds(volatile uint32_t *p, uint32_t pattern)
{
	for (size_t i = 0; i < WORDS; i++) {
		if (p[i] != pattern + i) {
			return false;
		}
	}

	return true;
}

static void *cache_setup(void)
{
	/* Initialize the area (and its ECC) through the non-cached view */
	fill(uncached, 0x10000000U);
	zassert_ok(sys_cache_data_invd_range((void *)cached, WORDS * sizeof(uint32_t)));
	sys_cache_data_enable();

	return NULL;
}

static void cache_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	sys_cache_data_disable();
}

ZTEST(tricore_cache, test_zero_size)
{
	/* A size of 0 must not walk any line */
	zassert_ok(sys_cache_data_flush_range((void *)cached, 0));
	zassert_ok(sys_cache_data_invd_range((void *)cached, 0));
	zassert_ok(sys_cache_data_flush_and_invd_range((void *)cached, 0));
}

ZTEST(tricore_cache, test_flush_range)
{
	fill(uncached, 0x20000000U);
	zassert_ok(sys_cache_data_invd_range((void *)cached, WORDS * sizeof(uint32_t)));
	zassert_true(holds(cached, 0x20000000U), "cached view does not see memory");

	/* Write-back: the update stays in the cache until it is flushed */
	fill(cached, 0x30000000U);
	zassert_true(holds(uncached, 0x20000000U), "cached write reached memory early");

	zassert_ok(sys_cache_data_flush_range((void *)cached, WORDS * sizeof(uint32_t)));
	zassert_true(holds(uncached, 0x30000000U), "flush did not write back");
}

ZTEST(tricore_cache, test_invd_range)
{
	fill(uncached, 0x40000000U);
	zassert_ok(sys_cache_data_invd_range((void *)cached, WORDS * sizeof(uint32_t)));
	zassert_true(holds(cached, 0x40000000U));

	/* Memory changes behind the cache: the cached view is stale until invalidated */
	fill(uncached, 0x50000000U);
	zassert_true(holds(cached, 0x40000000U), "line was not cached");

	zassert_ok(sys_cache_data_invd_range((void *)cached, WORDS * sizeof(uint32_t)));
	zassert_true(holds(cached, 0x50000000U), "invalidate kept the stale line");
}

ZTEST(tricore_cache, test_flush_all)
{
	fill(uncached, 0x60000000U);
	zassert_ok(sys_cache_data_invd_all());
	zassert_true(holds(cached, 0x60000000U));

	fill(cached, 0x70000000U);
	zassert_true(holds(uncached, 0x60000000U), "cached write reached memory early");

	zassert_ok(sys_cache_data_flush_and_invd_all());
	zassert_true(holds(uncached, 0x70000000U), "flush all did not write back");

	fill(uncached, 0x71000000U);
	zassert_true(holds(cached, 0x71000000U), "invalidate all kept a line");
}

ZTEST(tricore_cache, test_disable_writes_back)
{
	fill(uncached, 0x80000000U);
	zassert_ok(sys_cache_data_invd_all());
	zassert_true(holds(cached, 0x80000000U));
	fill(cached, 0x90000000U);

	sys_cache_data_disable();
	zassert_true(holds(uncached, 0x90000000U), "disable dropped dirty lines");
	sys_cache_data_enable();
}

ZTEST(tricore_cache, test_icache)
{
	sys_cache_instr_enable();
	zassert_ok(sys_cache_instr_invd_all());
	zassert_equal(sys_cache_instr_flush_all(), -ENOTSUP);
	sys_cache_instr_disable();
}

ZTEST_SUITE(tricore_cache, NULL, cache_setup, NULL, NULL, cache_teardown);
