/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink audio codec integration
 *
 * This file handles all the audio codec hardware integration for the sample.
 *
 * Two codec integration models are supported and selected at compile time:
 *
 *  - Path A (control-only codec): the codec driver only implements the codec
 *    control API (for example the WM8962 on NXP i.MX RT boards). Such a codec
 *    cannot pull or write PCM samples itself, so the PCM data is transported to
 *    the codec over the SoC SAI/I2S peripheral by hw_codec_i2s.c. This module
 *    owns the codec control (audio_codec_configure / start_output / stop_output
 *    / volume) while hw_codec_i2s.c owns the SAI/I2S data path.
 *    This path is selected when the board provides an "i2s-codec-tx"
 *    devicetree alias and both CONFIG_I2S and CONFIG_AUDIO_CODEC are enabled.
 *
 *  - Path B (smart codec): the codec driver implements a done callback and an
 *    audio_codec_write() entry point, so PCM samples are pulled from an
 *    internal ring buffer whenever the codec requests a new block. This is the
 *    default path used when no I2S transport alias is present.
 *
 * Both models are hidden behind the same hw_codec_open / hw_codec_cfg /
 * hw_codec_write_data / hw_codec_close facade, so the mpipe and non-mpipe
 * callers do not need to know which model is in use.
 *
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(codec, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Select Path A (control-only codec + SAI/I2S PCM transport) when the user
 * enables CONFIG_USE_I2S_CODEC_TRANSPORT. Otherwise fall back to Path B
 * (smart codec with its own host interface).
 */

/*
 * Default speaker/output volume sent via AUDIO_PROPERTY_OUTPUT_VOLUME.
 *
 * The valid numeric range and exact mapping to dB are codec-dependent; refer
 * to the specific codec driver / datasheet for details. The value 15 was
 * selected as a conservative, comfortable default for this sample application
 * to avoid an excessively loud/noisy output during testing.
 * Adjust as needed to match the desired default output level.
 */
#define SPEAKER_VOL 15U



#if CONFIG_USE_I2S_CODEC_TRANSPORT
/* ---------------------------------------------------------------------------
 * Path A: control-only codec (for example WM8962) with SAI/I2S PCM transport.
 * -------------------------------------------------------------------------
 */


#include "hw_codec_i2s.h"

static const struct device *g_dev;
static bool codec_configured;

int hw_codec_open(void)
{
	int ret;

	/*
	 * Idempotent: the codec is opened from the stream "started" callback,
	 * and the audio data path may also open it while bringing itself up.
	 * Opening twice is harmless but re-initialising the transport under a
	 * running stream is not, so return early once the device is held.
	 */
	if (g_dev != NULL) {
		return 0;
	}

	g_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
	if (!device_is_ready(g_dev)) {
		LOG_ERR("Could not open codec device");
		g_dev = NULL;
		return -EIO;
	}

	ret = hw_codec_i2s_init();
	if (ret != 0) {
		LOG_ERR("Failed to init I2S transport (err %d)", ret);
		g_dev = NULL;
		return ret;
	}

	return 0;
}

int hw_codec_cfg(uint32_t samplerate, uint32_t frame_dur_us, uint8_t channels)
{
	int ret;
	audio_property_value_t val = {.vol = SPEAKER_VOL};
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
		.dai_cfg.i2s.word_size = CODEC_PCM_WIDTH_BITS,
		.dai_cfg.i2s.channels = channels,
		.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S,
#ifdef CONFIG_USE_CODEC_CLOCK
		.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
#else
		.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
#endif
		.dai_cfg.i2s.frame_clk_freq = samplerate,
		/*
		 * mem_slab and block_size are intentionally left unset here:
		 * they only matter to the SoC I2S peripheral, which is
		 * configured separately in hw_codec_i2s_cfg(). The codec control
		 * driver (for example WM8962) ignores these two fields. This is
		 * unlike a2dp_sink, which reuses a single struct for both the
		 * audio_codec_configure() and the i2s_configure() calls and thus
		 * has to populate them.
		 */
	};

	if (codec_configured) {
		LOG_WRN("Already configured");
		return -EALREADY;
	}

	LOG_INF("codec (I2S): samplerate=%u channels=%u frame_dur=%u us", samplerate,
		channels, frame_dur_us);

	ret = audio_codec_configure(g_dev, &cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure codec (err %d)", ret);
		return ret;
	}

	k_msleep(1000);

	ret = hw_codec_i2s_cfg(samplerate, channels, frame_dur_us);
	if (ret != 0) {
		LOG_ERR("Failed to configure I2S transport (err %d)", ret);
		return ret;
	}

	audio_codec_start_output(g_dev);

	ret = audio_codec_set_property(g_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, val);
	if (ret != 0) {
		LOG_ERR("Failed to set codec output volume (err %d)", ret);
		audio_codec_stop_output(g_dev);
		return ret;
	}

	codec_configured = true;

	return 0;
}

uint32_t hw_codec_write_data(const uint8_t *data, uint32_t len)
{
	return hw_codec_i2s_write(data, len);
}

int hw_codec_close(void)
{
	if (g_dev != NULL && codec_configured) {
		(void)hw_codec_i2s_close();
		audio_codec_stop_output(g_dev);
		codec_configured = false;
		return 0;
	} else {
		return -EIO;
	}
}

