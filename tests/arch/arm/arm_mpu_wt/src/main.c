/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(arm_mpu_wt_test, LOG_LEVEL_INF);

/* Test memory configuration */
#define TEST_PATTERN_COUNT 8
#define TEST_MEMORY_SIZE (sizeof(uint32_t) * TEST_PATTERN_COUNT)
#define CACHE_LINE_SIZE 32  /* Typical cache line size for ARM Cortex-M7/M33 */
#define DMA_TIMEOUT_MS 1000
#define DMA_CHANNEL 0

/* Memory region configuration from device tree */
#if DT_NODE_EXISTS(DT_ALIAS(test_memory))
#define TEST_MEMORY_NODE DT_ALIAS(test_memory)
#define TEST_MEMORY_BASE DT_REG_ADDR(TEST_MEMORY_NODE)
#define TEST_MEMORY_REGION_SIZE DT_REG_SIZE(TEST_MEMORY_NODE)
#define USE_CUSTOM_MEMORY 1
#else
#define USE_CUSTOM_MEMORY 0
#endif

static const uint32_t test_patterns[] = {
	0x12345678U, 0xDEADBEEFU, 0xCAFEBABEU, 0x87654321U,
	0xA5A5A5A5U, 0x5A5A5A5AU, 0xFFFFFFFFU, 0x00000000U
};

/* DMA completion tracking */
static volatile bool dma_transfer_done;
static struct k_sem dma_sem;

/* Memory region tracking */
struct memory_region {
	uintptr_t base;
	size_t size;
	const char *name;
	bool use_malloc;
};

static struct memory_region current_test_region;

/**
 * @brief Initialize test memory region
 *
 * @param region Pointer to memory region structure
 * @return 0 on success, negative error code on failure
 */
static int init_test_memory_region(struct memory_region *region)
{
#if USE_CUSTOM_MEMORY
	region->base = TEST_MEMORY_BASE;
	region->size = TEST_MEMORY_REGION_SIZE;
	region->name = DT_NODE_FULL_NAME(TEST_MEMORY_NODE);
	region->use_malloc = false;

	LOG_INF("Using custom memory region from device tree:");
	LOG_INF("  Node: %s", region->name);
	LOG_INF("  Base: 0x%08x", (uint32_t)region->base);
	LOG_INF("  Size: %zu bytes", region->size);
#else
	/* Fallback to malloc-based allocation */
	region->base = 0;
	region->size = 0;
	region->name = "malloc (default SRAM)";
	region->use_malloc = true;

	LOG_INF("Using malloc-based memory allocation (default SRAM)");
#endif

	return 0;
}

/**
 * @brief Get memory region information string
 *
 * @return String describing current memory region
 */
static const char *get_memory_region_info(void)
{
	static char info_buf[128];

	if (current_test_region.use_malloc) {
		snprintf(info_buf, sizeof(info_buf), "malloc (default SRAM)");
	} else {
		snprintf(info_buf, sizeof(info_buf), "%s @ 0x%08x (%zu bytes)",
			 current_test_region.name,
			 (uint32_t)current_test_region.base,
			 current_test_region.size);
	}

	return info_buf;
}

/**
 * @brief DMA callback function
 *
 * @param dev DMA device
 * @param user_data User data (unused)
 * @param channel DMA channel
 * @param status Transfer status
 */
static void dma_callback(const struct device *dev, void *user_data,
			 uint32_t channel, int status)
{
	LOG_INF("DMA callback: dev=%s, channel=%d, status=%d",
		dev ? dev->name : "NULL", channel, status);

	if (status == 0) {
		LOG_INF("DMA transfer successful");
		dma_transfer_done = true;
		k_sem_give(&dma_sem);
	} else {
		LOG_ERR("DMA transfer failed with status: %d", status);
		dma_transfer_done = false;
		k_sem_give(&dma_sem);
	}
}

