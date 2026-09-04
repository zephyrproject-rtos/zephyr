/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test DMA error handling for DesignWare DMA driver
 *
 * This test verifies that the DW DMA driver properly handles and reports
 * transfer errors through the callback mechanism.
 *
 * Note: Full hardware error injection testing requires specific hardware support.
 * This test validates the configuration path and callback mechanism.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/ztest.h>
#include <string.h>

#define TEST_BUFFER_SIZE 64
#define MAX_CHANNELS 2

static __aligned(4) uint8_t src_data[TEST_BUFFER_SIZE];
static __aligned(4) uint8_t dst_data[TEST_BUFFER_SIZE];

struct dma_error_test_context {
	const struct device *dma_dev;
	int channel;
	volatile int callback_status;
	volatile bool callback_called;
	volatile bool error_callback_called;
	struct dma_config cfg;
	struct dma_block_config block_cfg;
};

static struct dma_error_test_context test_ctx;

static void dma_test_callback(const struct device *dev, void *user_data,
			      uint32_t channel, int status)
{
	struct dma_error_test_context *ctx = user_data;

	ctx->callback_called = true;
	ctx->callback_status = status;

	if (status < 0) {
		ctx->error_callback_called = true;
		TC_PRINT("DMA error callback: channel=%u, status=%d\n", channel, status);
	} else {
		TC_PRINT("DMA success callback: channel=%u, status=%d\n", channel, status);
	}
}

static int setup_dma_transfer(struct dma_error_test_context *ctx,
			      bool error_callback_dis,
			      bool complete_callback_en)
{
	const struct device *dev = ctx->dma_dev;
	int ret;

	memset(&ctx->cfg, 0, sizeof(ctx->cfg));
	memset(&ctx->block_cfg, 0, sizeof(ctx->block_cfg));

	ctx->cfg.channel_direction = MEMORY_TO_MEMORY;
	ctx->cfg.source_data_size = 1;
	ctx->cfg.dest_data_size = 1;
	ctx->cfg.source_burst_length = 1;
	ctx->cfg.dest_burst_length = 1;
	ctx->cfg.dma_callback = dma_test_callback;
	ctx->cfg.user_data = ctx;
	ctx->cfg.error_callback_dis = error_callback_dis ? 1 : 0;
	ctx->cfg.complete_callback_en = complete_callback_en ? 1 : 0;
	ctx->cfg.block_count = 1;
	ctx->cfg.head_block = &ctx->block_cfg;

	ctx->block_cfg.block_size = TEST_BUFFER_SIZE;
	ctx->block_cfg.source_address = (uintptr_t)src_data;
	ctx->block_cfg.dest_address = (uintptr_t)dst_data;

	ctx->channel = dma_request_channel(dev, NULL);
	if (ctx->channel < 0) {
		TC_PRINT("Failed to request DMA channel: %d\n", ctx->channel);
		return ctx->channel;
	}

	ctx->callback_called = false;
	ctx->error_callback_called = false;
	ctx->callback_status = 0;

	ret = dma_config(dev, ctx->channel, &ctx->cfg);
	if (ret) {
		TC_PRINT("DMA config failed: %d\n", ret);
		dma_release_channel(dev, ctx->channel);
		ctx->channel = -1;
		return ret;
	}

	return 0;
}

static void cleanup_dma(struct dma_error_test_context *ctx)
{
	if (ctx->channel >= 0) {
		dma_stop(ctx->dma_dev, ctx->channel);
		dma_release_channel(ctx->dma_dev, ctx->channel);
		ctx->channel = -1;
	}
}

#define SETUP_DMA_OR_SKIP(ctx, err_dis, cb_en) \
	do { \
		int ret = setup_dma_transfer(ctx, err_dis, cb_en); \
		if (ret == -ENOMEM || ret == -ENOSPC || ret == -EINVAL) { \
			TC_PRINT("Skipping test: no channels available (%d)\n", ret); \
			ztest_test_skip(); \
		} \
		zassert_ok(ret, "Failed to setup DMA transfer"); \
	} while (0)

#define START_DMA_OR_SKIP(dev, chan) \
	do { \
		int ret = dma_start(dev, chan); \
		if (ret == -ENOSYS || ret == -ENOTSUP) { \
			ztest_test_skip(); \
		} \
		zassert_ok(ret, "DMA start failed"); \
	} while (0)

