/*
 * Copyright (c) 2023 Tom Finet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma chained memory to memory transfer
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Configure block chaining.
 *   -# Start transfer
 * - Expected Results
 *   -# Data in both blocks is transferred correctly from src to dest
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define RX_BUFF_SIZE (48)

#ifdef CONFIG_NOCACHE_MEMORY
static __aligned(32) char tx_data1[RX_BUFF_SIZE] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION)));
static const char TX_DATA1[] = "It is harder to be kind than to be wise........";
static __aligned(32) char rx_data1[RX_BUFF_SIZE] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION".dma")));
static __aligned(32) char tx_data2[RX_BUFF_SIZE] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION)));
static const char TX_DATA2[] = "It is easier to be wise than to be kind........";
static __aligned(32) char rx_data2[RX_BUFF_SIZE] __used
	__attribute__((__section__(CONFIG_DMA_LOOP_TRANSFER_SRAM_SECTION".dma")));
#else
static const char tx_data1[] = "It is harder to be kind than to be wise........";
static char rx_data1[RX_BUFF_SIZE] = { 0 };
static const char tx_data2[] = "It is easier to be wise than to be kind........";
static char rx_data2[RX_BUFF_SIZE] = { 0 };
#endif

static void test_done(const struct device *dma_dev, void *arg,
		      uint32_t id, int status)
{
	if (status >= 0) {
		TC_PRINT("DMA chained transfer done\n");
	} else {
		TC_PRINT("DMA chained transfer met an error\n");
	}
}

static int test_task(const struct device *dma, uint32_t chan_id, uint32_t blen)
{
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg1 = { 0 };
	struct dma_block_config dma_block_cfg2 = { 0 };

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

#ifdef CONFIG_NOCACHE_MEMORY
	memcpy(tx_data1, TX_DATA1, sizeof(TX_DATA1));
	memcpy(tx_data2, TX_DATA2, sizeof(TX_DATA2));
#endif

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_en = 1U;
	dma_cfg.block_count = 2U;
	dma_cfg.head_block = &dma_block_cfg1;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	TC_PRINT("Preparing DMA Controller: Name=%s, Chan_ID=%u, BURST_LEN=%u\n",
		 dma->name, chan_id, blen >> 3);

	TC_PRINT("Starting the transfer\n");
	(void)memset(rx_data1, 0, sizeof(rx_data1));
	(void)memset(rx_data2, 0, sizeof(rx_data2));
	dma_block_cfg1.block_size = sizeof(tx_data1);
	dma_block_cfg2.block_size = sizeof(tx_data2);
#ifdef CONFIG_DMA_64BIT
	dma_block_cfg1.source_address = (uint64_t)tx_data1;
	dma_block_cfg1.dest_address = (uint64_t)rx_data1;
	dma_block_cfg2.source_address = (uint64_t)tx_data2;
	dma_block_cfg2.dest_address = (uint64_t)rx_data2;
#else
	dma_block_cfg1.source_address = (uint32_t)tx_data1;
	dma_block_cfg1.dest_address = (uint32_t)rx_data1;
	dma_block_cfg2.source_address = (uint32_t)tx_data2;
	dma_block_cfg2.dest_address = (uint32_t)rx_data2;
#endif
	dma_block_cfg1.next_block = &dma_block_cfg2;

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}
	k_sleep(K_MSEC(2000));

	TC_PRINT("%s\n", rx_data1);
	TC_PRINT("%s\n", rx_data2);
	if (strcmp(tx_data1, rx_data1) != 0 || strcmp(tx_data2, rx_data2) != 0) {
		return TC_FAIL;
	}
	return TC_PASS;
}

#define DMA_NAME(i, _) test_dma##i
#define DMA_LIST       LISTIFY(CONFIG_DMA_LOOP_TRANSFER_NUMBER_OF_DMAS, DMA_NAME, (,))

#define TEST_TASK(dma_name)                                                                        \
	ZTEST(dma_chain_m2m, test_##dma_name##_chain_m2m_chan0_burst8)                             \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 8) == TC_PASS));    \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_chain_m2m, test_##dma_name##_chain_m2m_chan1_burst8)                             \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 8) == TC_PASS));    \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_chain_m2m, test_##dma_name##_chain_m2m_chan0_burst16)                            \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 16) == TC_PASS));   \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_chain_m2m, test_##dma_name##_chain_m2m_chan1_burst16)                            \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 16) == TC_PASS));   \
	}

FOR_EACH(TEST_TASK, (), DMA_LIST);
