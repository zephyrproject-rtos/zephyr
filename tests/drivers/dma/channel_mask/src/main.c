/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Check that a DMA driver honors the dma-channel-mask property.
 *
 * The board overlay sets dma-channel-mask on the tst_dma0 controller. A
 * channel with its bit clear is reserved for code outside the driver:
 * dma_request_channel() must not return it and the channel API must return
 * -EINVAL for it. Memory-to-memory transfers on the other channels must still
 * complete, also on channels that share an interrupt line with a reserved one.
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DMA_NODE DT_NODELABEL(tst_dma0)

BUILD_ASSERT(DT_NODE_HAS_PROP(DMA_NODE, dma_channel_mask), "tst_dma0 needs dma-channel-mask");
BUILD_ASSERT(DT_PROP(DMA_NODE, dma_channels) <= 32, "the test covers 32 channels or less");

#define CHANNEL_COUNT DT_PROP(DMA_NODE, dma_channels)
/* Bit n set: Zephyr can use channel n */
#define KERNEL_CHANNELS                                                                            \
	((uint32_t)(DT_PROP(DMA_NODE, dma_channel_mask) & GENMASK(CHANNEL_COUNT - 1, 0)))
#define XFER_SIZE         64U
#define XFER_TIMEOUT      K_MSEC(100)
#define DMA_BUF_ALIGNMENT DT_PROP_OR(DMA_NODE, dma_buf_addr_alignment, 32)

static const struct device *const dma_dev = DEVICE_DT_GET(DMA_NODE);

/* Non-cacheable when the SoC has a data cache, so the CPU sees what the DMA wrote */
static __aligned(DMA_BUF_ALIGNMENT) uint8_t tx_buf[XFER_SIZE] __nocache;
static __aligned(DMA_BUF_ALIGNMENT) uint8_t rx_buf[XFER_SIZE] __nocache;

static K_SEM_DEFINE(xfer_done_sem, 0, 1);
static volatile int xfer_status;

static void xfer_done(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	xfer_status = status;
	k_sem_give(&xfer_done_sem);
}

static bool channel_is_reserved(uint32_t channel)
{
	return (KERNEL_CHANNELS & BIT(channel)) == 0U;
}

static void prepare_m2m_config(struct dma_config *cfg, struct dma_block_config *block)
{
	*block = (struct dma_block_config){
		.source_address = (uintptr_t)tx_buf,
		.dest_address = (uintptr_t)rx_buf,
		.block_size = XFER_SIZE,
	};

	*cfg = (struct dma_config){
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1U,
		.dest_data_size = 1U,
		.source_burst_length = 1U,
		.dest_burst_length = 1U,
		.block_count = 1U,
		.head_block = block,
		.dma_callback = xfer_done,
	};
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	cfg->dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif
}

/**
 * @brief Request channels until the driver has no free channel left.
 *
 * @param request_count Number of successful requests.
 *
 * @return Bitmap of the channels that the driver returned.
 */
static uint32_t request_all_channels(uint32_t *request_count)
{
	uint32_t granted = 0U;

	*request_count = 0U;

	/* One more try than channels: the last request must fail */
	for (int i = 0; i <= CHANNEL_COUNT; i++) {
		int channel = dma_request_channel(dma_dev, NULL);

		if (channel < 0) {
			break;
		}

		granted |= BIT(channel);
		(*request_count)++;
	}

	return granted;
}

static void release_channels(uint32_t channels)
{
	for (uint32_t channel = 0U; channel < CHANNEL_COUNT; channel++) {
		if ((channels & BIT(channel)) != 0U) {
			dma_release_channel(dma_dev, channel);
		}
	}
}

