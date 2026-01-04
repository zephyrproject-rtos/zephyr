/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink audio codec integration
 *
 * This file handles all the audio codec hardware integration for the sample
 *
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/audio/codec.h>

LOG_MODULE_REGISTER(codec, CONFIG_LOG_DEFAULT_LEVEL);

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
 * Default speaker/output volume sent via AUDIO_PROPERTY_OUTPUT_VOLUME.
 *
 * The valid numeric range and exact mapping to dB are codec‑dependent;
 * refer to the specific codec driver / datasheet for details. The value
 * 15 was selected as a conservative, comfortable default for this sample
 * application. Adjust as needed to match the desired default output level.
 */
#define SPEAKER_VOL 15

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

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(codec0))
static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf rb;
static bool codec_configed;
static uint8_t block_data[CODEC_BLOCK_SIZE];
static const struct device *g_dev;

static void tx_done(const struct device *dev, void *user_data)
{
	uint32_t avail;
	uint32_t read;
	uint32_t written;

	avail = ring_buf_size_get(&rb);
	if (avail < CODEC_BLOCK_SIZE) {
		LOG_WRN("Buffer is not enough for a full block");
		return;
	}
	read = ring_buf_get(&rb, block_data, CODEC_BLOCK_SIZE);
	if (read != CODEC_BLOCK_SIZE) {
		LOG_WRN("Failed to read full block from ring buffer: %u bytes", read);
		return;
	}
	written = audio_codec_write(dev, block_data, CODEC_BLOCK_SIZE);
	if (written != CODEC_BLOCK_SIZE) {
		LOG_WRN("Failed to write full block data to audio device: %u bytes", read);
		return;
	}
}
int audio_codec_open(void)
{
	g_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
	if (!device_is_ready(g_dev)) {
		LOG_ERR("Could not open codec device");
		return -EIO;
	}
	ring_buf_init(&rb, RING_BUF_SIZE, ring_buffer);
	return 0;
}
int audio_codec_cfg(uint32_t samplerate)
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

	if (codec_configed) {
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
		audio_codec_stop(g_dev, AUDIO_DAI_DIR_TX);
		return ret;
	}
	codec_configed = true;

	return 0;
}
uint32_t audio_codec_write_data(const uint8_t *data, uint32_t len)
{
	uint32_t bytes_put;

	bytes_put = ring_buf_put(&rb, data, len);
	if (bytes_put != len) {
		LOG_WRN("Buffer full");
	}
	return bytes_put;
}
int audio_codec_close(void)
{
	if (g_dev != NULL && codec_configed) {
		audio_codec_stop(g_dev, AUDIO_DAI_DIR_TX);
		codec_configed = false;
		return 0;
	} else {
		return -EALREADY;
	}
}
#else
int audio_codec_open(void)
{
	return 0;
}
int audio_codec_cfg(uint32_t samplerate)
{
	return 0;
}
uint32_t audio_codec_write_data(const uint8_t *data, uint32_t len)
{
	return len;
}
int audio_codec_close(void)
{
	return 0;
}
#endif
