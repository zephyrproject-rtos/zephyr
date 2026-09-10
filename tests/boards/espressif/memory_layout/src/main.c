/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

extern char _image_ram_start[];
extern char _end[];
extern char _heap_sentry[];

ZTEST(memory_layout, test_sram_region_size)
{
	size_t total_sram = (size_t)_heap_sentry - (size_t)_image_ram_start;
	size_t heap_size = (size_t)_heap_sentry - (size_t)_end;

	TC_PRINT("_image_ram_start=%p, _end=%p, _heap_sentry=%p\n",
		 (void *)_image_ram_start, (void *)_end,
		 (void *)_heap_sentry);
	TC_PRINT("total SRAM region: %zu bytes (%zu KB)\n",
		 total_sram, total_sram / 1024);
	TC_PRINT("libc heap region: %zu bytes (%zu KB)\n",
		 heap_size, heap_size / 1024);

	zassert_true(total_sram > 0, "SRAM region is empty");
	zassert_true(heap_size > 1024, "heap too small: %zu bytes", heap_size);
}

ZTEST(memory_layout, test_heap_boundary)
{
	volatile uint8_t *start = (volatile uint8_t *)_end;
	volatile uint8_t *end = (volatile uint8_t *)_heap_sentry;
	size_t region_size = end - start;
	size_t errors = 0;

	TC_PRINT("testing %zu bytes (%zu KB) from %p to %p\n",
		 region_size, region_size / 1024,
		 (void *)start, (void *)end);

	for (volatile uint8_t *p = start; p < end; p += 4) {
		*p = (uint8_t)(((uintptr_t)p >> 2) ^ 0xA5);
	}

	for (volatile uint8_t *p = start; p < end; p += 4) {
		uint8_t expected = (uint8_t)(((uintptr_t)p >> 2) ^ 0xA5);

		if (*p != expected) {
			if (errors == 0) {
				TC_PRINT("FIRST mismatch at %p: "
					 "got 0x%02x, expected 0x%02x\n",
					 (void *)p, *p, expected);
			}
			errors++;
		}
	}

	zassert_equal(errors, 0,
		      "pattern check: %zu errors in %zu bytes",
		      errors, region_size);

	TC_PRINT("libc heap boundary: %zu KB verified up to %p\n",
		 region_size / 1024, (void *)end);
}

ZTEST_SUITE(memory_layout, NULL, NULL, NULL, NULL, NULL);
