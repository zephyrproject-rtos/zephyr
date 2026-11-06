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

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#include "test_buffers.h"

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

#define DMA_NAME(i, _) tst_dma##i
#define DMA_LIST       LISTIFY(CONFIG_DMA_LOOP_TRANSFER_NUMBER_OF_DMAS, DMA_NAME, (,))

#if CONFIG_DMA_TRANSFER_BURST16
#define TEST_TASK(dma_name)                                                                        \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan0_burst8)                                         \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 8) == TC_PASS));    \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan1_burst8)                                         \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 8) == TC_PASS));    \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan0_burst16)                                        \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 16) == TC_PASS));   \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan1_burst16)                                        \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 16) == TC_PASS));   \
	}
#else
#define TEST_TASK(dma_name)                                                                        \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan0_burst8)                                         \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 8) == TC_PASS));    \
	}                                                                                          \
                                                                                                   \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan1_burst8)                                         \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		zassert_true((test_task(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 8) == TC_PASS));    \
	}
#endif

FOR_EACH(TEST_TASK, (), DMA_LIST);

#if CONFIG_DMA_TRANSFER_LONG_BLOCK_SIZE > 0 || CONFIG_DMA_TRANSFER_OVERSIZE_BLOCK_SIZE > 0
/* One block, one-byte data and bursts, so the block size is the loop count. */
static void one_block_config(struct dma_config *cfg, struct dma_block_config *blk, uintptr_t src,
			     uintptr_t dst, uint32_t size)
{
	*cfg = (struct dma_config){0};
	*blk = (struct dma_block_config){0};

	cfg->channel_direction = MEMORY_TO_MEMORY;
	cfg->source_data_size = 1U;
	cfg->dest_data_size = 1U;
	cfg->source_burst_length = 1U;
	cfg->dest_burst_length = 1U;
	cfg->dma_callback = test_done;
	cfg->user_data = &transfer_end_sem;
	cfg->block_count = 1U;
	cfg->head_block = blk;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	cfg->dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	blk->block_size = size;
	blk->source_address = src;
	blk->dest_address = dst;
}
#endif