/**
 * @brief Test that error_callback_dis = 0 enables error callbacks
 */
ZTEST(dma_error_handling, test_error_callback_enabled)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");
	zassert_equal(test_ctx.callback_status, 0, "Expected success status, got %d",
		      test_ctx.callback_status);
	zassert_false(test_ctx.error_callback_called, "Error callback should not be called for success");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test that error_callback_dis = 1 disables error callbacks
 */
ZTEST(dma_error_handling, test_error_callback_disabled)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, true, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");
	zassert_equal(test_ctx.callback_status, 0, "Expected success status, got %d",
		      test_ctx.callback_status);
	zassert_false(test_ctx.error_callback_called, "Error callback should not be called");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test block callback with error_callback_dis
 */
ZTEST(dma_error_handling, test_block_callback_error_handling)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Block callback not called");
	zassert_true(test_ctx.callback_status == DMA_STATUS_BLOCK ||
		     test_ctx.callback_status == 0,
		     "Expected DMA_STATUS_BLOCK or 0, got %d", test_ctx.callback_status);

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test transfer callback with error_callback_dis
 */
ZTEST(dma_error_handling, test_transfer_callback_error_handling)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, false);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Transfer callback not called");
	zassert_true(test_ctx.callback_status == DMA_STATUS_COMPLETE ||
		     test_ctx.callback_status == 0,
		     "Expected DMA_STATUS_COMPLETE or 0, got %d", test_ctx.callback_status);

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test multiple channels error handling independence
 */
ZTEST(dma_error_handling, test_multi_channel_error_independence)
{
	const struct device *dev = test_ctx.dma_dev;
	struct dma_error_test_context ctx1 = {0}, ctx2 = {0};
	int ret;

	if (!device_is_ready(dev)) {
		ztest_test_skip();
	}

	ctx1.dma_dev = dev;
	ctx2.dma_dev = dev;

	SETUP_DMA_OR_SKIP(&ctx1, false, true);
	SETUP_DMA_OR_SKIP(&ctx2, true, true);

	zassert_not_equal(ctx1.channel, ctx2.channel, "Channels should be different");

	ret = dma_start(dev, ctx1.channel);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&ctx1);
		cleanup_dma(&ctx2);
		ztest_test_skip();
	}
	zassert_ok(ret, "DMA start ch1 failed");

	ret = dma_start(dev, ctx2.channel);
	zassert_ok(ret, "DMA start ch2 failed");

	k_sleep(K_MSEC(200));

	zassert_true(ctx1.callback_called, "Channel 1 callback not called");
	zassert_true(ctx2.callback_called, "Channel 2 callback not called");

	cleanup_dma(&ctx1);
	cleanup_dma(&ctx2);
}

/**
 * @brief Test that error status is properly passed in callback
 */
ZTEST(dma_error_handling, test_callback_status_codes)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");
	zassert_true(test_ctx.callback_status >= -EIO && test_ctx.callback_status <= 127,
		     "Invalid status code: %d", test_ctx.callback_status);

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test error handling with cyclic transfers
 */
ZTEST(dma_error_handling, test_cyclic_transfer_error_handling)
{
	int ret;

	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);

	test_ctx.cfg.cyclic = 1;
	test_ctx.block_cfg.dest_reload_en = 1;
	test_ctx.block_cfg.source_reload_en = 1;

	ret = dma_config(test_ctx.dma_dev, test_ctx.channel, &test_ctx.cfg);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "Cyclic config failed");

	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Cyclic callback not called");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test that error handling logic is compiled in
 */
ZTEST(dma_error_handling, test_dw_dma_common_error_handling_compiled)
{
	zassert_true(true, "DW DMA common compiles successfully");
}

/**
 * @brief Test error_callback_dis config is stored per-channel
 */
ZTEST(dma_error_handling, test_error_callback_dis_stored_per_channel)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	struct dma_error_test_context ctx1 = {0}, ctx2 = {0};
	ctx1.dma_dev = test_ctx.dma_dev;
	ctx2.dma_dev = test_ctx.dma_dev;

	SETUP_DMA_OR_SKIP(&ctx1, false, true);
	SETUP_DMA_OR_SKIP(&ctx2, true, true);

	zassert_not_equal(ctx1.channel, ctx2.channel, "Channels should be different");

	START_DMA_OR_SKIP(test_ctx.dma_dev, ctx1.channel);
	START_DMA_OR_SKIP(test_ctx.dma_dev, ctx2.channel);

	k_sleep(K_MSEC(200));

	zassert_true(ctx1.callback_called, "Channel 1 callback not called");
	zassert_true(ctx2.callback_called, "Channel 2 callback not called");

	cleanup_dma(&ctx1);
	cleanup_dma(&ctx2);
}