static void check_m2m_transfer(uint32_t channel)
{
	struct dma_config cfg;
	struct dma_block_config block;

	for (uint32_t i = 0U; i < XFER_SIZE; i++) {
		tx_buf[i] = (uint8_t)(channel + i);
	}
	(void)memset(rx_buf, 0, sizeof(rx_buf));
	sys_cache_data_flush_range(tx_buf, sizeof(tx_buf));
	sys_cache_data_flush_and_invd_range(rx_buf, sizeof(rx_buf));

	prepare_m2m_config(&cfg, &block);
	k_sem_reset(&xfer_done_sem);
	xfer_status = -EINPROGRESS;

	zassert_ok(dma_config(dma_dev, channel, &cfg), "config failed on channel %u", channel);
	zassert_ok(dma_start(dma_dev, channel), "start failed on channel %u", channel);

	if (k_sem_take(&xfer_done_sem, XFER_TIMEOUT) != 0) {
		(void)dma_stop(dma_dev, channel);
		zassert_unreachable("no callback on channel %u", channel);
	}

	zassert_equal(xfer_status, DMA_STATUS_COMPLETE, "channel %u status %d", channel,
		      xfer_status);
	sys_cache_data_invd_range(rx_buf, sizeof(rx_buf));
	zassert_mem_equal(rx_buf, tx_buf, XFER_SIZE, "bad data on channel %u", channel);
}

ZTEST(dma_channel_mask, test_request_skips_reserved)
{
	uint32_t request_count;
	uint32_t granted = request_all_channels(&request_count);

	release_channels(granted);

	zassert_equal(request_count, POPCOUNT(granted), "a channel was returned twice");
	zassert_equal(granted & ~KERNEL_CHANNELS, 0U, "reserved channels returned: 0x%08x",
		      granted & ~KERNEL_CHANNELS);
	zassert_equal(granted, KERNEL_CHANNELS, "returned 0x%08x, expected 0x%08x", granted,
		      KERNEL_CHANNELS);
}

ZTEST(dma_channel_mask, test_reserved_channel_api)
{
	struct dma_config cfg;
	struct dma_block_config block;
	struct dma_status status;
	uint32_t tested = 0U;
	int ret;

	prepare_m2m_config(&cfg, &block);

	for (uint32_t channel = 0U; channel < CHANNEL_COUNT; channel++) {
		if (!channel_is_reserved(channel)) {
			continue;
		}

		zassert_equal(dma_config(dma_dev, channel, &cfg), -EINVAL, "config ch %u", channel);
		zassert_equal(dma_start(dma_dev, channel), -EINVAL, "start ch %u", channel);
		zassert_equal(dma_stop(dma_dev, channel), -EINVAL, "stop ch %u", channel);
		zassert_equal(dma_get_status(dma_dev, channel, &status), -EINVAL,
			      "get_status ch %u", channel);
		zassert_equal(dma_reload(dma_dev, channel, (uintptr_t)tx_buf, (uintptr_t)rx_buf,
					 XFER_SIZE),
			      -EINVAL, "reload ch %u", channel);

		/* -ENOSYS: the driver does not implement the operation */
		ret = dma_suspend(dma_dev, channel);
		zassert_true(ret == -EINVAL || ret == -ENOSYS, "suspend ch %u: %d", channel, ret);
		ret = dma_resume(dma_dev, channel);
		zassert_true(ret == -EINVAL || ret == -ENOSYS, "resume ch %u: %d", channel, ret);

		tested++;
	}

	zassert_true(tested > 0U, "the overlay reserves no channel");
}

ZTEST(dma_channel_mask, test_transfer_on_kernel_channels)
{
	for (uint32_t channel = 0U; channel < CHANNEL_COUNT; channel++) {
		if (!channel_is_reserved(channel)) {
			check_m2m_transfer(channel);
		}
	}
}

/* Runs last (tests run in name order): the wrong release changes the driver state */
ZTEST(dma_channel_mask, test_wrong_release_keeps_reserved)
{
	uint32_t request_count;
	uint32_t granted;

	/* A wrong release of a reserved channel must not give it to Zephyr */
	for (uint32_t channel = 0U; channel < CHANNEL_COUNT; channel++) {
		if (channel_is_reserved(channel)) {
			dma_release_channel(dma_dev, channel);
		}
	}

	granted = request_all_channels(&request_count);
	release_channels(granted);

	zassert_equal(granted & ~KERNEL_CHANNELS, 0U, "reserved channels returned: 0x%08x",
		      granted & ~KERNEL_CHANNELS);
}

static void *dma_channel_mask_setup(void)
{
	zassert_true(device_is_ready(dma_dev), "DMA controller is not ready");

	return NULL;
}

ZTEST_SUITE(dma_channel_mask, NULL, dma_channel_mask_setup, NULL, NULL, NULL);
