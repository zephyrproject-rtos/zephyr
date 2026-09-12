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

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#include "dma_test.h"

#define XFERS 4

/* Whole words, the transfer moves 4 bytes at a time. */
#define XFER_SIZE ROUND_DOWN(DMA_TEST_TRANSFER_SIZE, 4)

static DMA_TEST_BUF uint8_t tx_data[XFER_SIZE];
static DMA_TEST_BUF uint8_t rx_data[XFERS][XFER_SIZE];

K_SEM_DEFINE(xfer_sem, 0, 1);

static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfgs[XFERS];

static void dma_sg_callback(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	if (status < 0) {
		TC_PRINT("callback status %d\n", status);
	} else {
		TC_PRINT("giving xfer_sem\n");
		k_sem_give(&xfer_sem);
	}
}

static int test_sg(const struct dma_test_dev *td)
{
	const struct device *dma = td->dev;
	struct dma_test_channels chans;
	uint32_t chan_id;

	TC_PRINT("DMA memory to memory transfer started\n");
	TC_PRINT("Preparing DMA Controller\n");

	memset(tx_data, 0, sizeof(tx_data));

	for (int i = 0; i < XFER_SIZE; i++) {
		tx_data[i] = i;
	}

	memset(rx_data, 0, sizeof(rx_data));

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

	zassert_ok(dma_test_channels_get(td, 1, &chans));
	chan_id = chans.id[0];

	memset(dma_block_cfgs, 0, sizeof(dma_block_cfgs));
	for (int i = 0; i < XFERS; i++) {
		dma_block_cfgs[i].source_gather_en = 1U;
		dma_block_cfgs[i].block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
		dma_block_cfgs[i].source_address = (uint64_t)(tx_data);
		dma_block_cfgs[i].dest_address = (uint64_t)(rx_data[i]);
		TC_PRINT("dma block %d block_size %u, source addr 0x%" PRIx64 ", dest addr 0x%"
		     PRIx64 "\n", i, (unsigned int)XFER_SIZE, dma_block_cfgs[i].source_address,
			 dma_block_cfgs[i].dest_address);
#else
		dma_block_cfgs[i].source_address = (uint32_t)(tx_data);
		dma_block_cfgs[i].dest_address = (uint32_t)(rx_data[i]);
		TC_PRINT("dma block %d block_size %u, source addr 0x%x, dest addr 0x%x\n",
			 i, (unsigned int)XFER_SIZE, dma_block_cfgs[i].source_address,
			 dma_block_cfgs[i].dest_address);
#endif
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
		TC_PRINT("Timed out waiting for xfers\n");
		return TC_FAIL;
	}

	TC_PRINT("Verify RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < XFERS; i++) {
		TC_PRINT("rx_data[%d]\n", i);
		if (memcmp(tx_data, rx_data[i], XFER_SIZE)) {
			return TC_FAIL;
		}
	}

	dma_test_channels_put(td, &chans);

	TC_PRINT("Finished: DMA Scatter-Gather\n");
	return TC_PASS;
}

/* One test case per DMA controller the board says does scatter-gather. */
#define DEFINE_DMA_M2M_SG_TESTS(idx, _)                                                            \
	COND_CODE_1(DMA_TEST_INST_HAS(idx, scatter_gather), (DMA_M2M_SG_TEST(idx)), ())

#define DMA_M2M_SG_TEST(idx)                                                                       \
	ZTEST(dma_m2m_sg, test_dma##idx##_m2m_sg)                                                  \
	{                                                                                          \
		zassert_true(test_sg(&dma_test_devs[idx]) == TC_PASS,                              \
			     "%s failed scatter gather transfer", dma_test_devs[idx].dev->name);   \
	}

LISTIFY(DMA_TEST_DEV_COUNT, DEFINE_DMA_M2M_SG_TESTS, ())

ZTEST_SUITE(dma_m2m_sg, NULL, NULL, NULL, NULL, NULL);
