/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_dma_mem_to_mem
 * @{
 * @defgroup t_dma_m2m_chan_burst test_dma_m2m_chan_burst
 * @brief TestPurpose: verify zephyr dma memory to memory transfer
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transfer
 * - Expected Results
 *   -# Data is transferred correctly from src to dest
 * @}
 */

#include <zephyr.h>
#include <dma.h>
#include <ztest.h>

#define DMA_DEVICE_NAME CONFIG_DMA_0_NAME
#define RX_BUFF_SIZE (48)

static const char tx_data[] = "It is harder to be kind than to be wise";
static char rx_data[RX_BUFF_SIZE] = { 0 };

static void test_done(struct device *dev, u32_t id, int error_code)
{
	if (error_code == 0) {
		TC_PRINT("DMA transfer done\n");
	} else {
		TC_PRINT("DMA transfer met an error\n");
	}
}

static int test_task(u32_t chan_id, u32_t blen)
{
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	struct device *dma = device_get_binding(DMA_DEVICE_NAME);

	if (!dma) {
		TC_PRINT("Cannot get dma controller\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 0;
	dma_cfg.error_callback_en = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	TC_PRINT("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
			chan_id, blen >> 3);

	TC_PRINT("Starting the transfer\n");
	dma_block_cfg.block_size = strlen(tx_data);
	dma_block_cfg.source_address = (u32_t)tx_data;
	dma_block_cfg.dest_address = (u32_t)rx_data;

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}
	k_sleep(2000);
	TC_PRINT("%s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0)
		return TC_FAIL;
	return TC_PASS;
}

/* export test cases */
void test_dma_m2m_chan0_burst8(void)
{
	zassert_true((test_task(0, 8) == TC_PASS), NULL);
}

void test_dma_m2m_chan1_burst8(void)
{
	zassert_true((test_task(1, 8) == TC_PASS), NULL);
}

void test_dma_m2m_chan0_burst16(void)
{
	zassert_true((test_task(0, 16) == TC_PASS), NULL);
}

void test_dma_m2m_chan1_burst16(void)
{
	zassert_true((test_task(1, 16) == TC_PASS), NULL);
}
