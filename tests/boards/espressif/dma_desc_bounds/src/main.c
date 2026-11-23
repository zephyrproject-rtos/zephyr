/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Descriptor exhaustion on the ESP32 GDMA driver.
 *
 * CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM is pinned to one here, so the boundary sits at a
 * single descriptor's payload rather than at 64 KiB and the test needs no large buffers.
 * A transfer that fits must be accepted and one byte more must be refused, through
 * dma_config() and through dma_reload() alike.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>

#include <hal/dma_types.h>

#define DMA_NODE DT_NODELABEL(tst_dma0)

/* Taken from the HAL rather than written out, so the boundary follows the descriptor format. */
#define DESC_BYTES ((uint32_t)DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED)

#define CHANNEL     0U
#define CHANNEL_MAX DT_PROP(DMA_NODE, dma_channels)

static const struct device *const dma = DEVICE_DT_GET(DMA_NODE);

static uint8_t src_buf[DESC_BYTES + 1U] __aligned(4);
static uint8_t dst_buf[DESC_BYTES + 1U] __aligned(4);

static int config_transfer(uint32_t channel, uint32_t size)
{
	struct dma_block_config block = {
		.source_address = (uint32_t)(uintptr_t)src_buf,
		.dest_address = (uint32_t)(uintptr_t)dst_buf,
		.block_size = size,
	};
	struct dma_config config = {
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.head_block = &block,
	};

	return dma_config(dma, channel, &config);
}

ZTEST(dma_esp32_desc_bounds, test_a_transfer_that_fits_is_accepted)
{
	zassert_ok(config_transfer(CHANNEL, DESC_BYTES),
		   "%u bytes is exactly one descriptor and must be accepted", DESC_BYTES);
}

ZTEST(dma_esp32_desc_bounds, test_one_byte_past_the_pool_is_refused)
{
	zassert_equal(config_transfer(CHANNEL, DESC_BYTES + 1U), -EINVAL,
		      "%u bytes needs a second descriptor and must be refused", DESC_BYTES + 1U);
}

/*
 * The reload path builds the same chain and used to write through the pointer one past
 * the array, into whatever the neighbouring channel keeps there.
 */
ZTEST(dma_esp32_desc_bounds, test_reload_past_the_pool_is_refused)
{
	zassert_ok(config_transfer(CHANNEL, DESC_BYTES), "setup configure must pass");

	zassert_equal(dma_reload(dma, CHANNEL, (uint32_t)(uintptr_t)src_buf,
				 (uint32_t)(uintptr_t)dst_buf, DESC_BYTES + 1U),
		      -EINVAL, "reload of %u bytes must be refused", DESC_BYTES + 1U);
}

ZTEST(dma_esp32_desc_bounds, test_an_out_of_range_channel_is_refused)
{
	struct dma_status status;

	zassert_equal(config_transfer(CHANNEL_MAX, DESC_BYTES), -EINVAL, "config");
	zassert_equal(dma_start(dma, CHANNEL_MAX), -EINVAL, "start");
	zassert_equal(dma_stop(dma, CHANNEL_MAX), -EINVAL, "stop");
	zassert_equal(dma_get_status(dma, CHANNEL_MAX, &status), -EINVAL, "get_status");
	zassert_equal(dma_reload(dma, CHANNEL_MAX, (uint32_t)(uintptr_t)src_buf,
				 (uint32_t)(uintptr_t)dst_buf, 1U),
		      -EINVAL, "reload");
}

static void *setup(void)
{
	zassert_true(device_is_ready(dma), "DMA device not ready");
	return NULL;
}

ZTEST_SUITE(dma_esp32_desc_bounds, NULL, setup, NULL, NULL, NULL);
