/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify concurrent transfers on separate channels of a dma_emul
 *        controller complete independently.
 * @details
 * - Test Steps
 *   -# Configure two channels with distinct buffers, callbacks and user data
 *   -# Start both channels without waiting for the first to complete
 * - Expected Results
 *   -# Both transfers complete with data intact
 *   -# Each callback fires exactly once, with its own channel and user data
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define NUM_CHANNELS 2
#define XFER_SIZE    64
#define TIMEOUT      K_MSEC(1000)

struct chan_ctx {
	struct k_sem sem;
	void *seen_user_data;
	uint32_t channel;
	int status;
	int calls;
	uint8_t tx[XFER_SIZE];
	uint8_t rx[XFER_SIZE];
};

static const struct device *const dma_dev = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));
static struct chan_ctx chan_ctxs[NUM_CHANNELS];

static void dma_emul_test_callback(const struct device *dev, void *user_data, uint32_t channel,
				   int status)
{
	struct chan_ctx *ctx = (struct chan_ctx *)user_data;

	ARG_UNUSED(dev);

	ctx->seen_user_data = user_data;
	ctx->channel = channel;
	ctx->status = status;
	ctx->calls++;
	k_sem_give(&ctx->sem);
}

ZTEST(dma_emul_concurrent, test_chan_filter_exact_request)
{
	uint32_t wanted;
	int ch;

	zassert_true(device_is_ready(dma_dev), "dma controller device is not ready");

	/* A NULL filter param keeps the any-free-channel behavior. */
	ch = dma_request_channel(dma_dev, NULL);
	zassert_equal(ch, 0, "NULL filter param did not return the lowest free channel");
	dma_release_channel(dma_dev, (uint32_t)ch);

	/* A filter param pointing to a channel number requests exactly that channel. */
	wanted = 1;
	ch = dma_request_channel(dma_dev, &wanted);
	zassert_equal(ch, 1, "exact request for channel 1 returned %d", ch);

	/* The exact channel is taken now: requesting it again fails. */
	ch = dma_request_channel(dma_dev, &wanted);
	zassert_equal(ch, -EINVAL, "re-requesting taken channel 1 returned %d", ch);

	/* Channel zero is a valid exact request, distinct from a NULL param. */
	wanted = 0;
	ch = dma_request_channel(dma_dev, &wanted);
	zassert_equal(ch, 0, "exact request for channel 0 returned %d", ch);

	/* Both channels are taken: no request can succeed anymore. */
	zassert_equal(dma_request_channel(dma_dev, NULL), -EINVAL);
	wanted = 1;
	zassert_equal(dma_request_channel(dma_dev, &wanted), -EINVAL);

	/* An out-of-range exact request matches no channel at all. */
	wanted = NUM_CHANNELS;
	zassert_equal(dma_request_channel(dma_dev, &wanted), -EINVAL);

	dma_release_channel(dma_dev, 0);
	dma_release_channel(dma_dev, 1);
}

ZTEST(dma_emul_concurrent, test_concurrent_channels)
{
	static struct dma_config dma_cfg[NUM_CHANNELS];
	static struct dma_block_config block_cfg[NUM_CHANNELS];

	zassert_true(device_is_ready(dma_dev), "dma controller device is not ready");

	for (int i = 0; i < NUM_CHANNELS; i++) {
		struct chan_ctx *ctx = &chan_ctxs[i];

		memset(ctx, 0, sizeof(*ctx));
		k_sem_init(&ctx->sem, 0, 1);
		for (int j = 0; j < XFER_SIZE; j++) {
			ctx->tx[j] = (uint8_t)(j + 0x40 * i);
		}

		block_cfg[i].block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
		block_cfg[i].source_address = (uint64_t)(uintptr_t)ctx->tx;
		block_cfg[i].dest_address = (uint64_t)(uintptr_t)ctx->rx;
#else
		block_cfg[i].source_address = (uint32_t)(uintptr_t)ctx->tx;
		block_cfg[i].dest_address = (uint32_t)(uintptr_t)ctx->rx;
#endif

		dma_cfg[i].channel_direction = MEMORY_TO_MEMORY;
		dma_cfg[i].source_data_size = 1U;
		dma_cfg[i].dest_data_size = 1U;
		dma_cfg[i].source_burst_length = 1U;
		dma_cfg[i].dest_burst_length = 1U;
		dma_cfg[i].complete_callback_en = 1U;
		dma_cfg[i].user_data = ctx;
		dma_cfg[i].dma_callback = dma_emul_test_callback;
		dma_cfg[i].block_count = 1U;
		dma_cfg[i].head_block = &block_cfg[i];

		zassert_ok(dma_config(dma_dev, i, &dma_cfg[i]), "failed to configure channel %d",
			   i);
	}

	/* start both channels back to back, without yielding between them */
	zassert_ok(dma_start(dma_dev, 0), "failed to start channel 0");
	zassert_ok(dma_start(dma_dev, 1), "failed to start channel 1");

	zassert_ok(k_sem_take(&chan_ctxs[0].sem, TIMEOUT), "channel 0 transfer did not complete");
	zassert_ok(k_sem_take(&chan_ctxs[1].sem, TIMEOUT), "channel 1 transfer did not complete");

	for (int i = 0; i < NUM_CHANNELS; i++) {
		zassert_equal(chan_ctxs[i].calls, 1, "channel %d callback fired %d times", i,
			      chan_ctxs[i].calls);
		zassert_equal(chan_ctxs[i].channel, i, "channel %d callback got channel %u", i,
			      chan_ctxs[i].channel);
		zassert_equal(chan_ctxs[i].status, DMA_STATUS_COMPLETE,
			      "channel %d callback status %d", i, chan_ctxs[i].status);
		zassert_equal(chan_ctxs[i].seen_user_data, &chan_ctxs[i],
			      "channel %d callback got wrong user data", i);
		zassert_mem_equal(chan_ctxs[i].rx, chan_ctxs[i].tx, XFER_SIZE,
				  "channel %d data corrupted", i);
	}
}

ZTEST_SUITE(dma_emul_concurrent, NULL, NULL, NULL, NULL, NULL);
