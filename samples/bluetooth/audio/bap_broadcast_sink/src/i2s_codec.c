/* Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/time_units.h>

#include "hw_codec.h"

LOG_MODULE_REGISTER(i2s_codec, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_SAMPLE_RATE_HZ 48000U
#define BLOCKS_PER_SEC     100U
#define CHANNELS           2U
#define SAMPLE_BYTES       sizeof(int16_t)
#define MAX_BLOCK_FRAMES   (MAX_SAMPLE_RATE_HZ / BLOCKS_PER_SEC)
#define MAX_BLOCK_SIZE     (MAX_BLOCK_FRAMES * CHANNELS * SAMPLE_BYTES)
#define BLOCK_COUNT        6U
#define PREFILL_BLOCKS     3U
#define MCLK_FS_RATIO      256U

K_MEM_SLAB_DEFINE_STATIC(tx_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, 4);
RING_BUF_DECLARE(pcm_rb, MAX_BLOCK_FRAMES * SAMPLE_BYTES * 8U);

static const struct device *const i2s_dev = DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx));
static const struct device *const codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
static uint32_t block_frames;
static uint32_t queued_blocks;
static bool configured;
static bool started;

int hw_codec_open(void)
{
	if (!device_is_ready(i2s_dev) || !device_is_ready(codec_dev)) {
		LOG_ERR("I2S or codec device not ready");
		return -EIO;
	}

	ring_buf_reset(&pcm_rb);

	return 0;
}

int hw_codec_cfg(uint32_t samplerate)
{
	struct audio_codec_cfg codec_cfg = {0};
	struct i2s_config i2s_cfg = {0};
	int ret;

	if (configured) {
		return -EALREADY;
	}

	if (samplerate == 0U || samplerate > MAX_SAMPLE_RATE_HZ) {
		LOG_ERR("Unsupported sample rate %u", samplerate);
		return -EINVAL;
	}

	block_frames = samplerate / BLOCKS_PER_SEC;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	i2s_cfg.frame_clk_freq = samplerate;
	i2s_cfg.mem_slab = &tx_slab;
	i2s_cfg.block_size = block_frames * CHANNELS * SAMPLE_BYTES;
	i2s_cfg.timeout = 0;

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure I2S (err %d)", ret);
		return ret;
	}

	codec_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	codec_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	codec_cfg.mclk_freq = samplerate * MCLK_FS_RATIO;
	codec_cfg.dai_cfg.i2s = i2s_cfg;
	codec_cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;

	ret = audio_codec_configure(codec_dev, &codec_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure codec (err %d)", ret);
		return ret;
	}

	ring_buf_reset(&pcm_rb);
	queued_blocks = 0U;
	started = false;
	configured = true;

	audio_codec_start_output(codec_dev);

	LOG_INF("I2S codec output: %u Hz, %u frames per block", samplerate, block_frames);

	return 0;
}

static int queue_block(void)
{
	int16_t *out;
	void *block;
	int ret;

	ret = k_mem_slab_alloc(&tx_slab, &block, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	out = block;
	for (uint32_t i = 0U; i < block_frames; i++) {
		int16_t sample;

		(void)ring_buf_get(&pcm_rb, (uint8_t *)&sample, sizeof(sample));
		out[CHANNELS * i] = sample;
		out[CHANNELS * i + 1U] = sample;
	}

	ret = i2s_write(i2s_dev, block, block_frames * CHANNELS * SAMPLE_BYTES);
	if (ret != 0) {
		k_mem_slab_free(&tx_slab, block);
	}

	return ret;
}

uint32_t hw_codec_write_data(const uint8_t *data, uint32_t len)
{
	const uint32_t block_bytes = block_frames * SAMPLE_BYTES;
	bool restarted = false;
	uint32_t put;

	if (!configured) {
		return 0U;
	}

	put = ring_buf_put(&pcm_rb, data, len);
	if (put != len) {
		LOG_WRN("PCM buffer full, dropped %u bytes", len - put);
	}

	while (ring_buf_size_get(&pcm_rb) >= block_bytes) {
		int ret = queue_block();

		if (ret == -EIO && !restarted) {
			/* TX underrun leaves the stream in the error state */
			(void)i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_PREPARE);
			started = false;
			queued_blocks = 0U;
			restarted = true;
			continue;
		}

		if (ret != 0) {
			break;
		}

		queued_blocks++;
		if (!started && queued_blocks >= PREFILL_BLOCKS) {
			ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret != 0) {
				LOG_ERR("Failed to start I2S (err %d)", ret);
				break;
			}
			started = true;
		}
	}

	return put;
}

int hw_codec_close(void)
{
	if (!configured) {
		return -EIO;
	}

	(void)i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);
	audio_codec_stop_output(codec_dev);
	ring_buf_reset(&pcm_rb);
	started = false;
	configured = false;

	return 0;
}