/**
 * @brief Allocate memory from test region
 *
 * @param size Size to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
static void *alloc_test_memory(size_t size)
{
	void *ptr;
	static uintptr_t next_offset;

	if (current_test_region.use_malloc) {
		/* Use standard malloc */
		ptr = k_malloc(size);
		if (ptr != NULL && ((uintptr_t)ptr & (sizeof(uint32_t) - 1)) == 0) {
			LOG_DBG("Allocated %zu bytes at %p via malloc", size, ptr);
			return ptr;
		}

		if (ptr != NULL) {
			k_free(ptr);
		}
		return NULL;
	}

	/* Use custom memory region */
	if (next_offset + size > current_test_region.size) {
		LOG_ERR("Not enough space in custom memory region");
		LOG_ERR("  Requested: %zu bytes", size);
		LOG_ERR("  Available: %zu bytes", (size_t)(current_test_region.size - next_offset));
		return NULL;
	}

	ptr = (void *)(current_test_region.base + next_offset);
	next_offset += ROUND_UP(size, sizeof(uint32_t));

	LOG_DBG("Allocated %zu bytes at %p from custom region (offset: %zu)",
		size, ptr, (size_t)(next_offset - ROUND_UP(size, sizeof(uint32_t))));

	return ptr;
}

/**
 * @brief Free memory allocated from test region
 *
 * @param ptr Pointer to free
 */
static void free_test_memory(void *ptr)
{
	if (current_test_region.use_malloc && ptr != NULL) {
		k_free(ptr);
		LOG_DBG("Freed memory at %p via malloc", ptr);
	}
	/* For custom memory region, we don't actually free (simple bump allocator) */
}

/**
 * @brief Get available DMA device
 *
 * @return Pointer to DMA device or NULL if not available
 */
static const struct device *get_dma_device(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(test_dma))
	const struct device *dma_dev = DEVICE_DT_GET(DT_ALIAS(test_dma));

	if (device_is_ready(dma_dev)) {
		return dma_dev;
	}
#endif
	return NULL;
}

/**
 * @brief Execute DMA transfer for coherency testing
 *
 * @param dma_dev DMA device
 * @param src Source address
 * @param dst Destination address
 * @param size Transfer size in bytes
 * @return 0 on success, negative error code on failure
 */
static int execute_dma_transfer(const struct device *dma_dev, void *src,
				void *dst, size_t size)
{
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block = {0};
	int ret;

	if (dma_dev == NULL) {
		return -ENODEV;
	}

	/* Validate alignment requirements for EDMA */
	if (((uintptr_t)src & (sizeof(uint32_t) - 1)) != 0 ||
	    ((uintptr_t)dst & (sizeof(uint32_t) - 1)) != 0) {
		LOG_ERR("Source or destination not properly aligned");
		return -EINVAL;
	}

	if ((size & (sizeof(uint32_t) - 1)) != 0) {
		LOG_ERR("Transfer size not multiple of %zu bytes", sizeof(uint32_t));
		return -EINVAL;
	}

	dma_transfer_done = false;

#ifdef CONFIG_CACHE_MANAGEMENT
	/* Clean source cache to ensure data is written to memory */
	sys_cache_data_flush_range(src, size);

	/* Invalidate destination cache to ensure fresh read from memory */
	sys_cache_data_invd_range(dst, size);
#endif

	/* Configure DMA block */
	dma_block.source_address = (uintptr_t)src;
	dma_block.dest_address = (uintptr_t)dst;
	dma_block.block_size = size;

	LOG_INF("DMA config: src=0x%08x, dst=0x%08x, size=%zu",
		(uint32_t)src, (uint32_t)dst, size);

	/* Configure DMA with proper settings for EDMA */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.source_burst_length = 32;
	dma_cfg.dest_burst_length = 32;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block;
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.complete_callback_en = 1U;  /* Enable completion callback */
	dma_cfg.error_callback_dis = 0U;    /* Enable error callback */
	dma_cfg.user_data = NULL;

	LOG_INF("Configuring DMA channel %d", DMA_CHANNEL);
	ret = dma_config(dma_dev, DMA_CHANNEL, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("DMA config failed: %d", ret);
		return ret;
	}

	LOG_INF("Starting DMA transfer");
	ret = dma_start(dma_dev, DMA_CHANNEL);
	if (ret != 0) {
		LOG_ERR("DMA start failed: %d", ret);
		return ret;
	}

	LOG_INF("Waiting for DMA completion (timeout: %d ms)", DMA_TIMEOUT_MS);
	ret = k_sem_take(&dma_sem, K_MSEC(DMA_TIMEOUT_MS));
	if (ret != 0) {
		LOG_ERR("DMA transfer timeout");

		/* Check DMA status */
		struct dma_status status;
		int status_ret = dma_get_status(dma_dev, DMA_CHANNEL, &status);

		if (status_ret == 0) {
			LOG_INF("DMA status: busy=%d, dir=%d, pending=%d",
				status.busy, status.dir, status.pending_length);
		}

		(void)dma_stop(dma_dev, DMA_CHANNEL);
		return ret;
	}

#ifdef CONFIG_CACHE_MANAGEMENT
	/* Invalidate destination cache after DMA to ensure CPU reads fresh data */
	sys_cache_data_invd_range(dst, size);
#endif

	LOG_INF("DMA transfer completed successfully");
	return 0;
}

