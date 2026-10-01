/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#define DMA_NODE     DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_lpc_dma)
#define DMA_CHANNELS DT_PROP(DMA_NODE, dma_channels)

static const struct device *const dma = DEVICE_DT_GET(DMA_NODE);
static uint32_t source;
static uint32_t destination;

ZTEST(dma_mcux_lpc, test_stop_invalid_channel)
{
	zassert_true(device_is_ready(dma));
	zassert_equal(dma_stop(dma, DMA_CHANNELS), -EINVAL);
	zassert_equal(dma_stop(dma, UINT32_MAX), -EINVAL);
}

ZTEST(dma_mcux_lpc, test_stop_before_and_after_configure)
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
	};
	uint32_t channel = DMA_CHANNELS - 1U;

	zassert_true(device_is_ready(dma));

	/* No channel has been configured when this test first runs. */
	for (uint32_t i = 0; i < DMA_CHANNELS; i++) {
		zassert_ok(dma_stop(dma, i), "Stopping unconfigured channel %u", i);
		zassert_ok(dma_stop(dma, i), "Repeated stop of channel %u", i);
	}

	/* The hardware channel ID may exceed the number of allocated slots. */
	zassert_ok(dma_config(dma, channel, &config));
	for (uint32_t i = 0; i < channel; i++) {
		zassert_ok(dma_stop(dma, i), "Stopping unmapped channel %u after configuration", i);
		zassert_ok(dma_stop(dma, i), "Repeated stop of unmapped channel %u", i);
	}

	zassert_ok(dma_stop(dma, channel));
	zassert_ok(dma_stop(dma, channel));
}

ZTEST_SUITE(dma_mcux_lpc, NULL, NULL, NULL, NULL, NULL);
