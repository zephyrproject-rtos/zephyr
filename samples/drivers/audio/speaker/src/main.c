/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/util.h>

#include "audio_content.h"

#define LOG_LEVEL CONFIG_APP_LOG_LEVEL

LOG_MODULE_REGISTER(i2s_sample, LOG_LEVEL);

#define RETURN_ON_ERROR(func)                                                                      \
	do {                                                                                       \
		const int err = func;                                                              \
		if ((err) < 0) {                                                                   \
			while (log_process()) {                                                    \
			};                                                                         \
			return err;                                                                \
		}                                                                                  \
	} while (0)

#define AUDIO_FRAME_BUF_BYTES                                                                      \
	(2 /* channels */ * 2 /* bytes per sample */ * CONFIG_APP_AUDIO_SAMPLES)

/* NOTE With NXP RT SOCs the slab definition needs the `__nocache` keyword to be dma capable.
 * The usual definition with `K_MEM_SLAB_DEFINE_STATIC` does not work.
 * https://support.nxp.com/s/case/5002p00002xVeNzAAK/garbage-i2s-data-when-using-the-zephyr-sai-driver-in-tx-direction
 */
char __nocache __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(CONFIG_APP_I2S_PLAY_BUF_COUNT)*WB_UP(AUDIO_FRAME_BUF_BYTES)];

STRUCT_SECTION_ITERABLE(k_mem_slab,
			i2s_mem_slab) = Z_MEM_SLAB_INITIALIZER(i2s_mem_slab,
							       _k_mem_slab_buf_tx_0_mem_slab,
							       WB_UP(AUDIO_FRAME_BUF_BYTES),
							       CONFIG_APP_I2S_PLAY_BUF_COUNT);

static const struct device *spk_i2s_dev;
static const struct device *codec_device;
static const audio_property_value_t output_volume = {.vol = -10};

static int i2s_audio_init(void)
{
	int ret;
	struct i2s_config i2s_cfg;
	struct audio_codec_cfg codec_cfg;

	spk_i2s_dev = DEVICE_DT_GET(DT_ALIAS(speaker_i2s));
	codec_device = DEVICE_DT_GET(DT_ALIAS(audio0));

	if (!device_is_ready(spk_i2s_dev)) {
		LOG_ERR("I2S device %s not found", spk_i2s_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(codec_device)) {
		LOG_ERR("Codec device %s not found", codec_device->name);
		return -ENODEV;
	}

	/* configure i2s for audio playback */
	i2s_cfg.word_size = CONFIG_APP_AUDIO_SAMPLE_BIT_WIDTH;
	i2s_cfg.channels = CONFIG_APP_AUDIO_NUM_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_CLK_NF_NB;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = CONFIG_APP_AUDIO_SAMPLE_FREQ;
	i2s_cfg.block_size = AUDIO_FRAME_BUF_BYTES;
	i2s_cfg.mem_slab = &i2s_mem_slab;

	i2s_cfg.timeout = 2000;
	ret = i2s_configure(spk_i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure failed with %d error", ret);
		return -EIO;
	}

	/* configure codec */
	codec_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	codec_cfg.dai_cfg.i2s = i2s_cfg;

	RETURN_ON_ERROR(audio_codec_configure(codec_device, &codec_cfg));
	RETURN_ON_ERROR(audio_codec_set_property(codec_device, AUDIO_PROPERTY_OUTPUT_VOLUME,
						 AUDIO_CHANNEL_ALL, output_volume));

	return 0;
}

static int i2s_start_audio(void)
{
	int ret = 0;

	LOG_DBG("Starting audio playback...");
	/* start codec output */
	audio_codec_start_output(codec_device);

	/* start i2s */
	ret = i2s_trigger(spk_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret) {
		LOG_ERR("spk_i2s_dev TX start failed. code %d", ret);
		return ret;
	}

	return 0;
}

static int i2s_prepare_audio()
{
	int ret = 0;
	uint8_t zeros[AUDIO_FRAME_BUF_BYTES] = {0};

	LOG_DBG("Preloading silence...");
	for (int frame_counter = 0; frame_counter < CONFIG_APP_I2S_TX_PRELOAD_BUF_COUNT;
	     ++frame_counter) {
		ret = i2s_buf_write(spk_i2s_dev, zeros, ARRAY_SIZE(zeros));
		if (ret) {
			LOG_ERR("i2s_write failed %d", ret);
			return ret;
		}
	}

	return 0;
}

static int i2s_play_audio(void)
{
	const uint8_t *const in_buf = audio_test_content;
	const size_t buflen = ARRAY_SIZE(audio_test_content);
	size_t bytes_to_transmit = 0;
	size_t bytes_transmitted = 0;
	int ret = 0;

	LOG_INF("Stream with %d bytes started", buflen);

	while (bytes_transmitted < buflen) {
		bytes_to_transmit = MIN(buflen - bytes_transmitted, AUDIO_FRAME_BUF_BYTES);

		LOG_DBG("Bytes to transmit: %d, total transmitted bytes: %d", bytes_to_transmit,
			bytes_transmitted);

		LOG_DBG("Remaining bytes=%d current byte=0x%02X", buflen - bytes_transmitted,
			in_buf[bytes_transmitted]);

		ret = i2s_buf_write(spk_i2s_dev, (void *)&in_buf[bytes_transmitted],
				    bytes_to_transmit);
		if (ret < 0) {
			LOG_ERR("i2s_write failed %d", ret);
			return ret;
		}

		bytes_transmitted += bytes_to_transmit;
	}

	LOG_DBG("Reached end of playback after %d bytes and current data pointer at %p",
		bytes_transmitted, (void *)&in_buf[bytes_transmitted]);

	if (bytes_transmitted < buflen) {
		LOG_ERR("Playback incomplete: %d", bytes_transmitted);
		return -EIO;
	}

	return ret;
}

static int i2s_stop_audio(void)
{
	int ret;

	ret = i2s_trigger(spk_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("spk_i2s_dev stop failed with code %d", ret);
		return ret;
	}

	LOG_DBG("Stopping audio playback...");

	return 0;
}

int main(void)
{
	LOG_INF("Starting I2S audio sample app ...");
	RETURN_ON_ERROR(i2s_audio_init());
	RETURN_ON_ERROR(i2s_prepare_audio());
	RETURN_ON_ERROR(i2s_start_audio());
	RETURN_ON_ERROR(i2s_play_audio());
	RETURN_ON_ERROR(i2s_stop_audio());
	LOG_INF("Exiting I2S audio sample app ...");

	return 0;
}
