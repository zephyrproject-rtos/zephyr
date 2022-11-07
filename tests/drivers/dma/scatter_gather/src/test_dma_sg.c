/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory transfer loops with scatter gather
 * @details
 * - Test Steps
 *   -# Set dma configuration for scatter gather enable
 *   -# Set direction memory-to-memory with two block transfers
 *   -# Start transfer tx -> rx
 * - Expected Results
 *   -# Data is transferred correctly from src buffers to dest buffers without
 *      software intervention.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define XFERS 2
#define XFER_SIZE 64

#if CONFIG_NOCACHE_MEMORY
static __aligned(32) const char TX_DATA[] = "The quick brown fox jumps over the lazy dog";
static __aligned(32) char tx_data[XFER_SIZE] __used
	__attribute__((__section__(".nocache")));
static __aligned(32) char rx_data[XFERS][XFER_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
#else
/* this src memory shall be in RAM to support usingas a DMA source pointer.*/
static __aligned(32) const char tx_data[] = "The quick brown fox jumps over the lazy dog";
static __aligned(32) char rx_data[XFERS][XFER_SIZE] = { { 0 } };
#endif

K_SEM_DEFINE(xfer_sem, 0, 1);

static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfgs[XFERS];

static void dma_sg_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	if (status) {
		TC_PRINT("callback status %d\n", status);
	} else {
		TC_PRINT("giving xfer_sem\n");
		k_sem_give(&xfer_sem);
	}
}

static int test_sg(void)
{
	const struct device *dma;
	static int chan_id;

	TC_PRINT("DMA memory to memory transfer started\n");
	TC_PRINT("Preparing DMA Controller\n");

#if CONFIG_NOCACHE_MEMORY
	memset(tx_data, 0, sizeof(tx_data));
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif
	memset(rx_data, 0, sizeof(rx_data));

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
#ifdef CONFIG_DMAMUX_STM32
	dma_cfg.user_data = (struct device *)dma;
#else
	dma_cfg.user_data = NULL;
#endif /* CONFIG_DMAMUX_STM32 */
	dma_cfg.dma_callback = dma_sg_callback;
	dma_cfg.block_count = XFERS;
	dma_cfg.head_block = dma_block_cfgs;
	dma_cfg.complete_callback_en = false; /* per block completion */

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("Platform does not support dma request channel,"
			 " using Kconfig DMA_SG_CHANNEL_NR\n");
		chan_id = CONFIG_DMA_SG_CHANNEL_NR;
	}

	memset(dma_block_cfgs, 0, sizeof(dma_block_cfgs));
	for (int i = 0; i < XFERS; i++) {
		dma_block_cfgs[i].source_gather_en = 1U;
		dma_block_cfgs[i].block_size = XFER_SIZE;
		dma_block_cfgs[i].source_address = (uint32_t)(tx_data);
		dma_block_cfgs[i].dest_address = (uint32_t)(rx_data[i]);
		TC_PRINT("dma block %d block_size %d, source addr %x, dest addr %x\n",
			 i, XFER_SIZE, dma_block_cfgs[i].source_address,
			 dma_block_cfgs[i].dest_address);
		if (i < XFERS - 1) {
			dma_block_cfgs[i].next_block = &dma_block_cfgs[i+1];
			TC_PRINT("set next block pointer to %p\n", dma_block_cfgs[i].next_block);
		}
	}

	TC_PRINT("Configuring the scatter-gather transfer on channel %d\n", chan_id);

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer on channel %d and waiting completion\n", chan_id);

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < XFERS; i++) {
		TC_PRINT("rx_data[%d] %s\n", i, rx_data[i]);
		if (strncmp(tx_data, rx_data[i], sizeof(rx_data[i])) != 0) {
			return TC_FAIL;
		}
	}

	TC_PRINT("Finished: DMA Scatter-Gather\n");
	return TC_PASS;
}

/* export test cases */
ZTEST(dma_m2m_sg, test_dma_m2m_sg)
{
	zassert_true((test_sg() == TC_PASS));
}
