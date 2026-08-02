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

static void init_xfer(struct chan_ctx *ctx, struct dma_config *dma_cfg,
		      struct dma_block_config *block_cfg, uint8_t fill_seed)
{
	memset(ctx, 0, sizeof(*ctx));
	k_sem_init(&ctx->sem, 0, 1);
	for (int j = 0; j < XFER_SIZE; j++) {
		ctx->tx[j] = (uint8_t)(j + fill_seed);
	}

	memset(dma_cfg, 0, sizeof(*dma_cfg));
	memset(block_cfg, 0, sizeof(*block_cfg));
	block_cfg->block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
	block_cfg->source_address = (uint64_t)(uintptr_t)ctx->tx;
	block_cfg->dest_address = (uint64_t)(uintptr_t)ctx->rx;
#else
	block_cfg->source_address = (uint32_t)(uintptr_t)ctx->tx;
	block_cfg->dest_address = (uint32_t)(uintptr_t)ctx->rx;
#endif
	dma_cfg->channel_direction = MEMORY_TO_MEMORY;
	dma_cfg->source_data_size = 1U;
	dma_cfg->dest_data_size = 1U;
	dma_cfg->source_burst_length = 1U;
	dma_cfg->dest_burst_length = 1U;
	dma_cfg->complete_callback_en = 1U;
	dma_cfg->user_data = ctx;
	dma_cfg->dma_callback = dma_emul_test_callback;
	dma_cfg->block_count = 1U;
	dma_cfg->head_block = block_cfg;
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

/*
 * ztest runs a suite's tests in name order. This test needs both
 * channels in the UNUSED state they have at boot, and its name sorts
 * between test_chan_filter_exact_request (which only requests and
 * releases channels, leaving the state untouched) and
 * test_concurrent_channels (the first test that configures them).
 * It leaves channel 1 STOPPED, which every later test tolerates.
 */
ZTEST(dma_emul_concurrent, test_channel_chain_unconfigured_target)
{
	static struct dma_config dma_cfg;
	static struct dma_block_config block_cfg;
	struct chan_ctx *ctx = &chan_ctxs[1];

	memset(ctx, 0, sizeof(*ctx));
	k_sem_init(&ctx->sem, 0, 1);

	block_cfg.block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
	block_cfg.source_address = (uint64_t)(uintptr_t)ctx->tx;
	block_cfg.dest_address = (uint64_t)(uintptr_t)ctx->rx;
#else
	block_cfg.source_address = (uint32_t)(uintptr_t)ctx->tx;
	block_cfg.dest_address = (uint32_t)(uintptr_t)ctx->rx;
#endif
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.complete_callback_en = 1U;
	dma_cfg.user_data = ctx;
	dma_cfg.dma_callback = dma_emul_test_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &block_cfg;
	dma_cfg.source_chaining_en = 1U;
	dma_cfg.linked_channel = 0U;

	zassert_ok(dma_config(dma_dev, 1, &dma_cfg), "failed to configure channel 1");

	/*
	 * Channel 0 was never configured. Starting the chain must be
	 * rejected before any state changes; otherwise the start would
	 * mark the zero-initialized channel STARTED and the work handler
	 * would call its NULL dma_callback.
	 */
	zassert_equal(dma_start(dma_dev, 1), -EINVAL,
		      "chain to an unconfigured channel was not rejected");

	/* Channel 0 is still UNUSED: starting it directly is invalid. */
	zassert_equal(dma_start(dma_dev, 0), -EIO, "channel 0 is not UNUSED anymore");

	/* No work was submitted, so no callback fired. */
	zassert_equal(ctx->calls, 0, "callback fired for a rejected start");

	/* Leave channel 1 reconfigurable for the following tests. */
	zassert_ok(dma_stop(dma_dev, 1));
}

/*
 * This test needs channel 0 in the UNUSED state: its name sorts between
 * test_channel_chain_unconfigured_target (which leaves channel 0 UNUSED)
 * and test_concurrent_channels (the first test that configures it).
 */
ZTEST(dma_emul_concurrent, test_channel_stop_unused_noop)
{
	/*
	 * Stopping a channel that was never configured is a successful
	 * no-op, like stopping an already stopped channel. It must not
	 * mark the channel STOPPED: dma_emul_start() accepts STOPPED
	 * channels as linked-chain targets, so a stopped-but-never-
	 * configured channel would run with the zero-initialized
	 * configuration and call its NULL dma_callback.
	 */
	zassert_ok(dma_stop(dma_dev, 0), "stop on a never-configured channel failed");

	/* The no-op stop changed no state: channel 0 is still UNUSED. */
	zassert_equal(dma_start(dma_dev, 0), -EIO, "channel 0 is not UNUSED anymore");
}

ZTEST(dma_emul_concurrent, test_concurrent_channels)
{
	static struct dma_config dma_cfg[NUM_CHANNELS];
	static struct dma_block_config block_cfg[NUM_CHANNELS];

	zassert_true(device_is_ready(dma_dev), "dma controller device is not ready");

	for (int i = 0; i < NUM_CHANNELS; i++) {
		init_xfer(&chan_ctxs[i], &dma_cfg[i], &block_cfg[i], 0x40 * i);

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

ZTEST(dma_emul_concurrent, test_linked_channel_validation)
{
	static struct dma_config dma_cfg[NUM_CHANNELS];
	static struct dma_block_config block_cfg[NUM_CHANNELS];
	static uint8_t buf[NUM_CHANNELS][XFER_SIZE];

	for (int i = 0; i < NUM_CHANNELS; i++) {
		memset(&dma_cfg[i], 0, sizeof(dma_cfg[i]));
		memset(&block_cfg[i], 0, sizeof(block_cfg[i]));

		block_cfg[i].block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
		block_cfg[i].source_address = (uint64_t)(uintptr_t)buf[i];
		block_cfg[i].dest_address = (uint64_t)(uintptr_t)buf[i];
#else
		block_cfg[i].source_address = (uint32_t)(uintptr_t)buf[i];
		block_cfg[i].dest_address = (uint32_t)(uintptr_t)buf[i];
#endif
		dma_cfg[i].channel_direction = MEMORY_TO_MEMORY;
		dma_cfg[i].source_data_size = 1U;
		dma_cfg[i].dest_data_size = 1U;
		dma_cfg[i].source_burst_length = 1U;
		dma_cfg[i].dest_burst_length = 1U;
		dma_cfg[i].block_count = 1U;
		dma_cfg[i].head_block = &block_cfg[i];
		dma_cfg[i].source_chaining_en = 1U;
	}

	/* An out-of-range link target is rejected at configure time. */
	dma_cfg[0].linked_channel = NUM_CHANNELS;
	zassert_equal(dma_config(dma_dev, 0, &dma_cfg[0]), -EINVAL);

	/* A link pointing at the channel itself is rejected too. */
	dma_cfg[0].linked_channel = 0U;
	zassert_equal(dma_config(dma_dev, 0, &dma_cfg[0]), -EINVAL);

	/*
	 * A cycle spanning two channels passes per-link validation but
	 * must fail at start time instead of spinning with interrupts off.
	 */
	dma_cfg[0].linked_channel = 1U;
	dma_cfg[1].linked_channel = 0U;
	zassert_ok(dma_config(dma_dev, 0, &dma_cfg[0]));
	zassert_ok(dma_config(dma_dev, 1, &dma_cfg[1]));
	zassert_equal(dma_start(dma_dev, 0), -EINVAL);

	/* The rejected start changed no state: both channels stop cleanly. */
	zassert_ok(dma_stop(dma_dev, 0));
	zassert_ok(dma_stop(dma_dev, 1));
}

ZTEST(dma_emul_concurrent, test_start_chain_rejects_active_downstream)
{
	static struct dma_config dma_cfg[NUM_CHANNELS];
	static struct dma_block_config block_cfg[NUM_CHANNELS];
	static struct chan_ctx ctx[NUM_CHANNELS];

	for (int i = 0; i < NUM_CHANNELS; i++) {
		init_xfer(&ctx[i], &dma_cfg[i], &block_cfg[i], 0x40 * i);
	}

	/* ch0 chains to ch1; ch1 is a standalone transfer. */
	dma_cfg[0].source_chaining_en = 1U;
	dma_cfg[0].linked_channel = 1U;

	zassert_ok(dma_config(dma_dev, 0, &dma_cfg[0]), "failed to configure channel 0");
	zassert_ok(dma_config(dma_dev, 1, &dma_cfg[1]), "failed to configure channel 1");

	/*
	 * Starting the downstream channel first and then a chain into it
	 * must be rejected: ch1's descriptor is already queued and would
	 * otherwise run a second time from the chained tail.
	 */
	zassert_ok(dma_start(dma_dev, 1), "failed to start channel 1");
	zassert_equal(dma_start(dma_dev, 0), -EINVAL,
		      "chain start into active downstream channel was not rejected");

	/* ch1 still completes exactly once; ch0 was not started. */
	zassert_ok(k_sem_take(&ctx[1].sem, TIMEOUT), "channel 1 transfer did not complete");
	zassert_equal(ctx[1].calls, 1, "channel 1 callback fired %d times", ctx[1].calls);
	zassert_equal(ctx[1].status, DMA_STATUS_COMPLETE, "channel 1 callback status %d",
		      ctx[1].status);
	zassert_equal(ctx[0].calls, 0, "channel 0 callback fired despite rejected start");

	/* Once the downstream channel is idle, starting the chain succeeds. */
	zassert_ok(dma_start(dma_dev, 0), "chain start after downstream completion failed");
	zassert_ok(k_sem_take(&ctx[0].sem, TIMEOUT), "channel 0 transfer did not complete");
	zassert_ok(k_sem_take(&ctx[1].sem, TIMEOUT), "chained channel 1 transfer did not complete");

	zassert_equal(ctx[0].calls, 1, "channel 0 callback fired %d times", ctx[0].calls);
	zassert_equal(ctx[0].status, DMA_STATUS_COMPLETE, "channel 0 callback status %d",
		      ctx[0].status);
	zassert_equal(ctx[1].calls, 2, "channel 1 callback fired %d times", ctx[1].calls);
	zassert_equal(ctx[1].status, DMA_STATUS_COMPLETE, "channel 1 callback status %d",
		      ctx[1].status);
	zassert_mem_equal(ctx[1].rx, ctx[1].tx, XFER_SIZE, "channel 1 data corrupted");
}

ZTEST(dma_emul_concurrent, test_stop_chain_head_restart)
{
	static struct dma_config dma_cfg[NUM_CHANNELS];
	static struct dma_block_config block_cfg[NUM_CHANNELS];
	static struct chan_ctx ctx[NUM_CHANNELS];

	for (int i = 0; i < NUM_CHANNELS; i++) {
		init_xfer(&ctx[i], &dma_cfg[i], &block_cfg[i], 0x40 * i);
	}

	/* ch0 chains to ch1. */
	dma_cfg[0].source_chaining_en = 1U;
	dma_cfg[0].linked_channel = 1U;

	zassert_ok(dma_config(dma_dev, 0, &dma_cfg[0]), "failed to configure channel 0");
	zassert_ok(dma_config(dma_dev, 1, &dma_cfg[1]), "failed to configure channel 1");

	/*
	 * Stop the chain head before the work queue gets a chance to run:
	 * the ztest thread runs at a cooperative priority
	 * (CONFIG_ZTEST_THREAD_PRIORITY defaults to -1), so it cannot be
	 * preempted by the work queue thread, and start plus stop are
	 * atomic with respect to the work handler.
	 */
	zassert_ok(dma_start(dma_dev, 0), "failed to start channel 0");
	zassert_ok(dma_stop(dma_dev, 0), "failed to stop channel 0");

	/*
	 * The cancel is still in flight: STOPPED only means the stop was
	 * requested, not that the work handler has acknowledged it. An
	 * immediate restart must be rejected instead of marking the chain
	 * channels STARTED behind the back of the still-running handler.
	 */
	zassert_equal(dma_start(dma_dev, 0), -EBUSY,
		      "restart while the cancel was in flight was not rejected");

	/* Let the handler observe the stop and acknowledge the cancel. */
	zassert_ok(k_sem_take(&ctx[0].sem, TIMEOUT), "channel 0 cancel callback did not fire");
	zassert_equal(ctx[0].status, -ECANCELED, "channel 0 callback status %d", ctx[0].status);
	zassert_equal(ctx[0].calls, 1, "channel 0 callback fired %d times", ctx[0].calls);
	zassert_equal(ctx[1].calls, 0, "downstream channel ran despite the canceled chain head");

	/*
	 * Once the cancel completed, the whole chain must be restartable:
	 * the handler must have reset the downstream channel it never ran.
	 */
	zassert_ok(dma_start(dma_dev, 0), "chain restart after cancel failed");
	zassert_ok(k_sem_take(&ctx[0].sem, TIMEOUT), "channel 0 transfer did not complete");
	zassert_ok(k_sem_take(&ctx[1].sem, TIMEOUT), "chained channel 1 transfer did not complete");

	zassert_equal(ctx[0].calls, 2, "channel 0 callback fired %d times", ctx[0].calls);
	zassert_equal(ctx[0].status, DMA_STATUS_COMPLETE, "channel 0 callback status %d",
		      ctx[0].status);
	zassert_equal(ctx[1].calls, 1, "channel 1 callback fired %d times", ctx[1].calls);
	zassert_equal(ctx[1].status, DMA_STATUS_COMPLETE, "channel 1 callback status %d",
		      ctx[1].status);
	zassert_mem_equal(ctx[0].rx, ctx[0].tx, XFER_SIZE, "channel 0 data corrupted");
	zassert_mem_equal(ctx[1].rx, ctx[1].tx, XFER_SIZE, "channel 1 data corrupted");
}

ZTEST_SUITE(dma_emul_concurrent, NULL, NULL, NULL, NULL, NULL);
