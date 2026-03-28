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
#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S)
#include <zephyr/drivers/i2s.h>
#endif
#include <zephyr/audio/codec.h>
#include "audio_buf.h"
#include "codec_play.h"

#if IS_ENABLED(CONFIG_AUDIO_CODEC)

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
static uint32_t audio_sample_channel_num;
static uint8_t *audio_data_sync_buf[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT];
static uint32_t audio_data_sync_buf_size[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT];
static uint8_t audio_data_sync_buf_w;
static uint8_t audio_data_sync_buf_r;
static __NOCACHE __aligned(4) uint8_t a2dp_silence_data[A2DP_SBC_DATA_PLAY_SIZE_48K];

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S)
#define I2S_CODEC_TX DT_ALIAS(i2s_codec_tx)
#define I2S_TIMEOUT (2000U)
const struct device *const codec_tx = DEVICE_DT_GET(I2S_CODEC_TX);
static __NOCACHE __aligned(4) uint8_t mem_slab_buffer[CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT *
						      A2DP_SBC_DATA_PLAY_SIZE_48K];
static struct k_mem_slab mem_slab;
#elif DT_HAS_ALIAS(codec0)
const struct device *const codec_tx = DEVICE_DT_GET(DT_ALIAS(codec0));
static struct k_sem tx_done_sem;

static void tx_done(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	k_sem_give(&tx_done_sem);
}
#else
#error	"Unsupported codec device, please check your board overlay and Kconfig"
#endif

int codec_play_init(void)
{
	const struct device *codec_dev;

#if DT_HAS_ALIAS(i2s_codec_tx)
	codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	if (!device_is_ready(codec_tx)) {
		printk("%s is not ready\n", codec_tx->name);
		return -EIO;
	}
#elif DT_HAS_ALIAS(codec0)
	codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));

	k_sem_init(&tx_done_sem, 0, 1);
#endif
	if (!device_is_ready(codec_dev)) {
		printk("%s is not ready\n", codec_dev->name);
		return -EIO;
	}

	return 0;
}

static void codec_play_configure_audio(uint32_t sample_rate, uint8_t sample_width,
				       uint8_t channels, size_t block_size)
{
	struct audio_codec_cfg audio_cfg;

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S)
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

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
#elif DT_HAS_ALIAS(codec0)
	const struct device *const codec_dev = codec_tx;

	audio_codec_register_done_callback(codec_dev, tx_done, NULL, NULL, NULL);
	audio_cfg.dai_type = AUDIO_DAI_TYPE_PCM;
	audio_cfg.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TX;
	audio_cfg.dai_cfg.pcm.pcm_width = sample_width;
	/* Convert to mono channel for codec without enough channel support. */
	audio_cfg.dai_cfg.pcm.channels = 1U;
	block_size /= channels;
	audio_cfg.dai_cfg.pcm.block_size = block_size;
	audio_cfg.dai_cfg.pcm.samplerate = sample_rate;
#endif
	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);
}

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	size_t block_size = (sample_rate == 44100) ?
		A2DP_SBC_DATA_PLAY_SIZE_44_1K : A2DP_SBC_DATA_PLAY_SIZE_48K;

	audio_sample_rate = sample_rate;
	audio_sample_channel_num = channels;
	codec_play_configure_audio(sample_rate, sample_width, channels, block_size);

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S)
	struct i2s_config config;

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
#endif
}

static void codec_play_to_dev(uint8_t *data, uint32_t length)
{
	int err;

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S)
	err = i2s_buf_write(codec_tx, data, length);
	if (err < 0) {
		printk("Failed to write data: %d\n", err);
	}

#elif DT_HAS_ALIAS(codec0)
	/*Convert to mono channel and play*/
	if (audio_sample_channel_num == 2) {
		static int16_t mono_buf[A2DP_SBC_DATA_PLAY_SIZE_48K/2];

		for (uint32_t i = 0; i < length / 2; i++) {
			int16_t sample = ((int16_t *)data)[i * 2] / 2 +
				 ((int16_t *)data)[i * 2 + 1] / 2;

			mono_buf[i] = sample;
		}
		data = (uint8_t *)mono_buf;
		length /= 2;
	}
	err = audio_codec_write(codec_tx, data, length);
	if (err < 0) {
		printk("Failed to write data: %d\n", err);
	}
	k_sem_take(&tx_done_sem, K_FOREVER);
#endif
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

#if DT_HAS_ALIAS(codec0)
	int err;

	err = audio_codec_start(codec_tx, AUDIO_DAI_DIR_TX);
	if (err != 0) {
		printk("Failed to start CODEC (err %d)", err);
		return;
	}
#else
	for (uint8_t i = 0; i < CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT; i++) {
		codec_play_to_dev(a2dp_silence_data,
				  audio_sample_rate == 48000 ?
				  A2DP_SBC_DATA_PLAY_SIZE_48K : A2DP_SBC_DATA_PLAY_SIZE_44_1K);

		if (i == 0) {
			i2s_trigger(codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);
		}
	}
#endif

	audio_start = true;
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
