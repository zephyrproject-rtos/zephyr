/*
 * Copyright (c) 2025 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory cyclic transfer
 * @details
 * - Test Steps
 *   -# Set dma configuration for cyclic configuration
 *   -# Start transfer tx -> rx
 *   -# Wait for a block transfer to complete
 *   -# Suspend transfer and check tx/rx contents match
 *   -# Invalidate rx data and resume transfer
 *   -# Wait for a block transfer to complete
 *   -# Stop transfer and check tx/rx contents match
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

static __aligned(32) uint8_t tx_data[CONFIG_DMA_CYCLIC_XFER_SIZE];
static __aligned(32) uint8_t rx_data[CONFIG_DMA_CYCLIC_XFER_SIZE];

K_SEM_DEFINE(xfer_sem, 0, 1);

static struct dma_config dma_cfg;
static struct dma_block_config dma_block_cfgs;

static void dma_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	k_sem_give(&xfer_sem);
}


static int test_cyclic(void)
{
	const struct device *dma;
	int chan_id;

	TC_PRINT("Preparing DMA Controller\n");

	for (int i = 0; i < CONFIG_DMA_CYCLIC_XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	dma = DEVICE_DT_GET(DT_ALIAS(dma0));
	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 4U;
	dma_cfg.dest_data_size = 4U;
	dma_cfg.source_burst_length = 4U;
	dma_cfg.dest_burst_length = 4U;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfgs;
	dma_cfg.complete_callback_en = true; /* per block completion */
	dma_cfg.cyclic = 1;

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("Platform does not support dma request channel,"
			 " using Kconfig DMA_SG_CHANNEL_NR\n");
		chan_id = CONFIG_DMA_CYCLIC_CHANNEL_NR;
	}

	dma_block_cfgs.block_size = CONFIG_DMA_CYCLIC_XFER_SIZE;

	dma_block_cfgs.source_address = (uint32_t)(tx_data);
	dma_block_cfgs.dest_address = (uint32_t)(rx_data);

	TC_PRINT("Configuring cyclic transfer on channel %d\n", chan_id);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting cyclic transfer on channel %d and waiting for first block to complete\n",
		 chan_id);

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (k_sem_take(&xfer_sem, K_MSEC(10)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	if (dma_suspend(dma, chan_id) != 0) {
		TC_PRINT("Failed to suspend transfer\n");
		return TC_FAIL;
	}

	if (memcmp(tx_data, rx_data, CONFIG_DMA_CYCLIC_XFER_SIZE)) {
		TC_PRINT("Failed to verify tx/rx in the first cycle.\n");
		return TC_FAIL;
	}

	k_sem_reset(&xfer_sem);

	/* reset rx_data to validate that transfer cycles */
	memset(rx_data, 0, sizeof(rx_data));

	if (dma_resume(dma, chan_id) != 0) {
		TC_PRINT("Failed to resume transfer\n");
		return TC_FAIL;
	}

	if (k_sem_take(&xfer_sem, K_MSEC(10)) != 0) {
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	if (dma_stop(dma, chan_id) != 0) {
		TC_PRINT("Failed to stop dma\n");
		return TC_FAIL;
	}

	if (memcmp(tx_data, rx_data, CONFIG_DMA_CYCLIC_XFER_SIZE)) {
		TC_PRINT("Failed to verify tx/rx in the second cycle.\n");
		return TC_FAIL;
	}

	TC_PRINT("Finished: DMA Cyclic test\n");
	return TC_PASS;
}

/* export test cases */
ZTEST(dma_m2m_cyclic, test_dma_m2m_cyclic)
{
	zassert_true((test_cyclic() == TC_PASS));
}
