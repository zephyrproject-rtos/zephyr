/*
 * Copyright (c) 2025 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(dma_test, LOG_LEVEL_DBG);

/* Buffer size for DMA transfer */
#define BUFFER_SIZE 128

#ifdef CONFIG_DMA_TEST_SG_MODE
#define NUM_SG_BLOCKS     CONFIG_DMA_TEST_SG_BLOCK_COUNT
#define TOTAL_BUFFER_SIZE (BUFFER_SIZE * NUM_SG_BLOCKS)

static struct dma_block_config sg_blocks[CONFIG_DMA_TRANSFER_CHANNEL][NUM_SG_BLOCKS];
#else
#define TOTAL_BUFFER_SIZE BUFFER_SIZE
#endif

static uint8_t src_buffer[CONFIG_DMA_TRANSFER_CHANNEL][TOTAL_BUFFER_SIZE] __aligned(64);
static uint8_t dst_buffer[CONFIG_DMA_TRANSFER_CHANNEL][TOTAL_BUFFER_SIZE] __aligned(64);

static void dma_callback(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	if (status >= 0) {
#ifdef CONFIG_DMA_TEST_SG_MODE
		LOG_INF("DMA SG transfer completed on channel %u", channel);
#else
		LOG_INF("DMA transfer completed on channel %u", channel);
#endif
	} else {
		LOG_ERR("DMA transfer failed on channel %u (status: %d)", channel, status);
	}
}

int main(void)
{
	const struct device *dma_dev[CONFIG_DMA_TRANSFER_CHANNEL];
	struct dma_config dma_cfg[CONFIG_DMA_TRANSFER_CHANNEL] = {0};
#ifndef CONFIG_DMA_TEST_SG_MODE
	struct dma_block_config dma_block_cfg[CONFIG_DMA_TRANSFER_CHANNEL] = {0};
#endif
	int ret;

	for (int i = 0; i < CONFIG_DMA_TRANSFER_CHANNEL; i++) {
#ifdef CONFIG_DMA_TEST_SG_MODE
		for (int j = 0; j < TOTAL_BUFFER_SIZE; j++) {
			src_buffer[i][j] = (i * NUM_SG_BLOCKS) + (j % 256);
		}
		LOG_INF("Channel %d: Initialized %d bytes across %d SG blocks", i,
			TOTAL_BUFFER_SIZE, NUM_SG_BLOCKS);
#else
		for (int j = 0; j < BUFFER_SIZE; j++) {
			src_buffer[i][j] = i + j;
		}
#endif
		memset(dst_buffer[i], 0, TOTAL_BUFFER_SIZE);
	}

	dma_dev[0] = DEVICE_DT_GET(DT_NODELABEL(adma0));
	dma_dev[1] = DEVICE_DT_GET(DT_NODELABEL(adma1));
	dma_dev[2] = DEVICE_DT_GET(DT_NODELABEL(adma2));
	dma_dev[3] = DEVICE_DT_GET(DT_NODELABEL(adma3));
	dma_dev[4] = DEVICE_DT_GET(DT_NODELABEL(adma4));
	dma_dev[5] = DEVICE_DT_GET(DT_NODELABEL(adma5));
	dma_dev[6] = DEVICE_DT_GET(DT_NODELABEL(adma6));
	dma_dev[7] = DEVICE_DT_GET(DT_NODELABEL(adma7));

	/* Check if DMA device is ready */
	for (int i = 0; i < CONFIG_DMA_TRANSFER_CHANNEL; i++) {
		if (!device_is_ready(dma_dev[i])) {
			LOG_ERR("DMA device is not ready");
			continue;
		}

		/* Configure DMA channel */
		dma_cfg[i].channel_direction = MEMORY_TO_MEMORY;
		dma_cfg[i].source_data_size = 1;
		dma_cfg[i].dest_data_size = 1;
		dma_cfg[i].source_burst_length = 1;
		dma_cfg[i].dest_burst_length = 1;
		dma_cfg[i].dma_callback = dma_callback;
		dma_cfg[i].user_data = NULL;

#ifdef CONFIG_DMA_TEST_SG_MODE
		/* Configure scatter-gather mode */
		for (int j = 0; j < NUM_SG_BLOCKS; j++) {
			sg_blocks[i][j].block_size = BUFFER_SIZE;
			sg_blocks[i][j].source_address =
				(uint32_t)(&src_buffer[i][j * BUFFER_SIZE]);
			sg_blocks[i][j].dest_address = (uint32_t)(&dst_buffer[i][j * BUFFER_SIZE]);
			sg_blocks[i][j].source_gather_en = 0;
			sg_blocks[i][j].dest_scatter_en = 0;

			/* Link to next block, except for the last one */
			if (j < NUM_SG_BLOCKS - 1) {
				sg_blocks[i][j].next_block = &sg_blocks[i][j + 1];
			} else {
				sg_blocks[i][j].next_block = NULL;
			}
		}

		dma_cfg[i].head_block = &sg_blocks[i][0];
		LOG_INF("Configured SG mode on channel %d", i);
#else
		/* Configure DMA block */
		dma_block_cfg[i].block_size = BUFFER_SIZE;
		dma_block_cfg[i].source_address = (uint32_t)src_buffer[i];
		dma_block_cfg[i].dest_address = (uint32_t)dst_buffer[i];
		dma_block_cfg[i].source_gather_en = 0;
		dma_block_cfg[i].dest_scatter_en = 0;
		dma_block_cfg[i].next_block = NULL;
		dma_cfg[i].head_block = &dma_block_cfg[i];

		LOG_INF("Channel %d: Configured simple mode", i);
#endif
		/* Configure the DMA channel */
		ret = dma_config(dma_dev[i], 0, &dma_cfg[i]);
		if (ret != 0) {
			LOG_ERR("Failed to configure DMA channel (error: %d)", ret);
			return -EINVAL;
		}

		/* Start the DMA transfer */
		ret = dma_start(dma_dev[i], 0);
		if (ret != 0) {
			LOG_ERR("Failed to start DMA transfer (error: %d)", ret);
			return -EINVAL;
		}
	}

	/* Verify the transfer */
	for (int i = 0; i < CONFIG_DMA_TRANSFER_CHANNEL; i++) {
#ifdef CONFIG_DMA_TEST_SG_MODE
		/* Verify all SG blocks */
		bool verify_pass = true;

		for (int j = 0; j < NUM_SG_BLOCKS; j++) {
			if (memcmp(&src_buffer[i][j * BUFFER_SIZE], &dst_buffer[i][j * BUFFER_SIZE],
				   BUFFER_SIZE) != 0) {
				LOG_ERR("Channel %d Block %d: verification failed", i, j);
				verify_pass = false;
			}
		}

		if (verify_pass) {
			LOG_INF("SG blocks verified successfully on channel %d", i);
		} else {
			LOG_ERR("SG transfer verification failed on channel %d", i);
			return -EINVAL;
		}
#else
		if (memcmp(src_buffer[i], dst_buffer[i], BUFFER_SIZE) == 0) {
			LOG_INF("DMA transfer verified successfully on channel %d", i);
		} else {
			LOG_ERR("DMA transfer verification failed on channel %d", i);
			return -EINVAL;
		}
#endif
	}

	LOG_INF("DMA transfer verified all on channels\n");
	return 0;
}
