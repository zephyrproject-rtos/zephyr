/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample I2S codec output
 *
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "i2s_codec.h"
#include "stereo_out.h"

LOG_MODULE_REGISTER(i2s_codec, CONFIG_LOG_DEFAULT_LEVEL);

#define I2S_CODEC_THREAD_PRIO   5
#define I2S_CODEC_STACK_SIZE    1024U
#define I2S_CODEC_CHANNELS      2U
#define I2S_CODEC_WORD_SIZE     16U
#define I2S_CODEC_BLOCK_MS      10U
#define I2S_CODEC_BLOCK_FRAMES  (STEREO_OUT_SAMPLE_RATE_HZ * I2S_CODEC_BLOCK_MS / MSEC_PER_SEC)
#define I2S_CODEC_BLOCK_SIZE    (I2S_CODEC_BLOCK_FRAMES * I2S_CODEC_CHANNELS * sizeof(int16_t))
#define I2S_CODEC_BLOCK_COUNT   4U
#define I2S_CODEC_MCLK_HZ       (32U * STEREO_OUT_SAMPLE_RATE_HZ)
#define I2S_CODEC_TIMEOUT_MS    100
#define I2S_CODEC_RETRY_MAX_MS  1000
#define CODEC_RECOVER_MAX       3U
#define VOLUME_MIN_DB           (-64)
#define VOLUME_MAX_DB           0
#define VOLUME_STEP_DB          3
#define VOLUME_INITIAL_DB       (-20)

enum codec_request {
	CODEC_REQ_RECOVER,
	CODEC_REQ_VOLUME_UP,
	CODEC_REQ_VOLUME_DOWN,
	CODEC_REQ_VOLUME_SET,
	CODEC_REQ_MUTE_TOGGLE,
};

BUILD_ASSERT(K_LOWEST_APPLICATION_THREAD_PRIO >= I2S_CODEC_THREAD_PRIO);

K_MEM_SLAB_DEFINE_STATIC(i2s_tx_slab, I2S_CODEC_BLOCK_SIZE, I2S_CODEC_BLOCK_COUNT, 4);
K_THREAD_STACK_DEFINE(i2s_codec_thread_stack, I2S_CODEC_STACK_SIZE);

static const struct device *const codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
static const struct device *const i2s_dev = DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx));
static struct k_thread i2s_codec_thread;
static struct k_work codec_work;
static atomic_t codec_requests;
static atomic_t initialized;
static int volume_db = VOLUME_INITIAL_DB;
static bool muted;
static uint8_t recover_cnt;

static void i2s_codec_downmix(int16_t *block)
{
	for (size_t i = 0U; i < I2S_CODEC_BLOCK_FRAMES; i++) {
		const int32_t sum = (int32_t)block[i * 2U] + block[i * 2U + 1U];

		block[i * 2U] = (int16_t)(sum / 2);
		block[i * 2U + 1U] = 0;
	}
}

static int i2s_codec_start_stream(void)
{
	int err;

	err = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (err != 0) {
		LOG_ERR("Failed to drop I2S stream: %d", err);
		return err;
	}

	for (size_t i = 0U; i < I2S_CODEC_BLOCK_COUNT; i++) {
		void *block;

		err = k_mem_slab_alloc(&i2s_tx_slab, &block, K_NO_WAIT);
		if (err != 0) {
			LOG_ERR("Failed to allocate I2S block: %d", err);
			return err;
		}

		(void)memset(block, 0, I2S_CODEC_BLOCK_SIZE);

		err = i2s_write(i2s_dev, block, I2S_CODEC_BLOCK_SIZE);
		if (err != 0) {
			LOG_ERR("Failed to queue I2S block: %d", err);
			k_mem_slab_free(&i2s_tx_slab, block);
			return err;
		}
	}

	err = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (err != 0) {
		LOG_ERR("Failed to start I2S: %d", err);
	}

	return err;
}

static void i2s_codec_restart_stream(void)
{
	int32_t backoff_ms = I2S_CODEC_BLOCK_MS;

	while (i2s_codec_start_stream() != 0) {
		LOG_WRN("Retrying I2S start in %d ms", backoff_ms);
		k_msleep(backoff_ms);
		backoff_ms = MIN(backoff_ms * 2, I2S_CODEC_RETRY_MAX_MS);
	}
}

static void i2s_codec_thread_func(void *arg1, void *arg2, void *arg3)
{
	size_t underrun_cnt = 0U;
	size_t cnt = 0U;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		uint32_t size;
		void *block;
		int err;

		err = k_mem_slab_alloc(&i2s_tx_slab, &block, K_MSEC(I2S_CODEC_TIMEOUT_MS));
		if (err != 0) {
			LOG_ERR("No I2S block released: %d", err);
			i2s_codec_restart_stream();
			continue;
		}

		size = stereo_out_read(block, I2S_CODEC_BLOCK_SIZE);
		if (size != I2S_CODEC_BLOCK_SIZE) {
			(void)memset((uint8_t *)block + size, 0, I2S_CODEC_BLOCK_SIZE - size);
			underrun_cnt++;
		}

		i2s_codec_downmix(block);

		if (CONFIG_INFO_REPORTING_INTERVAL > 0 &&
		    (++cnt % CONFIG_INFO_REPORTING_INTERVAL) == 0U) {
			LOG_INF("[%zu]: Sent I2S audio, %zu underruns", cnt, underrun_cnt);
		}

		err = i2s_write(i2s_dev, block, I2S_CODEC_BLOCK_SIZE);
		if (err != 0) {
			LOG_ERR("Failed to write I2S block: %d", err);
			k_mem_slab_free(&i2s_tx_slab, block);
			i2s_codec_restart_stream();
		}
	}
}

