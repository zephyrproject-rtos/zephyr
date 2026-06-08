/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define DMA_NODE DT_NODELABEL(dma)
#define DMA_CHANNEL 0U
#define TEST_LEN 512U
#define CACHE_ALIGN 32U

static uint8_t src[TEST_LEN] __aligned(CACHE_ALIGN);
static uint8_t dst[TEST_LEN] __aligned(CACHE_ALIGN);

static K_SEM_DEFINE(done, 0, 1);
static int callback_status;

static void dma_done(const struct device *dev, void *user_data, uint32_t channel,
		     int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	callback_status = status;
	k_sem_give(&done);
}

ZTEST(bcm2835_dma_memcpy, test_memcpy_channel0)
{
	const struct device *dma = DEVICE_DT_GET(DMA_NODE);
	struct dma_block_config block = { 0 };
	struct dma_config config = { 0 };

	zassert_true(device_is_ready(dma), "DMA device is not ready");

	for (size_t i = 0U; i < TEST_LEN; i++) {
		src[i] = (uint8_t)((i * 7U) ^ 0x5aU);
		dst[i] = 0U;
	}

	(void)sys_cache_data_flush_range(src, sizeof(src));
	(void)sys_cache_data_flush_range(dst, sizeof(dst));

	block.source_address = (uint32_t)src;
	block.dest_address = (uint32_t)dst;
	block.block_size = TEST_LEN;
	block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	config.channel_direction = MEMORY_TO_MEMORY;
	config.source_data_size = 1U;
	config.dest_data_size = 1U;
	config.source_burst_length = 1U;
	config.dest_burst_length = 1U;
	config.block_count = 1U;
	config.head_block = &block;
	config.dma_callback = dma_done;
	config.error_callback_dis = 0U;

	callback_status = -EINPROGRESS;
	k_sem_reset(&done);

	zassert_ok(dma_config(dma, DMA_CHANNEL, &config));
	zassert_ok(dma_start(dma, DMA_CHANNEL));
	zassert_ok(k_sem_take(&done, K_SECONDS(2)), "DMA completion timed out");

	(void)sys_cache_data_invd_range(dst, sizeof(dst));

	zassert_equal(callback_status, DMA_STATUS_COMPLETE);
	zassert_mem_equal(dst, src, TEST_LEN);
}

ZTEST_SUITE(bcm2835_dma_memcpy, NULL, NULL, NULL, NULL, NULL);
