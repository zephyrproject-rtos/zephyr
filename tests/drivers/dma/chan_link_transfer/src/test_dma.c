/*
 * Copyright (c) 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma link transfer
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transfer tx -> rx
 *   -# after a major/minor loop trigger another channel to transfer, rx -> rx2
 * - Expected Results
 *   -# Data is transferred correctly from src to dest
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/dma.h>
#include <ztest.h>

#define DMA_DEVICE_NAME CONFIG_DMA_LINK_TRANSFER_DRV_NAME
#define TEST_DMA_CHANNEL_0 (0)
#define TEST_DMA_CHANNEL_1 (1)
#define RX_BUFF_SIZE (48)

#ifdef CONFIG_NOCACHE_MEMORY
static __aligned(32) char tx_data[RX_BUFF_SIZE] __used
	__attribute__((__section__(".nocache")));
static const char TX_DATA[] = "It is harder to be kind than to be wise........";
static __aligned(32) char rx_data[RX_BUFF_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
static __aligned(32) char rx_data2[RX_BUFF_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
#else
static const char tx_data[] = "It is harder to be kind than to be wise........";
static char rx_data[RX_BUFF_SIZE] = { 0 };
static char rx_data2[RX_BUFF_SIZE] = { 0 };
#endif

static void test_done(const struct device *dma_dev, void *arg, uint32_t id,
		      int error_code)
{
	if (error_code == 0) {
		TC_PRINT("DMA transfer done ch %d\n", id);
	} else {
		TC_PRINT("DMA transfer met an error\n");
	}
}

static int test_task(int minor, int major)
{
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg = { 0 };
	const struct device *dma = device_get_binding(DMA_DEVICE_NAME);

	if (!dma) {
		TC_PRINT("Cannot get dma controller\n");
		return TC_FAIL;
	}

#ifdef CONFIG_NOCACHE_MEMORY
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 16;
	dma_cfg.dest_burst_length = 16;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_en = 1U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	TC_PRINT("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
		 TEST_DMA_CHANNEL_1, 8 >> 3);

	TC_PRINT("Starting the transfer\n");
	(void)memset(rx_data, 0, sizeof(rx_data));
	(void)memset(rx_data2, 0, sizeof(rx_data2));

	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data2;

	if (dma_config(dma, TEST_DMA_CHANNEL_1, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START + 1;
#endif

	dma_cfg.source_chaining_en = minor;
	dma_cfg.dest_chaining_en = major;
	dma_cfg.linked_channel = TEST_DMA_CHANNEL_1;

	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;

	if (dma_config(dma, TEST_DMA_CHANNEL_0, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	if (dma_start(dma, TEST_DMA_CHANNEL_0)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}
	k_sleep(K_MSEC(2000));
	TC_PRINT("%s\n", rx_data);
	TC_PRINT("%s\n", rx_data2);
	if (minor == 0 && major == 1) {
		/* major link only trigger lined channel minor loop once */
		if (strncmp(tx_data, rx_data2,
				dma_cfg.source_burst_length) != 0)
			return TC_FAIL;
	} else if (minor == 1 && major == 0) {
		/* minor link trigger linked channel except the last one*/
		if (strncmp(tx_data, rx_data2,
				dma_block_cfg.block_size - dma_cfg.source_burst_length) != 0)
			return TC_FAIL;
	} else if (minor == 1 && major == 1) {
		if (strcmp(tx_data, rx_data2) != 0)
			return TC_FAIL;
	}

	return TC_PASS;
}

/* export test cases */
void test_dma_m2m_chan0_1_major_link(void)
{
	zassert_true((test_task(0, 1) == TC_PASS), NULL);
}

void test_dma_m2m_chan0_1_minor_link(void)
{
	zassert_true((test_task(1, 0) == TC_PASS), NULL);
}

void test_dma_m2m_chan0_1_minor_major_link(void)
{
	zassert_true((test_task(1, 1) == TC_PASS), NULL);
}