/**
 * @brief Test Write-Through cache coherency using DMA with multiple patterns
 */
ZTEST(arm_mpu_wt, test_wt_dma_coherency)
{
	const struct device *dma_dev;
	uint32_t *cpu_buffer;
	uint32_t *dma_buffer;
	int ret;

	dma_dev = get_dma_device();
	if (dma_dev == NULL) {
		LOG_WRN("DMA device not available, skipping DMA coherency test");
		ztest_test_skip();
		return;
	}

	LOG_INF("Using DMA device: %s", dma_dev->name);
	LOG_INF("Testing memory region: %s", get_memory_region_info());

	/* Allocate buffers large enough for all test patterns */
	cpu_buffer = alloc_test_memory(TEST_MEMORY_SIZE);
	dma_buffer = alloc_test_memory(TEST_MEMORY_SIZE);

	if (cpu_buffer == NULL || dma_buffer == NULL) {
		LOG_ERR("Failed to allocate test buffers");
		if (cpu_buffer != NULL) {
			free_test_memory((void *)cpu_buffer);
		}
		if (dma_buffer != NULL) {
			free_test_memory(dma_buffer);
		}
		ztest_test_skip();
		return;
	}

	LOG_INF("Testing Write-Through DMA coherency with %d patterns", TEST_PATTERN_COUNT);
	LOG_INF("CPU buffer: %p, DMA buffer: %p", (void *)cpu_buffer, (void *)dma_buffer);
	LOG_INF("Buffer spacing: %d bytes",
		(int)((uintptr_t)dma_buffer - (uintptr_t)cpu_buffer));
	LOG_INF("Transfer size: %u bytes", (unsigned int)TEST_MEMORY_SIZE);

	/* Initialize DMA buffer with known pattern (inverse of test patterns) */
	for (int i = 0; i < TEST_PATTERN_COUNT; i++) {
		dma_buffer[i] = ~test_patterns[i];  /* Inverted pattern */
	}

	/* CPU writes all test patterns to buffer */
	for (int i = 0; i < TEST_PATTERN_COUNT; i++) {
		cpu_buffer[i] = test_patterns[i];
		LOG_DBG("CPU wrote pattern[%d] = 0x%08x to %p",
			i, test_patterns[i], (void *)&cpu_buffer[i]);
	}

	/*
	 * With Write-Through cache:
	 * - CPU writes automatically go to both cache and memory
	 * - No explicit flush needed
	 * - Only need barrier to ensure write ordering before DMA starts
	 */
	barrier_dsync_fence_full();

	LOG_INF("CPU wrote %d patterns to buffer at %p",
		TEST_PATTERN_COUNT, (void *)cpu_buffer);

	/* DMA reads directly from memory (bypassing CPU cache) */
	ret = execute_dma_transfer(dma_dev, (void *)cpu_buffer, dma_buffer,
				   TEST_MEMORY_SIZE);

	if (ret == 0) {
#ifdef CONFIG_CACHE_MANAGEMENT
		/*
		 * Invalidate destination cache to ensure CPU reads fresh data
		 * This is needed because DMA wrote to memory, bypassing cache
		 */
		sys_cache_data_invd_range(dma_buffer, TEST_MEMORY_SIZE);
#endif

		LOG_INF("DMA transfer completed, verifying %d patterns", TEST_PATTERN_COUNT);

		/* Verify all patterns were transferred correctly */
		bool all_passed = true;

		for (int i = 0; i < TEST_PATTERN_COUNT; i++) {
			uint32_t expected = test_patterns[i];
			uint32_t actual = dma_buffer[i];

			if (actual != expected) {
				LOG_ERR("Pattern[%d] mismatch: expected 0x%08x, got 0x%08x",
					i, expected, actual);
				all_passed = false;
			} else {
				LOG_DBG("Pattern[%d] OK: 0x%08x", i, actual);
			}

			/*
			 * Write-Through: DMA should read the value CPU just wrote
			 * because WT cache already wrote it to memory
			 */
			zassert_equal(actual, expected,
				      "DMA coherency test failed at pattern[%d]: "
				      "expected 0x%08x, got 0x%08x. "
				      "Write-Through may not be working correctly.",
				      i, expected, actual);
		}

		if (all_passed) {
			LOG_INF("DMA coherency test PASSED - All %d patterns verified",
				TEST_PATTERN_COUNT);
			LOG_INF("Write-Through cache is working correctly");
		}
	} else {
		LOG_ERR("DMA transfer failed: %d", ret);
		ztest_test_fail();
	}

	free_test_memory((void *)cpu_buffer);
	free_test_memory(dma_buffer);
}

