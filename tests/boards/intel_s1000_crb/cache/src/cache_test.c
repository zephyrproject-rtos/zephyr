/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(cache_test);

#define LP_SRAM_BASE		0xBE800000
#define LP_SRAM_BASE_UNCACHED	0x9E800000

#define CACHE_TEST_BUFFER_SIZE	256

struct test_buffer {
	u8_t	flush[CACHE_TEST_BUFFER_SIZE];
	u8_t	invalidate[CACHE_TEST_BUFFER_SIZE];
};

static struct test_buffer *cached_buffer = (struct test_buffer *)LP_SRAM_BASE;

static struct test_buffer *mem_buffer =
	(struct test_buffer *)LP_SRAM_BASE_UNCACHED;

static void buffer_fill_sequence(u8_t *buffer, bool inv_seq)
{
	int byte;

	for (byte = 0; byte < CACHE_TEST_BUFFER_SIZE; byte++) {
		buffer[byte] = inv_seq ? ~byte : byte;
	}
}

static void cache_flush_test(void)
{
	LOG_INF("Filling main memory with an inverted byte sequence ...");
	buffer_fill_sequence(mem_buffer->flush, true);

	LOG_INF("Filling cacheable memory with a normal byte sequence ...");
	buffer_fill_sequence(cached_buffer->flush, false);

	LOG_INF("Comparing contents of cached memory vs main memory ...");
	if (memcmp(mem_buffer->flush, cached_buffer->flush,
				CACHE_TEST_BUFFER_SIZE)) {
		LOG_INF("Contents mismatch. This is expected");
	} else {
		LOG_ERR("Contents match. Is Cache configured write-through?");
	}

	LOG_INF("Flushing cache to commit contents to main memory ...");
	xthal_dcache_region_writeback(cached_buffer->flush,
			CACHE_TEST_BUFFER_SIZE);

	LOG_INF("Comparing contents of cached memory vs main memory ...");
	if (memcmp(mem_buffer->flush, cached_buffer->flush,
				CACHE_TEST_BUFFER_SIZE)) {
		LOG_ERR("Contents mismatch. Cache flush test Failed");
	} else {
		LOG_INF("Contents match. Cache flush test Passed");
	}
}

static void cache_invalidation_test(void)
{
	LOG_INF("Filling main memory with an inverted byte sequence ...");
	buffer_fill_sequence(mem_buffer->invalidate, true);

	LOG_INF("Filling cacheable memory with a normal byte sequence ...");
	buffer_fill_sequence(cached_buffer->invalidate, false);

	LOG_INF("Comparing contents of cached memory vs main memory ...");
	if (memcmp(mem_buffer->invalidate, cached_buffer->invalidate,
				CACHE_TEST_BUFFER_SIZE)) {
		LOG_INF("Contents mismatch. This is expected");
	} else {
		LOG_ERR("Contents match. This is unexpected");
	}

	LOG_INF("Invalidating cache to read contents from main memory ...");
	xthal_dcache_region_invalidate(cached_buffer->invalidate,
			CACHE_TEST_BUFFER_SIZE);

	LOG_INF("Comparing contents of cached memory vs main memory ...");
	if (memcmp(mem_buffer->invalidate, cached_buffer->invalidate,
				CACHE_TEST_BUFFER_SIZE)) {
		LOG_ERR("Contents mismatch. Cache invalidation test Failed");
	} else {
		LOG_INF("Contents match. Cache invalidation test Passed");
	}
}

void main(void)
{
	LOG_INF("Data Cache write-back test for Intel S1000");
	cache_flush_test();
	LOG_INF("Data Cache invalidation test for Intel S1000");
	cache_invalidation_test();
}
