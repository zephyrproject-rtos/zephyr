/*
 * Copyright (c) 2020, 2026 NXP
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

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#include "dma_test.h"

#define GUARD_BUFF_SIZE (16)
#define RX_BUFF_SIZE (48)

static const char test_pattern[RX_BUFF_SIZE] = "It is harder to be kind than to be wise........";

static DMA_TEST_BUF char tx_data[RX_BUFF_SIZE];
static DMA_TEST_BUF char rx_data[RX_BUFF_SIZE + GUARD_BUFF_SIZE];
static DMA_TEST_BUF char rx_data2[RX_BUFF_SIZE + GUARD_BUFF_SIZE];

static int check_overflow_buffer(const uint8_t *buf, int len)
{
	for (int i = 0; i < len; ++i) {
		if (buf[i] != 0xA5) {
			return 1;
		}
	}

	return 0;
}

static void test_done(const struct device *dma_dev, void *arg, uint32_t id,
		      int status)
{
	if (status >= 0) {
		TC_PRINT("DMA transfer done ch %d\n", id);
	} else {
		TC_PRINT("DMA transfer met an error\n");
	}
}

static int test_task(const struct dma_test_dev *td, int minor, int major)
{
	const struct device *dma = td->dev;
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg = { 0 };
	struct dma_test_channels chans;
	uint32_t chan0, chan1;

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	zassert_ok(dma_test_channels_get(td, 2, &chans));
	if (chans.count < 2) {
		TC_PRINT("%s gives the test one channel, two are needed to link\n", dma->name);
		dma_test_channels_put(td, &chans);
		ztest_test_skip();
		return TC_SKIP;
	}
	chan0 = chans.id[0];
	chan1 = chans.id[1];

	memcpy(tx_data, test_pattern, sizeof(test_pattern));

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 16;
	dma_cfg.dest_burst_length = 16;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	TC_PRINT("Preparing DMA Controller: %s, Chan_ID=%u, BURST_LEN=%u\n",
		 dma->name, chan1, 8 >> 3);

	TC_PRINT("Starting the transfer\n");
	(void)memset(rx_data, 0, RX_BUFF_SIZE);
	(void)memset(rx_data + RX_BUFF_SIZE, 0xA5, GUARD_BUFF_SIZE);
	(void)memset(rx_data2, 0, RX_BUFF_SIZE);
	(void)memset(rx_data2 + RX_BUFF_SIZE, 0xA5, GUARD_BUFF_SIZE);

	dma_block_cfg.block_size = sizeof(tx_data);
#ifdef CONFIG_DMA_64BIT
	dma_block_cfg.source_address = (uint64_t)tx_data;
	dma_block_cfg.dest_address = (uint64_t)rx_data2;
#else
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data2;
#endif

	if (dma_config(dma, chan1, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START + 1;
#endif

	dma_cfg.source_chaining_en = minor;
	dma_cfg.dest_chaining_en = major;
	dma_cfg.linked_channel = chan1;

	dma_block_cfg.block_size = sizeof(tx_data);
#ifdef CONFIG_DMA_64BIT
	dma_block_cfg.source_address = (uint64_t)tx_data;
	dma_block_cfg.dest_address = (uint64_t)rx_data;
#else
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;
#endif

	if (dma_config(dma, chan0, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	if (dma_start(dma, chan0)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}
	k_sleep(K_MSEC(2000));
	TC_PRINT("%s\n", rx_data);
	TC_PRINT("%s\n", rx_data2);
	if (minor == 0 && major == 1) {
		/* major link only trigger lined channel minor loop once */
		if (strncmp(tx_data, rx_data2, dma_cfg.source_burst_length) != 0) {
			return TC_FAIL;
		}
	} else if (minor == 1 && major == 0) {
		/* minor link trigger linked channel except the last one*/
		if (strncmp(tx_data, rx_data2,
			    dma_block_cfg.block_size - dma_cfg.source_burst_length) != 0) {
			return TC_FAIL;
		}
	} else if (minor == 1 && major == 1) {
		if (strcmp(tx_data, rx_data2) != 0) {
			return TC_FAIL;
		}
	}

	if (check_overflow_buffer(rx_data + RX_BUFF_SIZE, GUARD_BUFF_SIZE)) {
		TC_PRINT("rx_data guard pattern has been overwritten.");
		return TC_FAIL;
	}
	if (check_overflow_buffer(rx_data2 + RX_BUFF_SIZE, GUARD_BUFF_SIZE)) {
		TC_PRINT("rx_data2 guard pattern has been overwritten.");
		return TC_FAIL;
	}

	dma_test_channels_put(td, &chans);

	return TC_PASS;
}

/* One set of test cases per DMA controller the board says links channels, so
 * a failure on one controller does not prevent the remaining controllers
 * from running.
 */
#define DEFINE_DMA_M2M_LINK_TESTS(idx, _)                                                          \
	COND_CODE_1(DMA_TEST_INST_HAS(idx, channel_link), (DMA_M2M_LINK_TESTS(idx)), ())

#define DMA_M2M_LINK_TESTS(idx)                                                                    \
	ZTEST(dma_m2m_link, test_dma##idx##_m2m_chan0_1_major_link)                                \
	{                                                                                          \
		zassert_true(test_task(&dma_test_devs[idx], 0, 1) == TC_PASS,                      \
			     "%s failed major link transfer", dma_test_devs[idx].dev->name);       \
	}                                                                                          \
	ZTEST(dma_m2m_link, test_dma##idx##_m2m_chan0_1_minor_link)                                \
	{                                                                                          \
		zassert_true(test_task(&dma_test_devs[idx], 1, 0) == TC_PASS,                      \
			     "%s failed minor link transfer", dma_test_devs[idx].dev->name);       \
	}                                                                                          \
	ZTEST(dma_m2m_link, test_dma##idx##_m2m_chan0_1_minor_major_link)                          \
	{                                                                                          \
		zassert_true(test_task(&dma_test_devs[idx], 1, 1) == TC_PASS,                      \
			     "%s failed minor major link transfer", dma_test_devs[idx].dev->name); \
	}

LISTIFY(DMA_TEST_DEV_COUNT, DEFINE_DMA_M2M_LINK_TESTS, ())

ZTEST_SUITE(dma_m2m_link, NULL, NULL, NULL, NULL, NULL);
