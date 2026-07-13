/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory transfer
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transfer
 * - Expected Results
 *   -# Data is transferred correctly from src to dest
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#include "test_buffers.h"

#define DMA_TEST_NODE      DT_PATH(zephyr_user)
#define DMA_TEST_DEVS_PROP dma_test_devs

#if DT_NODE_HAS_PROP(DMA_TEST_NODE, DMA_TEST_DEVS_PROP)
/* Boards list the DMA controllers to test in a zephyr,user dma-test-devs
 * phandle list.
 */
#define DMA_TEST_DEV_COUNT DT_PROP_LEN(DMA_TEST_NODE, DMA_TEST_DEVS_PROP)
#define DMA_TEST_DEV_GET(idx, _)                                                                   \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DMA_TEST_NODE, DMA_TEST_DEVS_PROP, idx))
#else
/* Legacy boards use tst_dmaN devicetree labels and
 * CONFIG_DMA_LOOP_TRANSFER_NUMBER_OF_DMAS.
 */
#define DMA_TEST_DEV_COUNT CONFIG_DMA_LOOP_TRANSFER_NUMBER_OF_DMAS
#define DMA_TEST_DEV_GET(idx, _) DEVICE_DT_GET(DT_NODELABEL(tst_dma##idx))
#endif

static const struct device *const dma_test_devs[] = {
	LISTIFY(DMA_TEST_DEV_COUNT, DMA_TEST_DEV_GET, (,))
};

static K_SEM_DEFINE(transfer_end_sem, 0, 1);
static int dma_complete_status;

static int check_overflow_buffer(const uint8_t *buf, int len)
{
	for (int i = 0; i < len; ++i) {
		if (buf[i] != 0xA5) {
			return 1;
		}
	}

	return 0;
}

static void test_done(const struct device *dma_dev, void *arg,
		      uint32_t id, int status)
{
	dma_complete_status = status;
	k_sem_give((struct k_sem *)arg);
}

static int test_task(const struct device *dma, uint32_t chan_id, uint32_t blen)
{
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg = { 0 };
	int res;

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.user_data = &transfer_end_sem;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	TC_PRINT("Preparing DMA Controller: Name=%s, Chan_ID=%u, BURST_LEN=%u\n",
		 dma->name, chan_id, blen >> 3);

	TC_PRINT("Starting the transfer\n");
	(void)memset(rx_data, 0, TEST_BUF_SIZE);
	(void)memset(rx_data + TEST_BUF_SIZE, 0xA5, GUARD_BUF_SIZE);
	dma_block_cfg.block_size = TEST_BUF_SIZE;
#ifdef CONFIG_DMA_64BIT
	dma_block_cfg.source_address = (uint64_t)tx_data;
	dma_block_cfg.dest_address = (uint64_t)rx_data;
#else
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;
#endif

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	k_sem_reset(&transfer_end_sem);

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	/*
	 * Use a timeout to ensure the test never deadlocks
	 * even if the DMA driver is buggy and doesn't call
	 * the registered user callback.
	 */
	res = k_sem_take(&transfer_end_sem, K_MSEC(2000));
	if (res == -EAGAIN) {
		TC_PRINT("ERROR: transfer timeout (dma_callback not called)\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: failed to stop DMA transfer");
		}
		return TC_FAIL;
	} /* else: res == 0 (-EBUSY cannot have been returned here)*/

	if (dma_complete_status != DMA_STATUS_COMPLETE) {
		TC_PRINT("DMA transfer error\n");
		return TC_FAIL;
	}

	TC_PRINT("DMA transfer successful\n");
	TC_PRINT("%s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0) {
		return TC_FAIL;
	}
	if (check_overflow_buffer(rx_data + TEST_BUF_SIZE, GUARD_BUF_SIZE)) {
		TC_PRINT("Guard pattern has been overwritten.");
		return TC_FAIL;
	}
	return TC_PASS;
}

static void run_dma_m2m_test(uint32_t chan, uint32_t blen)
{
	for (size_t i = 0; i < ARRAY_SIZE(dma_test_devs); i++) {
		zassert_true(test_task(dma_test_devs[i], chan, blen) == TC_PASS,
			     "%s failed chan %u burst %u", dma_test_devs[i]->name, chan, blen);
	}
}

ZTEST(dma_m2m, test_dma_m2m_chan0_burst8)
{
	run_dma_m2m_test(CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 8);
}

ZTEST(dma_m2m, test_dma_m2m_chan1_burst8)
{
	run_dma_m2m_test(CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 8);
}

#if CONFIG_DMA_TRANSFER_BURST16
ZTEST(dma_m2m, test_dma_m2m_chan0_burst16)
{
	run_dma_m2m_test(CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 16);
}

ZTEST(dma_m2m, test_dma_m2m_chan1_burst16)
{
	run_dma_m2m_test(CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 16);
}
#endif
