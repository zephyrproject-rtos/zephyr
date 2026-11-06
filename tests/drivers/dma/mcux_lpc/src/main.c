/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define DMA_NODE     DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_lpc_dma)
#define DMA_CHANNELS DT_PROP(DMA_NODE, dma_channels)

static const struct device *const dma = DEVICE_DT_GET(DMA_NODE);
static uint32_t source;
static volatile uint32_t destination;
static bool channel_configured;
static int completion_status;
static K_SEM_DEFINE(completed, 0, 1);

static void assert_unavailable_channel(uint32_t channel, int32_t stop_ret)
{
	struct dma_status status = {
		.busy = true,
		.dir = MEMORY_TO_PERIPHERAL,
		.pending_length = sizeof(source),
	};

	zassert_equal(dma_start(dma, channel), -EINVAL, "Starting channel %u", channel);
	zassert_equal(dma_reload(dma, channel, (uintptr_t)&source, (uintptr_t)&destination,
				 sizeof(source)),
		      -EINVAL, "Reloading channel %u", channel);
	zassert_equal(dma_get_status(dma, channel, &status), -EINVAL, "Querying channel %u",
		      channel);
	zassert_true(status.busy);
	zassert_equal(status.dir, MEMORY_TO_PERIPHERAL);
	zassert_equal(status.pending_length, sizeof(source));
	zassert_equal(dma_stop(dma, channel), stop_ret, "Stopping channel %u", channel);
	zassert_equal(dma_stop(dma, channel), stop_ret, "Repeated stop of channel %u", channel);
}

static void transfer_done(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	completion_status = status;
	k_sem_give(user_data);
}

static void run_transfer(uint32_t channel)
{
	int ret;

	k_sem_reset(&completed);
	completion_status = -EINPROGRESS;
	zassert_ok(dma_start(dma, channel));
	ret = k_sem_take(&completed, K_SECONDS(2));
	zassert_ok(dma_stop(dma, channel));
	zassert_ok(ret, "DMA completion timed out");
	zassert_equal(completion_status, DMA_STATUS_COMPLETE);
	zassert_equal(destination, source);
}

ZTEST(dma_mcux_lpc, test_invalid_channel)
{
	zassert_true(device_is_ready(dma));
	assert_unavailable_channel(DMA_CHANNELS, -EINVAL);
	assert_unavailable_channel(UINT32_MAX, -EINVAL);
}

ZTEST(dma_mcux_lpc, test_channel_before_and_after_configure)
{
	struct dma_block_config block = {
		.source_address = (uintptr_t)&source,
		.dest_address = (uintptr_t)&destination,
		.block_size = sizeof(source),
	};
	struct dma_config config = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = sizeof(source),
		.dest_data_size = sizeof(destination),
		.source_burst_length = sizeof(source),
		.dest_burst_length = sizeof(destination),
		.block_count = 1,
		.head_block = &block,
		.dma_callback = transfer_done,
		.user_data = &completed,
	};
	struct dma_status status;
	uint32_t channel = DMA_CHANNELS - 1U;

	zassert_true(device_is_ready(dma));

	/* The configured slot survives stop and suite repetition. */
	for (uint32_t i = 0; i < DMA_CHANNELS; i++) {
		if (!channel_configured || i != channel) {
			assert_unavailable_channel(i, 0);
		}
	}

	/* The hardware channel ID may exceed the number of allocated slots. */
	zassert_ok(dma_config(dma, channel, &config));
	channel_configured = true;
	for (uint32_t i = 0; i < channel; i++) {
		assert_unavailable_channel(i, 0);
	}

	zassert_ok(dma_get_status(dma, channel, &status));
	zassert_false(status.busy);
	zassert_equal(status.pending_length, 0);
	zassert_equal(status.dir, MEMORY_TO_MEMORY);

	source = 0x12345678U;
	destination = 0U;
	run_transfer(channel);

	source = 0x87654321U;
	destination = 0U;
	zassert_ok(dma_reload(dma, channel, (uintptr_t)&source, (uintptr_t)&destination,
			      sizeof(source)));
	run_transfer(channel);

	zassert_ok(dma_get_status(dma, channel, &status));
	zassert_false(status.busy);
	zassert_equal(status.pending_length, 0);
	zassert_equal(status.dir, MEMORY_TO_MEMORY);
	zassert_ok(dma_stop(dma, channel));
}

ZTEST_SUITE(dma_mcux_lpc, NULL, NULL, NULL, NULL, NULL);