#if CONFIG_DMA_TRANSFER_LONG_BLOCK_SIZE > 0
static void run_dma_m2m_long_block_test(const struct device *dma, uint32_t chan)
{
	struct dma_config cfg;
	struct dma_block_config blk;

	zassert_true(device_is_ready(dma), "%s not ready", dma->name);

	for (size_t i = 0; i < LONG_BUF_SIZE; i++) {
		long_tx_data[i] = (uint8_t)(i * 7U + (i >> 8));
	}
	(void)memset(long_rx_data, 0, LONG_BUF_SIZE);
	(void)memset(long_rx_data + LONG_BUF_SIZE, 0xA5, GUARD_BUF_SIZE);

	one_block_config(&cfg, &blk, (uintptr_t)long_tx_data, (uintptr_t)long_rx_data,
			 LONG_BUF_SIZE);
	zassert_ok(dma_config(dma, chan, &cfg), "%s chan %u: config failed", dma->name, chan);

	k_sem_reset(&transfer_end_sem);
	sys_cache_data_flush_range(long_tx_data, LONG_BUF_SIZE);
	sys_cache_data_flush_and_invd_range(long_rx_data, LONG_BUF_SIZE + GUARD_BUF_SIZE);

	zassert_ok(dma_start(dma, chan), "%s chan %u: start failed", dma->name, chan);
	if (k_sem_take(&transfer_end_sem, K_MSEC(2000)) != 0) {
		(void)dma_stop(dma, chan);
		zassert_unreachable("%s chan %u: callback not called", dma->name, chan);
	}
	zassert_equal(dma_complete_status, DMA_STATUS_COMPLETE, "%s chan %u: status %d", dma->name,
		      chan, dma_complete_status);

	sys_cache_data_invd_range(long_rx_data, LONG_BUF_SIZE + GUARD_BUF_SIZE);
	zassert_mem_equal(long_rx_data, long_tx_data, LONG_BUF_SIZE, "%s chan %u: data mismatch",
			  dma->name, chan);
	zassert_false(check_overflow_buffer(long_rx_data + LONG_BUF_SIZE, GUARD_BUF_SIZE),
		      "%s chan %u: guard overwritten", dma->name, chan);

	struct dma_status status;

	zassert_ok(dma_get_status(dma, chan, &status), "%s chan %u: get_status failed", dma->name,
		   chan);
	zassert_false(status.busy, "%s chan %u: still busy after completion", dma->name, chan);

#ifdef CONFIG_DMA_TRANSFER_LONG_BLOCK_RELOAD
	zassert_equal(dma_reload(dma, chan, (uintptr_t)long_tx_data, (uintptr_t)long_rx_data,
				 LONG_BUF_SIZE),
		      -EINVAL, "%s chan %u: long reload not rejected", dma->name, chan);

	/*
	 * A short reload after the long block must run alone. A channel left
	 * holding the finished last piece runs that again first, and signals
	 * completion before the reload has been copied.
	 */
	(void)memset(rx_data, 0, TEST_BUF_SIZE);
	(void)memset(rx_data + TEST_BUF_SIZE, 0xA5, GUARD_BUF_SIZE);
	sys_cache_data_flush_range(tx_data, TEST_BUF_SIZE);
	sys_cache_data_flush_and_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);
	k_sem_reset(&transfer_end_sem);

	zassert_ok(dma_reload(dma, chan, (uintptr_t)tx_data, (uintptr_t)rx_data, TEST_BUF_SIZE),
		   "%s chan %u: short reload failed", dma->name, chan);
	zassert_ok(dma_start(dma, chan), "%s chan %u: start after reload failed", dma->name, chan);
	if (k_sem_take(&transfer_end_sem, K_MSEC(2000)) != 0) {
		(void)dma_stop(dma, chan);
		zassert_unreachable("%s chan %u: reload callback not called", dma->name, chan);
	}
	zassert_equal(dma_complete_status, DMA_STATUS_COMPLETE, "%s chan %u: reload status %d",
		      dma->name, chan, dma_complete_status);

	sys_cache_data_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);
	zassert_mem_equal(rx_data, tx_data, TEST_BUF_SIZE, "%s chan %u: reload data mismatch",
			  dma->name, chan);
	zassert_false(check_overflow_buffer(rx_data + TEST_BUF_SIZE, GUARD_BUF_SIZE),
		      "%s chan %u: reload guard overwritten", dma->name, chan);
	(void)dma_stop(dma, chan);
#endif
}

#define LONG_BLOCK_TASK(dma_name)                                                                  \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan0_long_block)                                     \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		run_dma_m2m_long_block_test(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0);                \
	}

FOR_EACH(LONG_BLOCK_TASK, (), DMA_LIST);
#endif

#if CONFIG_DMA_TRANSFER_OVERSIZE_BLOCK_SIZE > 0
static void run_dma_m2m_oversize_block_test(const struct device *dma, uint32_t chan)
{
	struct dma_config cfg;
	struct dma_block_config blk;
	unsigned int key;
	int ret;

	zassert_true(device_is_ready(dma), "%s not ready", dma->name);

	/* Rejected before anything is queued, so the small buffers are enough. */
	one_block_config(&cfg, &blk, (uintptr_t)tx_data, (uintptr_t)rx_data,
			 CONFIG_DMA_TRANSFER_OVERSIZE_BLOCK_SIZE);
	ret = dma_config(dma, chan, &cfg);

	key = irq_lock();
	irq_unlock(key);
	zassert_true(arch_irq_unlocked(key), "%s chan %u: config returned with interrupts locked",
		     dma->name, chan);
	zassert_equal(ret, -EINVAL, "%s chan %u: config returned %d", dma->name, chan, ret);

	zassert_equal(test_task(dma, chan, 8), TC_PASS,
		      "%s chan %u: transfer after a rejected config failed", dma->name, chan);
}

#define OVERSIZE_BLOCK_TASK(dma_name)                                                              \
	ZTEST(dma_m2m, test_##dma_name##_m2m_chan0_oversize_block)                                 \
	{                                                                                          \
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma_name));                  \
		run_dma_m2m_oversize_block_test(dma, CONFIG_DMA_TRANSFER_CHANNEL_NR_0);            \
	}

FOR_EACH(OVERSIZE_BLOCK_TASK, (), DMA_LIST);
#endif
