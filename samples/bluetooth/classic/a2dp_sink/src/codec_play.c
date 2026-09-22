/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/audio/codec.h>
#include "audio_buf.h"
#include "codec_play.h"

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S) && IS_ENABLED(CONFIG_AUDIO_CODEC)

#if CONFIG_NOCACHE_MEMORY
#define __NOCACHE	__attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE	__attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else /* CONFIG_NOCACHE_MEMORY */
#define __NOCACHE
#endif /* CONFIG_NOCACHE_MEMORY */

/* audio stream control variables */
static volatile bool audio_start;
static uint32_t audio_sample_rate;
static uint8_t *audio_data_sync_buf[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT];
static uint32_t audio_data_sync_buf_size[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT];
static uint8_t audio_data_sync_buf_w;
static uint8_t audio_data_sync_buf_r;
static __NOCACHE __aligned(4) uint8_t a2dp_silence_data[A2DP_SBC_DATA_PLAY_SIZE_48K];
#define I2S_CODEC_TX DT_ALIAS(i2s_codec_tx)
#define I2S_TIMEOUT (2000U)

const struct device *const codec_tx = DEVICE_DT_GET(I2S_CODEC_TX);

static __NOCACHE __aligned(4) uint8_t mem_slab_buffer[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT *
						      A2DP_SBC_DATA_PLAY_SIZE_48K];
static struct k_mem_slab mem_slab;

int codec_play_init(void)
{
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

	if (!device_is_ready(codec_tx)) {
		printk("%s is not ready\n", codec_tx->name);
		return -EIO;
	}

	if (!device_is_ready(codec_dev)) {
		printk("%s is not ready\n", codec_dev->name);
		return -EIO;
	}

	return 0;
}

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;
	size_t block_size;

	audio_sample_rate = sample_rate;
	if (sample_rate == 44100) {
		block_size = A2DP_SBC_DATA_PLAY_SIZE_44_1K;
	} else {
		block_size = A2DP_SBC_DATA_PLAY_SIZE_48K;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = sample_width;
	audio_cfg.dai_cfg.i2s.channels = channels;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_USE_CODEC_CLOCK
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
#else
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
#endif
	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = block_size;
	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);

	config.word_size = sample_width;
	config.channels = channels;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_USE_CODEC_CLOCK
	config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
#else
	config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
#endif
	config.frame_clk_freq = sample_rate;
	config.mem_slab = &mem_slab;
	config.block_size = block_size;
	config.timeout = I2S_TIMEOUT;
	if (i2s_configure(codec_tx, I2S_DIR_TX, &config)) {
		printk("failure to config streams\n");
	}

	k_mem_slab_init(&mem_slab, mem_slab_buffer, block_size, CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT);
}

static void codec_play_to_dev(uint8_t *data, uint32_t length)
{
	int ret;

	ret = i2s_buf_write(codec_tx, data, length);
	if (ret < 0) {
		printk("Failed to write data: %d\n", ret);
	}
}

static void codec_play_data(uint8_t *data, uint32_t length)
{
	audio_data_sync_buf[audio_data_sync_buf_w % CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT] = data;
	audio_data_sync_buf_size[audio_data_sync_buf_w % CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT] =
											length;
	audio_data_sync_buf_w++;

	if (!audio_start) {
		return;
	}

	if ((data != NULL) && (length != 0U)) {
		codec_play_to_dev(data, length);
	} else {
		codec_play_to_dev(a2dp_silence_data,
				  audio_sample_rate == 48000 ?
				  A2DP_SBC_DATA_PLAY_SIZE_48K : A2DP_SBC_DATA_PLAY_SIZE_44_1K);
	}
}

void codec_play_start(void)
{
	if (audio_start) {
		return;
	}

	audio_start = true;

	for (uint8_t i = 0; i < CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT; i++) {
		codec_play_data(a2dp_silence_data,
				audio_sample_rate == 48000 ?
				A2DP_SBC_DATA_PLAY_SIZE_48K : A2DP_SBC_DATA_PLAY_SIZE_44_1K);

		if (i == 0) {
			i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);
		}
	}
}

void codec_play_stop(void)
{
	if (!audio_start) {
		return;
	}

	audio_start = false;
	/* Don't need to stop codec_tx. After all the written buf is sent, the I2S tx is stopped. */
	/* i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_STOP); */
}

void codec_keep_play(void)
{
	uint8_t *get_data;
	uint32_t length;

	while (true) {
		if (!audio_start) {
			continue;
		}

		if (audio_sample_rate == 44100) {
			length = A2DP_SBC_DATA_PLAY_SIZE_44_1K;
		} else {
			length = A2DP_SBC_DATA_PLAY_SIZE_48K;
		}
		/* play data */
		audio_get_pcm_data(&get_data, length);
		codec_play_data(get_data, length);

		/* sync the already played media data */
		audio_media_sync(audio_data_sync_buf[audio_data_sync_buf_r %
				CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT],
				audio_data_sync_buf_size[audio_data_sync_buf_r %
				CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT]);

		audio_data_sync_buf_r++;
	}
}

#else

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	printk("Codec is unsupported\n");
}

int codec_play_init(void)
{
	return 0;
}

void codec_play_start(void)
{
}

void codec_play_stop(void)
{
}

void codec_keep_play(void)
{
}

#endif