#else /* !CONFIG_USE_I2S_CODEC_TRANSPORT */
/* ---------------------------------------------------------------------------
 * Path B: smart codec (pull-driven via done callback + audio_codec_write).
 * -------------------------------------------------------------------------
 */


/*
 * Size of one audio block transferred to the codec, in bytes.
 *
 * For the configuration used here (16bit samples, mono channel),
 * 480 bytes correspond to 240 PCM samples per block. This value was
 * chosen as a compromise between:
 *  - keeping latency reasonably low (smaller blocks)
 *  - avoiding excessive interrupt / DMA overhead (larger blocks).
 * If the PCM format (sample width or number of channels) is changed,
 * this constant should be revisited so that the block size remains
 * appropriate for the target use case and hardware.
 */
#define CODEC_BLOCK_SIZE 480U

/*
 * Total size of the ring buffer used to queue audio data for the codec.
 *
 * The buffer is sized to hold 20 CODEC_BLOCK_SIZE blocks. Having multiple
 * blocks queued reduces the likelihood of underruns when the producer is
 * briefly delayed, while still keeping overall latency and RAM usage
 * bounded. Increasing the multiplier adds buffering (and latency / RAM
 * consumption); decreasing it reduces buffering but makes underruns more
 * likely on a busy system.
 */
#define RING_BUF_SIZE (CODEC_BLOCK_SIZE * 20U)

static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf rb;
static bool codec_configured;
static uint8_t block_data[CODEC_BLOCK_SIZE];
static const struct device *g_dev;

static void tx_done(const struct device *dev, void *user_data)
{
	uint32_t avail;
	uint32_t read;
	int written;

	ARG_UNUSED(user_data);

	avail = ring_buf_size_get(&rb);
	if (avail < CODEC_BLOCK_SIZE) {
		LOG_WRN("Buffer does not have enough data for a full block");
		return;
	}

	read = ring_buf_get(&rb, block_data, CODEC_BLOCK_SIZE);
	if (read != CODEC_BLOCK_SIZE) {
		LOG_WRN("Failed to read full block from ring buffer: %u bytes", read);
		return;
	}

	written = audio_codec_write(dev, block_data, CODEC_BLOCK_SIZE);
	if (written != CODEC_BLOCK_SIZE) {
		LOG_WRN("Failed to write full block data to audio device: %u bytes", written);
		return;
	}
}

int hw_codec_open(void)
{
	/*
	 * Idempotent: the codec is opened from the stream "started" callback,
	 * and the audio data path may also open it while bringing itself up.
	 * Returning early keeps a second open from resetting the ring buffer
	 * under a codec that is already streaming.
	 */
	if (g_dev != NULL) {
		return 0;
	}

	g_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
	if (!device_is_ready(g_dev)) {
		LOG_ERR("Could not open codec device");
		g_dev = NULL;
		return -EIO;
	}
	ring_buf_init(&rb, RING_BUF_SIZE, ring_buffer);
	return 0;
}

int hw_codec_cfg(uint32_t samplerate, uint32_t frame_dur_us, uint8_t channels)
{
	int ret;
	audio_property_value_t val = {.vol = SPEAKER_VOL};
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_PCM,
		.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TX,
		.dai_cfg.pcm.pcm_width = AUDIO_PCM_WIDTH_16_BITS,
		.dai_cfg.pcm.channels = 1U,
		.dai_cfg.pcm.block_size = CODEC_BLOCK_SIZE,
		.dai_cfg.pcm.samplerate = samplerate,
	};

	/*
	 * The smart-codec path pulls fixed-size blocks (CODEC_BLOCK_SIZE) from
	 * its own ring buffer, so the codec frame duration does not affect its
	 * configuration.
	 */
	ARG_UNUSED(frame_dur_us);
	ARG_UNUSED(channels);

	if (codec_configured) {
		LOG_WRN("Already configured");
		return -EALREADY;
	}

	LOG_INF("codec: samplerate=%d block_size=%d", samplerate, CODEC_BLOCK_SIZE);
	audio_codec_register_done_callback(g_dev, tx_done, NULL, NULL, NULL);
	ret = audio_codec_configure(g_dev, &cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure codec (err %d)", ret);
		return ret;
	}

	ret = audio_codec_start(g_dev, AUDIO_DAI_DIR_TX);
	if (ret != 0) {
		LOG_ERR("Failed to start codec (err %d)", ret);
		return ret;
	}

	ret = audio_codec_set_property(g_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, val);
	if (ret != 0) {
		LOG_ERR("Failed to set codec output volume (err %d)", ret);
		if (audio_codec_stop(g_dev, AUDIO_DAI_DIR_TX) < 0) {
			LOG_ERR("Failed to stop codec");
		}
		return ret;
	}
	codec_configured = true;

	return 0;
}

uint32_t hw_codec_write_data(const uint8_t *data, uint32_t len)
{
	uint32_t bytes_put;

	bytes_put = ring_buf_put(&rb, data, len);
	if (bytes_put != len) {
		LOG_WRN("Buffer full, only added %u bytes", bytes_put);
	}
	return bytes_put;
}

int hw_codec_close(void)
{
	if (g_dev != NULL && codec_configured) {
		int ret = audio_codec_stop(g_dev, AUDIO_DAI_DIR_TX);

		if (ret < 0) {
			return ret;
		}
		ring_buf_reset(&rb);
		codec_configured = false;
		return 0;
	} else {
		return -EIO;
	}
}

#endif /* CONFIG_USE_I2S_CODEC_TRANSPORT */
