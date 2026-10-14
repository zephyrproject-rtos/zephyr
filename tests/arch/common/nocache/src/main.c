/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

#define LOAD_MAGIC 0x600dcafeU

/* Sizes are picked so that the region needs some alignment padding */
static volatile uint8_t nocache_buf[200] __nocache;

static volatile struct {
	uint32_t magic;
	uint8_t bytes[13];
} nocache_load_data __nocache_load = {
	.magic = LOAD_MAGIC,
	.bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
};

static bool in_range(uintptr_t addr, size_t size, uintptr_t start, uintptr_t end)
{
	return (addr >= start) && (addr + size <= end);
}

ZTEST(arch_nocache, test_placement)
{
	uintptr_t start = (uintptr_t)_nocache_ram_start;
	uintptr_t end = (uintptr_t)_nocache_ram_end;

	zassert_true(in_range((uintptr_t)nocache_buf, sizeof(nocache_buf),
			      (uintptr_t)_nocache_noload_ram_start,
			      (uintptr_t)_nocache_noload_ram_end),
		     "__nocache data outside of the non-loadable part");
	zassert_true(in_range((uintptr_t)&nocache_load_data, sizeof(nocache_load_data),
			      (uintptr_t)_nocache_load_ram_start, (uintptr_t)_nocache_load_ram_end),
		     "__nocache_load data outside of the loadable part");
	zassert_true(in_range((uintptr_t)_nocache_noload_ram_start,
			      (uintptr_t)_nocache_noload_ram_size, start, end),
		     "non-loadable part outside of the nocache region");
	zassert_true(in_range((uintptr_t)_nocache_load_ram_start, (uintptr_t)_nocache_load_ram_size,
			      start, end),
		     "loadable part outside of the nocache region");
}

ZTEST(arch_nocache, test_region_alignment)
{
	uintptr_t start = (uintptr_t)_nocache_ram_start;
	size_t size = (size_t)_nocache_ram_size;

	zassert_equal(size, (uintptr_t)_nocache_ram_end - start);

#if defined(CONFIG_MMU)
	zassert_true(IS_ALIGNED(start, CONFIG_MMU_PAGE_SIZE), "start 0x%lx not page aligned",
		     start);
	zassert_true(IS_ALIGNED(size, CONFIG_MMU_PAGE_SIZE), "size 0x%zx not page aligned", size);
#elif defined(CONFIG_ARM_MPU) && !defined(CONFIG_CUSTOM_SECTION_ALIGN)
	zassert_true(IS_ALIGNED(start, CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE),
		     "start 0x%lx not aligned to the MPU granularity", start);
	zassert_true(IS_ALIGNED(size, CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE),
		     "size 0x%zx not aligned to the MPU granularity", size);
#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
	/* The whole region is covered by a single MPU region */
	zassert_true(IS_POWER_OF_TWO(size), "size 0x%zx is not a power of two", size);
	zassert_true(IS_ALIGNED(start, size), "start 0x%lx not aligned to size 0x%zx", start, size);
#endif
#endif
}

ZTEST(arch_nocache, test_noload_zeroed)
{
	for (size_t i = 0; i < sizeof(nocache_buf); i++) {
		zassert_equal(nocache_buf[i], 0, "__nocache byte %zu not zeroed", i);
	}
}

ZTEST(arch_nocache, test_load_initialized)
{
	zassert_equal(nocache_load_data.magic, LOAD_MAGIC);
	for (size_t i = 0; i < ARRAY_SIZE(nocache_load_data.bytes); i++) {
		zassert_equal(nocache_load_data.bytes[i], i + 1,
			      "__nocache_load byte %zu not initialized", i);
	}
}

ZTEST(arch_nocache, test_load_not_padded)
{
	/* nocache_load_data is the only __nocache_load object of this image.
	 * The loadable part must hold just that object: the alignment padding
	 * of the region belongs to the non-loadable part, otherwise it ends up
	 * in the ROM image and is copied to RAM at boot.
	 */
	zassert_equal((uintptr_t)_nocache_load_ram_start, (uintptr_t)&nocache_load_data);
	zassert_equal((size_t)_nocache_load_ram_size, sizeof(nocache_load_data),
		      "loadable part is 0x%zx bytes for 0x%zx bytes of data",
		      (size_t)_nocache_load_ram_size, sizeof(nocache_load_data));
}

ZTEST_SUITE(arch_nocache, NULL, NULL, NULL, NULL, NULL);
