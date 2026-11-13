/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define BLOCK_COUNT 3U
#define BLOCK_SIZE  128U
#define RX_CHANNEL  0U

static const struct device *const dma = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));
static struct dma_status completed_status;
static int callback_status;
static int get_status_result;
K_SEM_DEFINE(completed, 0, 1);

static void transfer_done(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(user_data);

	if (channel != RX_CHANNEL || status == DMA_STATUS_BLOCK) {
		return;
	}

	callback_status = status;
	get_status_result = dma_get_status(dev, channel, &completed_status);
	k_sem_give(&completed);
}

static void check_progress(const size_t sizes[BLOCK_COUNT])
{
	static uint8_t tx[BLOCK_COUNT][BLOCK_SIZE] __aligned(64);
	static uint8_t rx[BLOCK_COUNT][BLOCK_SIZE] __aligned(64);
	struct dma_block_config blocks[BLOCK_COUNT] = {0};
	struct dma_config config = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = BLOCK_COUNT,
		.head_block = blocks,
		.dma_callback = transfer_done,
	};
	uint64_t total = 0U;

	k_sem_reset(&completed);
	callback_status = -EIO;
	get_status_result = -EIO;
	memset(&completed_status, 0, sizeof(completed_status));
	memset(rx, 0, sizeof(rx));
	for (size_t i = 0U; i < BLOCK_COUNT; i++) {
		memset(tx[i], (int)(i + 1U), sizeof(tx[i]));
		blocks[i].source_address = (uint32_t)(uintptr_t)tx[i];
		blocks[i].dest_address = (uint32_t)(uintptr_t)rx[i];
		blocks[i].block_size = sizes[i];
		blocks[i].next_block = i + 1U < BLOCK_COUNT ? &blocks[i + 1U] : NULL;
		total += sizes[i];
	}

	zassert_ok(dma_config(dma, RX_CHANNEL, &config));
	zassert_ok(dma_start(dma, RX_CHANNEL));
	zassert_ok(k_sem_take(&completed, K_SECONDS(1)), "DMA completion timed out");
	zassert_ok(callback_status, "DMA transfer failed: %d", callback_status);
	zassert_ok(get_status_result, "DMA status failed: %d", get_status_result);
	zassert_equal(completed_status.read_position, BLOCK_COUNT - 1U);
	zassert_equal(completed_status.total_copied, total);
	for (size_t i = 0U; i < BLOCK_COUNT; i++) {
		zassert_mem_equal(tx[i], rx[i], sizes[i], "Block %zu differs", i);
	}
}

ZTEST(esp32_dma_status, test_short_middle_descriptor)
{
	const size_t sizes[BLOCK_COUNT] = {128U, 32U, 64U};

	check_progress(sizes);
}

ZTEST(esp32_dma_status, test_long_middle_descriptor)
{
	const size_t sizes[BLOCK_COUNT] = {32U, 128U, 64U};

	check_progress(sizes);
}

ZTEST(esp32_dma_status, test_uniform_descriptors)
{
	const size_t sizes[BLOCK_COUNT] = {64U, 64U, 64U};

	check_progress(sizes);
}

ZTEST(esp32_dma_status, test_short_final_descriptor)
{
	const size_t sizes[BLOCK_COUNT] = {64U, 64U, 32U};

	check_progress(sizes);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(dma_stop(dma, RX_CHANNEL));
}

static void *setup(void)
{
	zassert_true(device_is_ready(dma), "DMA device is not ready");
	return NULL;
}

ZTEST_SUITE(esp32_dma_status, NULL, setup, NULL, after, NULL);