static void codec_set_volume(void)
{
	const audio_property_value_t val = {.vol = volume_db};
	int err;

	err = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
				       val);
	if (err != 0) {
		LOG_ERR("Failed to set volume: %d", err);
		return;
	}

	LOG_INF("Volume %d dB", volume_db);
}

static void codec_set_mute(void)
{
	const audio_property_value_t val = {.mute = muted};
	int err;

	err = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
				       val);
	if (err != 0) {
		LOG_ERR("Failed to set mute: %d", err);
		return;
	}

	LOG_INF("%s", muted ? "Muted" : "Unmuted");
}

static void codec_recover(void)
{
	int err;

	if (recover_cnt >= CODEC_RECOVER_MAX) {
		return;
	}

	recover_cnt++;
	if (recover_cnt == CODEC_RECOVER_MAX) {
		LOG_ERR("Codec error, restarting the output for the last time");
	} else {
		LOG_WRN("Codec error, restarting the output");
	}

	err = audio_codec_clear_errors(codec_dev);
	if (err != 0) {
		LOG_ERR("Failed to clear codec errors: %d", err);
	}

	audio_codec_stop_output(codec_dev);
	audio_codec_start_output(codec_dev);
}

static void codec_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (atomic_test_and_clear_bit(&codec_requests, CODEC_REQ_RECOVER)) {
		codec_recover();
	}

	if (atomic_test_and_clear_bit(&codec_requests, CODEC_REQ_VOLUME_UP)) {
		volume_db = MIN(volume_db + VOLUME_STEP_DB, VOLUME_MAX_DB);
		atomic_set_bit(&codec_requests, CODEC_REQ_VOLUME_SET);
	}

	if (atomic_test_and_clear_bit(&codec_requests, CODEC_REQ_VOLUME_DOWN)) {
		volume_db = MAX(volume_db - VOLUME_STEP_DB, VOLUME_MIN_DB);
		atomic_set_bit(&codec_requests, CODEC_REQ_VOLUME_SET);
	}

	if (atomic_test_and_clear_bit(&codec_requests, CODEC_REQ_VOLUME_SET)) {
		codec_set_volume();
	}

	if (atomic_test_and_clear_bit(&codec_requests, CODEC_REQ_MUTE_TOGGLE)) {
		muted = !muted;
		codec_set_mute();
	}
}

static void codec_request(enum codec_request req)
{
	if (!atomic_get(&initialized)) {
		return;
	}

	atomic_set_bit(&codec_requests, req);
	(void)k_work_submit(&codec_work);
}

static void codec_error_cb(const struct device *dev, uint32_t errors)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(errors);

	codec_request(CODEC_REQ_RECOVER);
}

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->value == 0) {
		return;
	}

	switch (evt->code) {
	case INPUT_KEY_VOLUMEUP:
		codec_request(CODEC_REQ_VOLUME_UP);
		break;
	case INPUT_KEY_VOLUMEDOWN:
		codec_request(CODEC_REQ_VOLUME_DOWN);
		break;
	case INPUT_KEY_MUTE:
		codec_request(CODEC_REQ_MUTE_TOGGLE);
		break;
	default:
		break;
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

int i2s_codec_init(void)
{
	struct audio_codec_cfg codec_cfg = {
		.mclk_freq = I2S_CODEC_MCLK_HZ,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s = {
			.word_size = I2S_CODEC_WORD_SIZE,
			.channels = I2S_CODEC_CHANNELS,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET,
			.frame_clk_freq = STEREO_OUT_SAMPLE_RATE_HZ,
			.mem_slab = &i2s_tx_slab,
			.block_size = I2S_CODEC_BLOCK_SIZE,
		},
	};
	const struct i2s_config i2s_cfg = {
		.word_size = I2S_CODEC_WORD_SIZE,
		.channels = I2S_CODEC_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
		.frame_clk_freq = STEREO_OUT_SAMPLE_RATE_HZ,
		.mem_slab = &i2s_tx_slab,
		.block_size = I2S_CODEC_BLOCK_SIZE,
		.timeout = I2S_CODEC_TIMEOUT_MS,
	};
	int err;

	if (atomic_get(&initialized)) {
		return -EALREADY;
	}

	if (!device_is_ready(codec_dev) || !device_is_ready(i2s_dev)) {
		LOG_ERR("Codec or I2S device not ready");
		return -ENODEV;
	}

	k_work_init(&codec_work, codec_work_handler);

	err = audio_codec_register_error_callback(codec_dev, codec_error_cb);
	if (err != 0) {
		LOG_ERR("Failed to register codec error callback: %d", err);
		return err;
	}

	err = audio_codec_configure(codec_dev, &codec_cfg);
	if (err != 0) {
		LOG_ERR("Failed to configure codec: %d", err);
		return err;
	}

	err = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (err != 0) {
		LOG_ERR("Failed to configure I2S: %d", err);
		return err;
	}

	err = i2s_codec_start_stream();
	if (err != 0) {
		return err;
	}

	k_thread_create(&i2s_codec_thread, i2s_codec_thread_stack,
			K_THREAD_STACK_SIZEOF(i2s_codec_thread_stack), i2s_codec_thread_func, NULL,
			NULL, NULL, I2S_CODEC_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&i2s_codec_thread, "i2s_codec");

	audio_codec_start_output(codec_dev);

	atomic_set(&initialized, 1);
	codec_request(CODEC_REQ_VOLUME_SET);

	LOG_INF("I2S codec output initialized");

	return 0;
}
