/*
 * Copyright 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>
#include "sine.h"
LOG_MODULE_REGISTER(codec_sample);

#define TONE_SAMPLE_RATE 16000
#define AUDIO_BLOCK_SIZE 320
#define BLOCK_SAMPLES    (AUDIO_BLOCK_SIZE / sizeof(int16_t))
#define SPEAKER_VOL      15

static bool loopback;
static struct sine_gen tone_gen;

/* The tone is generated one block at a time. Alternate between two blocks so
 * the one handed to the codec is not overwritten by the next fill.
 */
static int16_t __aligned(4) tone_blocks[2][BLOCK_SAMPLES];
static uint8_t tone_block_idx;

static void tx_done(const struct device *dev, void *user_data)
{
	if (!loopback) {
		int16_t *block = tone_blocks[tone_block_idx];

		sine_gen_fill_s16(&tone_gen, block, BLOCK_SAMPLES, 1);
		audio_codec_write(dev, (uint8_t *)block, AUDIO_BLOCK_SIZE);
		tone_block_idx ^= 1U;
	}
}
static void rx_done(const struct device *dev, uint8_t *buf, uint32_t len, void *user_data)
{
	if (loopback) {
		audio_codec_write(dev, buf, AUDIO_BLOCK_SIZE);
	}
}

int main(void)
{
	static const struct device *dev;
	audio_property_value_t val;
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_PCM,
		.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TX,
		.dai_cfg.pcm.pcm_width = AUDIO_PCM_WIDTH_16_BITS,
		.dai_cfg.pcm.channels = 1,
		.dai_cfg.pcm.block_size = AUDIO_BLOCK_SIZE,
		.dai_cfg.pcm.samplerate = AUDIO_PCM_RATE_16K,
	};

	LOG_INF("Audio codec sample");
	sine_gen_init(&tone_gen, CONFIG_SAMPLE_TONE_HZ, TONE_SAMPLE_RATE);
#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(codec0))
	dev = DEVICE_DT_GET(DT_ALIAS(codec0));
#else
	LOG_ERR("No audio codec on this board. Skipping audio test.\n");
	return 0;
#endif
	if (!device_is_ready(dev)) {
		LOG_ERR("codec device is not ready\n");
		return -EBUSY;
	}
	LOG_INF("codec device is ready");
	k_sleep(K_MSEC(1000));

	LOG_INF("codec playback example");
	loopback = false;
	if (audio_codec_configure(dev, &cfg) < 0) {
		LOG_ERR("configure codec error\n");
		return -EIO;
	}
	if (audio_codec_register_done_callback(dev, tx_done, NULL, rx_done, NULL) < 0) {
		LOG_ERR("could not register codec callbacks\n");
		return -EIO;
	}
	audio_codec_start(dev, AUDIO_DAI_DIR_TX);
	LOG_INF("playback started");
	val.vol = SPEAKER_VOL;
	if (audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, val) < 0) {
		LOG_ERR("could not set volume\n");
		return -EIO;
	}
	k_sleep(K_MSEC(15000));
	audio_codec_stop(dev, AUDIO_DAI_DIR_TX);
	LOG_INF("codec transfer stopped");

	LOG_INF("codec loopback example");
	loopback = true;
	cfg.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TXRX;
	if (audio_codec_configure(dev, &cfg) < 0) {
		LOG_ERR("configure codec error\n");
		return -EIO;
	}
	if (audio_codec_register_done_callback(dev, tx_done, NULL, rx_done, NULL) < 0) {
		LOG_ERR("could not register codec callbacks\n");
		return -EIO;
	}
	audio_codec_start(dev, AUDIO_DAI_DIR_TXRX);
	LOG_INF("loopback started");
	if (audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, val) < 0) {
		LOG_ERR("could not set volume\n");
		return -EIO;
	}
	k_sleep(K_MSEC(15000));
	audio_codec_stop(dev, AUDIO_DAI_DIR_TXRX);
	LOG_INF("loopback stopped");

	LOG_INF("Exiting");
	return 0;
}