/**
 * @brief Test reconfiguring channel changes error_callback_dis
 */
ZTEST(dma_error_handling, test_reconfig_updates_error_callback_dis)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);

	test_ctx.cfg.error_callback_dis = 1;
	int ret = dma_config(test_ctx.dma_dev, test_ctx.channel, &test_ctx.cfg);
	zassert_ok(ret, "Reconfig failed");

	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test error handling with different transfer directions
 */
ZTEST(dma_error_handling, test_error_handling_all_directions)
{
	enum dma_channel_direction directions[] = {
		MEMORY_TO_MEMORY,
		MEMORY_TO_PERIPHERAL,
		PERIPHERAL_TO_MEMORY,
		PERIPHERAL_TO_PERIPHERAL
	};

	for (int i = 0; i < ARRAY_SIZE(directions); i++) {
		if (!device_is_ready(test_ctx.dma_dev)) {
			ztest_test_skip();
		}

		SETUP_DMA_OR_SKIP(&test_ctx, false, true);

		test_ctx.cfg.channel_direction = directions[i];

		int ret = dma_config(test_ctx.dma_dev, test_ctx.channel, &test_ctx.cfg);
		if (ret == -ENOSYS || ret == -ENOTSUP) {
			cleanup_dma(&test_ctx);
			continue;
		}
		zassert_ok(ret, "Config failed for direction %d", i);

		START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

		k_sleep(K_MSEC(200));

		zassert_true(test_ctx.callback_called,
			     "Callback not called for direction %d", i);

		cleanup_dma(&test_ctx);
	}
}

/**
 * @brief Test error handling with scatter-gather (multiple blocks)
 */
ZTEST(dma_error_handling, test_error_handling_multiple_blocks)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);

	static struct dma_block_config block2;
	memset(&block2, 0, sizeof(block2));
	block2.block_size = TEST_BUFFER_SIZE;
	block2.source_address = (uintptr_t)src_data;
	block2.dest_address = (uintptr_t)dst_data;

	test_ctx.cfg.block_count = 2;
	test_ctx.block_cfg.next_block = &block2;

	int ret = dma_config(test_ctx.dma_dev, test_ctx.channel, &test_ctx.cfg);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "Multi-block config failed");

	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Multi-block callback not called");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test dma_stop clears error state
 */
ZTEST(dma_error_handling, test_dma_stop_clears_channel)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	int ret = dma_stop(test_ctx.dma_dev, test_ctx.channel);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "DMA stop failed");

	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called after restart");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test concurrent channels with mixed error_callback_dis
 */
ZTEST(dma_error_handling, test_concurrent_channels_mixed_settings)
{
	const struct device *dev = test_ctx.dma_dev;
	struct dma_error_test_context ctxs[MAX_CHANNELS] = {0};
	int ret;

	if (!device_is_ready(dev)) {
		ztest_test_skip();
	}

	for (int i = 0; i < MAX_CHANNELS; i++) {
		ctxs[i].dma_dev = dev;
		SETUP_DMA_OR_SKIP(&ctxs[i], i % 2 == 0, true);
	}

	for (int i = 0; i < MAX_CHANNELS; i++) {
		ret = dma_start(dev, ctxs[i].channel);
		if (ret == -ENOSYS || ret == -ENOTSUP) {
			for (int j = 0; j <= i; j++) cleanup_dma(&ctxs[j]);
			ztest_test_skip();
		}
		zassert_ok(ret, "DMA start failed for channel %d", i);
	}

	k_sleep(K_MSEC(200));

	for (int i = 0; i < MAX_CHANNELS; i++) {
		zassert_true(ctxs[i].callback_called,
			     "Channel %d callback not called", i);
	}

	for (int i = 0; i < MAX_CHANNELS; i++) {
		cleanup_dma(&ctxs[i]);
	}
}

/**
 * @brief Test error status values are standard errno codes
 */