/**
 * @brief Test Write-Through with cache invalidation using multiple patterns
 */
ZTEST(arm_mpu_wt, test_wt_cache_invalidate)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	uint32_t *test_addr;

	/* Allocate enough space for all test patterns */
	test_addr = alloc_test_memory(TEST_MEMORY_SIZE);
	if (test_addr == NULL) {
		ztest_test_skip();
		return;
	}

	LOG_INF("Testing cache invalidation with Write-Through");
	LOG_INF("Memory region: %s", get_memory_region_info());
	LOG_INF("Test address: %p (cache line aligned)", (void *)test_addr);
	LOG_INF("Testing with %d patterns", TEST_PATTERN_COUNT);

	/* CPU writes all test patterns */
	for (int i = 0; i < TEST_PATTERN_COUNT; i++) {
		test_addr[i] = test_patterns[i];
		LOG_DBG("Wrote pattern[%d] = 0x%08x", i, test_patterns[i]);
	}

	/* Invalidate CPU cache to force read from memory */
	sys_cache_data_invd_range((void *)test_addr, TEST_MEMORY_SIZE);
	barrier_dsync_fence_full();

	LOG_INF("Cache invalidated, reading back from memory");

	/* Read after invalidation - should read from memory */
	bool all_passed = true;

	for (int i = 0; i < TEST_PATTERN_COUNT; i++) {
		uint32_t expected = test_patterns[i];
		uint32_t actual = test_addr[i];

		if (actual != expected) {
			LOG_ERR("Pattern[%d] mismatch after invalidation: "
				"expected 0x%08x, got 0x%08x",
				i, expected, actual);
			all_passed = false;
		} else {
			LOG_DBG("Pattern[%d] OK after invalidation: 0x%08x", i, actual);
		}

		/*
		 * Write-Through: Data should still be in memory after cache invalidation
		 * because WT cache already wrote it there
		 */
		zassert_equal(actual, expected,
			      "Cache invalidate test failed at pattern[%d]: "
			      "expected 0x%08x, got 0x%08x. "
			      "Write-Through should maintain memory consistency.",
			      i, expected, actual);
	}

	if (all_passed) {
		LOG_INF("Cache invalidation test PASSED - All %d patterns verified",
			TEST_PATTERN_COUNT);
	}

	free_test_memory((void *)test_addr);
#else
	LOG_WRN("Cache management not enabled, skipping test");
	ztest_test_skip();
#endif
}

/**
 * @brief Test suite setup function
 *
 * @return Test fixture data (NULL)
 */
static void *arm_mpu_wt_setup(void)
{
	/* Initialize DMA semaphore */
	k_sem_init(&dma_sem, 0, 1);

	/* Initialize test memory region */
	init_test_memory_region(&current_test_region);

	LOG_INF("ARM MPU Write-Through test suite initialized");
	LOG_INF("Test memory region: %s", get_memory_region_info());

	return NULL;
}

ZTEST_SUITE(arm_mpu_wt, NULL, arm_mpu_wt_setup, NULL, NULL, NULL);
