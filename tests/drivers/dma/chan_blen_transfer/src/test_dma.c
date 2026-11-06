/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2026 NXP
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

	/*
	 * Flush tx_data so the DMA reads committed data from memory, and
	 * invalidate rx_data (including guard area) so the CPU will not observe
	 * stale cache lines after the DMA writes to it.
	 */
	sys_cache_data_flush_range(tx_data, TEST_BUF_SIZE);
	sys_cache_data_flush_and_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);

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

	/*
	 * Invalidate rx_data again after DMA completion to ensure the CPU reads
	 * what the DMA actually wrote, not any speculatively cached lines.
	 */
	sys_cache_data_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);

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

static void run_dma_m2m_test(const struct device *dma, uint32_t chan, uint32_t blen)
{
	zassert_true(test_task(dma, chan, blen) == TC_PASS, "%s failed chan %u burst %u", dma->name,
		     chan, blen);
}

/* Generate one set of test cases per DMA controller under test so a failure
 * on one controller does not prevent the remaining controllers from running.
 */
#define DEFINE_DMA_M2M_BURST8_TESTS(idx, _)                                                        \
	ZTEST(dma_m2m, test_dma##idx##_m2m_chan0_burst8)                                           \
	{                                                                                          \
		run_dma_m2m_test(dma_test_devs[idx], CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 8);         \
	}                                                                                          \
	ZTEST(dma_m2m, test_dma##idx##_m2m_chan1_burst8)                                           \
	{                                                                                          \
		run_dma_m2m_test(dma_test_devs[idx], CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 8);         \
	}

LISTIFY(DMA_TEST_DEV_COUNT, DEFINE_DMA_M2M_BURST8_TESTS, ())

#if CONFIG_DMA_TRANSFER_BURST16
#define DEFINE_DMA_M2M_BURST16_TESTS(idx, _)                                                       \
	ZTEST(dma_m2m, test_dma##idx##_m2m_chan0_burst16)                                          \
	{                                                                                          \
		run_dma_m2m_test(dma_test_devs[idx], CONFIG_DMA_TRANSFER_CHANNEL_NR_0, 16);        \
	}                                                                                          \
	ZTEST(dma_m2m, test_dma##idx##_m2m_chan1_burst16)                                          \
	{                                                                                          \
		run_dma_m2m_test(dma_test_devs[idx], CONFIG_DMA_TRANSFER_CHANNEL_NR_1, 16);        \
	}

LISTIFY(DMA_TEST_DEV_COUNT, DEFINE_DMA_M2M_BURST16_TESTS, ())
#endif

#if defined(CONFIG_DMA_TRANSFER_NO_CALLBACK) || CONFIG_DMA_TRANSFER_IDLE_SLOT >= 0 ||              \
	CONFIG_DMA_TRANSFER_MINOR_LINK_MAX_LOOPS > 0
static void small_block_config(struct dma_config *cfg, struct dma_block_config *blk, uint32_t burst)
{
	*cfg = (struct dma_config){0};
	*blk = (struct dma_block_config){0};

	cfg->channel_direction = MEMORY_TO_MEMORY;
	cfg->source_data_size = 1U;
	cfg->dest_data_size = 1U;
	cfg->source_burst_length = burst;
	cfg->dest_burst_length = burst;
	cfg->dma_callback = test_done;
	cfg->user_data = &transfer_end_sem;
	cfg->block_count = 1U;
	cfg->head_block = blk;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	cfg->dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	blk->block_size = TEST_BUF_SIZE;
	blk->source_address = (uintptr_t)tx_data;
	blk->dest_address = (uintptr_t)rx_data;
}
#endif

#ifdef CONFIG_DMA_TRANSFER_NO_CALLBACK
ZTEST(dma_m2m, test_dma0_m2m_chan0_no_callback)
{
	const struct device *dma = dma_test_devs[0];
	uint32_t chan = CONFIG_DMA_TRANSFER_CHANNEL_NR_0;
	struct dma_config cfg;
	struct dma_block_config blk;
	struct dma_status status;

	zassert_equal(test_task(dma, chan, 8), TC_PASS, "transfer with a callback failed");

	small_block_config(&cfg, &blk, 8U);
	cfg.dma_callback = NULL;
	cfg.user_data = NULL;
	(void)memset(rx_data, 0, TEST_BUF_SIZE);
	sys_cache_data_flush_range(tx_data, TEST_BUF_SIZE);
	sys_cache_data_flush_and_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);
	k_sem_reset(&transfer_end_sem);

	zassert_ok(dma_config(dma, chan, &cfg), "config without a callback failed");
	zassert_ok(dma_start(dma, chan), "start without a callback failed");

	for (int i = 0; i < 100; i++) {
		zassert_ok(dma_get_status(dma, chan, &status), "get_status failed");
		if (!status.busy) {
			break;
		}
		k_msleep(1);
	}
	zassert_false(status.busy, "transfer without a callback did not finish");
	zassert_equal(k_sem_count_get(&transfer_end_sem), 0,
		      "the callback of the previous configuration was called");

	sys_cache_data_invd_range(rx_data, TEST_BUF_SIZE + GUARD_BUF_SIZE);
	zassert_mem_equal(rx_data, tx_data, TEST_BUF_SIZE, "data mismatch");
}
#endif

#if CONFIG_DMA_TRANSFER_IDLE_SLOT >= 0
static K_SEM_DEFINE(idle_end_sem, 0, 1);
static uint8_t idle_rx[TEST_BUF_SIZE];

/* A transfer on a request line that never fires, so it stays pending. */
static void start_idle_transfer(const struct device *dma, uint32_t chan)
{
	struct dma_config cfg;
	struct dma_block_config blk;

	small_block_config(&cfg, &blk, 8U);
	cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	cfg.dma_slot = CONFIG_DMA_TRANSFER_IDLE_SLOT;
	cfg.user_data = &idle_end_sem;
	blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk.dest_address = (uintptr_t)idle_rx;

	k_sem_reset(&idle_end_sem);
	zassert_ok(dma_config(dma, chan, &cfg), "idle channel config failed");
	zassert_ok(dma_start(dma, chan), "idle channel start failed");
}

ZTEST(dma_m2m, test_dma0_chan1_pending_length)
{
	const struct device *dma = dma_test_devs[0];
	uint32_t chan = CONFIG_DMA_TRANSFER_CHANNEL_NR_1;
	struct dma_status status;

	start_idle_transfer(dma, chan);
	zassert_ok(dma_get_status(dma, chan, &status), "get_status failed");
	zassert_true(status.busy, "idle transfer not busy");
	zassert_equal(status.pending_length, TEST_BUF_SIZE, "pending_length %u, expected %u",
		      status.pending_length, TEST_BUF_SIZE);
	zassert_ok(dma_stop(dma, chan), "stop failed");
}

#if CONFIG_DMA_TRANSFER_FAULT_ADDRESS != 0
ZTEST(dma_m2m, test_dma0_m2m_chan0_error_callback)
{
	const struct device *dma = dma_test_devs[0];
	uint32_t chan = CONFIG_DMA_TRANSFER_CHANNEL_NR_0;
	uint32_t idle_chan = CONFIG_DMA_TRANSFER_CHANNEL_NR_1;
	struct dma_config cfg;
	struct dma_block_config blk;
	struct dma_status status;

	start_idle_transfer(dma, idle_chan);

	small_block_config(&cfg, &blk, 8U);
	blk.dest_address = CONFIG_DMA_TRANSFER_FAULT_ADDRESS;
	k_sem_reset(&transfer_end_sem);
	dma_complete_status = 0;

	zassert_ok(dma_config(dma, chan, &cfg), "faulting config failed");
	zassert_ok(dma_start(dma, chan), "faulting start failed");
	zassert_ok(k_sem_take(&transfer_end_sem, K_MSEC(100)), "error not reported");
	zassert_true(dma_complete_status < 0, "error reported as %d", dma_complete_status);

	zassert_ok(dma_get_status(dma, idle_chan, &status), "get_status failed");
	zassert_true(status.busy, "the error stopped another channel");
	zassert_equal(k_sem_count_get(&idle_end_sem), 0, "the other channel got a callback");
	zassert_ok(dma_stop(dma, idle_chan), "stop failed");

	/* The controller must still serve other channels after the error. */
	zassert_equal(test_task(dma, idle_chan, 8), TC_PASS, "transfer after the error failed");
}
#endif
#endif

#if CONFIG_DMA_TRANSFER_MINOR_LINK_MAX_LOOPS > 0
ZTEST(dma_m2m, test_dma0_m2m_chan0_minor_link_limit)
{
	const struct device *dma = dma_test_devs[0];
	uint32_t chan = CONFIG_DMA_TRANSFER_CHANNEL_NR_0;
	struct dma_config cfg;
	struct dma_block_config blk;

	/* Only configured, never started, so the small buffers are enough. */
	small_block_config(&cfg, &blk, 1U);
	cfg.source_chaining_en = 1U;
	cfg.linked_channel = CONFIG_DMA_TRANSFER_CHANNEL_NR_1;

	blk.block_size = CONFIG_DMA_TRANSFER_MINOR_LINK_MAX_LOOPS;
	zassert_ok(dma_config(dma, chan, &cfg), "largest linked block rejected");

	blk.block_size = CONFIG_DMA_TRANSFER_MINOR_LINK_MAX_LOOPS + 1;
	zassert_equal(dma_config(dma, chan, &cfg), -EINVAL, "oversized linked block accepted");
}
#endif