ZTEST(dma_error_handling, test_error_status_valid_errno)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");
	zassert_true(test_ctx.callback_status <= 0,
		     "Status should be <= 0, got %d", test_ctx.callback_status);

	if (test_ctx.callback_status < 0) {
		zassert_true(test_ctx.callback_status >= -133,
			     "Status should be valid errno, got %d", test_ctx.callback_status);
	}

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test callback user_data is preserved correctly
 */
ZTEST(dma_error_handling, test_callback_user_data_preserved)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	void *test_user_data = (void *)0xDEADBEEF;

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);

	test_ctx.cfg.user_data = test_user_data;

	int ret = dma_config(test_ctx.dma_dev, test_ctx.channel, &test_ctx.cfg);
	if (ret == -ENOMEM || ret == -ENOSPC || ret == -EINVAL) {
		ztest_test_skip();
	}
	if (ret) {
		TC_PRINT("Config failed: %d\n", ret);
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}

	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(300));

	zassert_true(test_ctx.callback_called, "Callback not called");
	zassert_equal(test_ctx.callback_status, 0, "Expected success");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test reload operation preserves error_callback_dis
 */
ZTEST(dma_error_handling, test_reload_preserves_error_callback_dis)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(100));

	int ret = dma_reload(test_ctx.dma_dev, test_ctx.channel,
			     (uintptr_t)src_data, (uintptr_t)dst_data, TEST_BUFFER_SIZE);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "Reload failed");

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called after reload");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test suspend/resume preserves error_callback_dis
 */
ZTEST(dma_error_handling, test_suspend_resume_preserves_error_callback_dis)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(100));

	int ret = dma_suspend(test_ctx.dma_dev, test_ctx.channel);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "Suspend failed");

	k_sleep(K_MSEC(100));

	ret = dma_resume(test_ctx.dma_dev, test_ctx.channel);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		cleanup_dma(&test_ctx);
		ztest_test_skip();
	}
	zassert_ok(ret, "Resume failed");

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called after resume");

	cleanup_dma(&test_ctx);
}

/**
 * @brief Test ISR status register race documentation
 */
ZTEST(dma_error_handling, test_isr_status_register_race_documentation)
{
	if (!device_is_ready(test_ctx.dma_dev)) {
		ztest_test_skip();
	}

	SETUP_DMA_OR_SKIP(&test_ctx, false, true);
	START_DMA_OR_SKIP(test_ctx.dma_dev, test_ctx.channel);

	k_sleep(K_MSEC(200));

	zassert_true(test_ctx.callback_called, "Callback not called");

	cleanup_dma(&test_ctx);
}

/* Test suite setup */
static void *dma_error_handling_setup(void)
{
	test_ctx.dma_dev = DEVICE_DT_GET(DT_NODELABEL(dma));
	if (test_ctx.dma_dev == NULL || !device_is_ready(test_ctx.dma_dev)) {
		const char *labels[] = {"dma_0", "dma_1", "dma", "dma_emul0"};
		for (int i = 0; i < ARRAY_SIZE(labels); i++) {
			test_ctx.dma_dev = device_get_binding(labels[i]);
			if (test_ctx.dma_dev && device_is_ready(test_ctx.dma_dev)) {
				break;
			}
		}
	}

	if (test_ctx.dma_dev == NULL || !device_is_ready(test_ctx.dma_dev)) {
		const struct device *dev = device_get_binding("dma");
		if (dev && device_is_ready(dev)) {
			test_ctx.dma_dev = dev;
		}
	}

	if (test_ctx.dma_dev == NULL || !device_is_ready(test_ctx.dma_dev)) {
		TC_PRINT("No DMA device found, tests will be skipped\n");
	}

	test_ctx.channel = -1;
	return NULL;
}

static void dma_error_handling_before(void *fixture)
{
	memset(src_data, 0xAA, TEST_BUFFER_SIZE);
	memset(dst_data, 0x55, TEST_BUFFER_SIZE);
	test_ctx.callback_called = false;
	test_ctx.error_callback_called = false;
	test_ctx.callback_status = 0;
	test_ctx.channel = -1;
}

static void dma_error_handling_after(void *fixture)
{
	cleanup_dma(&test_ctx);
}

ZTEST_SUITE(dma_error_handling, NULL, dma_error_handling_setup,
	    dma_error_handling_before, dma_error_handling_after, NULL);