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

	int ret;

	audio_sample_rate = sample_rate;
	if (sample_rate == 44100) {
		/*
		 * A2DP_SBC_DATA_PLAY_SIZE_44_1K (1764 bytes) is not a multiple of
		 * sizeof(void *). k_mem_slab_init()/create_free_list() require the
		 * block size to be word aligned (they build an in-place free list by
		 * linking each block's first pointer-sized word). On 32-bit targets
		 * 1764 happens to be 4-byte aligned so this went unnoticed, but on
		 * this AArch64 board (8-byte pointers) it fails the alignment check,
		 * k_mem_slab_init() returns -EINVAL, and the mem_slab is left with
		 * an uninitialized wait_q/free_list. The buffer itself is already
		 * sized using the larger A2DP_SBC_DATA_PLAY_SIZE_48K, so rounding
		 * the block size up here is safe (the actual data written per
		 * block, A2DP_SBC_DATA_PLAY_SIZE_44_1K, still fits within it).
		 */
		block_size = ROUND_UP(A2DP_SBC_DATA_PLAY_SIZE_44_1K, sizeof(void *));
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

	/*
	 * Configure the SAI (MCLK master) BEFORE the codec. On boards where
	 * MCLK is derived from a runtime-tunable CCM root clock (i.MX93 SAI3),
	 * i2s_configure() may need to retune the SAI root clock to a
	 * sample-rate-aligned frequency (e.g. 11.2896 MHz for the 44.1 kHz
	 * family vs. 12.288 MHz for the 48 kHz family). If the codec is
	 * configured first, it reads a stale MCLK rate via
	 * clock_control_get_rate() and programs its internal FLL for the
	 * wrong input frequency, producing an out-of-spec SYSCLK once the
	 * SAI subsequently retunes MCLK.
	 *
	 * Reordering is safe on boards whose MCLK is DT-static (e.g. NXP RT
	 * anatop pll-clocks): configuring the SAI first has no effect on the
	 * codec beyond a slightly earlier MCLK enable, which the WM8962
	 * tolerates.
	 */
	if (i2s_configure(codec_tx, I2S_DIR_TX, &config)) {
		printk("failure to config streams\n");
	}

	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);

	ret = k_mem_slab_init(&mem_slab, mem_slab_buffer, block_size,
			      CONFIG_A2DP_BOARD_CODEC_PLAY_COUNT);
	if (ret < 0) {
		printk("failed to init mem slab: %d\n", ret);
	}
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
			k_sleep(K_MSEC(1));
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
