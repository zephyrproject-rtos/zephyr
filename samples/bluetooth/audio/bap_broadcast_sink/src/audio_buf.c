/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "audio_buf.h"

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S) && IS_ENABLED(CONFIG_AUDIO_CODEC)

/* Whole PCM ring buffer size (holds N maximum play blocks). */
#define BAP_PCM_DECODER_BUFFER_SIZE                                                                \
	(BAP_PCM_DATA_PLAY_SIZE_48K * CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT)

/* Actual usable ring buffer size for the current sample rate. */
static uint32_t pcm_buffer_size;

/* Ring buffer producer/consumer state. */
static volatile uint32_t pcm_r;
static volatile uint32_t pcm_w;
static volatile uint32_t pcm_rm;
static volatile uint32_t pcm_w_count;
static volatile uint32_t pcm_r_count;
static volatile uint32_t pcm_rm_count;

static bool reach_threshold;

static __aligned(4) uint8_t decoded_pcm_buf[BAP_PCM_DECODER_BUFFER_SIZE];

static uint32_t audio_pcm_buffer_free_size(void)
{
	return pcm_buffer_size - (pcm_w_count - pcm_rm_count);
}

static uint32_t audio_add_pcm_data(const uint8_t *data, uint32_t length)
{
	uint32_t free_space;

	free_space = audio_pcm_buffer_free_size();

	if (free_space < length) {
		length = free_space;
	}

	if ((pcm_w + length) <= pcm_buffer_size) {
		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data, length);
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0, length);
		}

		pcm_w += length;
	} else {
		uint32_t first = pcm_buffer_size - pcm_w;

		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data, first);
			memcpy(&decoded_pcm_buf[0], &data[first], length - first);
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0, first);
			memset(&decoded_pcm_buf[0], 0, length - first);
		}

		pcm_w = length - first;
	}
	pcm_w_count += length;

	if (pcm_w == pcm_buffer_size) {
		pcm_w = 0;
	}

	return length;
}

int audio_media_sync(uint8_t *data, uint16_t datalen)
{
	if (data != NULL) {
		pcm_rm += datalen;
		pcm_rm_count += datalen;

		if (pcm_rm >= pcm_buffer_size) {
			pcm_rm -= pcm_buffer_size;
		}
	}

	return 0;
}

void audio_get_pcm_data(uint8_t **data, uint32_t length)
{
	uint32_t data_space;
	uint32_t w_count = pcm_w_count;
	uint32_t r_count = pcm_r_count;

	data_space = w_count - r_count;

	if (!reach_threshold) {
		if (data_space > CONFIG_BAP_BROADCAST_SINK_BOARD_CODEC_PLAY_THRESHOLD *
				 pcm_buffer_size / 100U) {
			reach_threshold = true;
		} else {
			*data = NULL;
			return;
		}
	}

	if (data_space < length) {
		*data = NULL;
		reach_threshold = false;
	} else {
		pcm_r_count += length;
		*data = &decoded_pcm_buf[pcm_r];
		pcm_r += length;

		if (pcm_r >= pcm_buffer_size) {
			pcm_r = 0;
		}
	}
}

void audio_feed_pcm_lc3(const uint8_t *data, size_t len, uint8_t channels)
{
	if ((data == NULL) || (len == 0U) || (channels == 0U)) {
		return;
	}

	/* The playback path always operates on stereo interleaved samples.
	 * If the decoded LC3 frame is mono, duplicate each sample to both
	 * channels; if it is already stereo (or more), pass through and let
	 * the sink truncate any additional channels.
	 */
	if (channels == 1U) {
		const int16_t *src = (const int16_t *)data;
		const size_t samples = len / sizeof(int16_t);
		int16_t frame[2];

		for (size_t i = 0U; i < samples; i++) {
			frame[0] = src[i];
			frame[1] = src[i];
			(void)audio_add_pcm_data((const uint8_t *)frame, sizeof(frame));
		}
	} else {
		(void)audio_add_pcm_data(data, len);
	}
}

void audio_buf_reset(uint32_t fs)
{
	uint32_t block;

	pcm_r = 0;
	pcm_w = 0;
	pcm_rm = 0;
	pcm_w_count = 0;
	pcm_r_count = 0;
	pcm_rm_count = 0;
	reach_threshold = false;

	switch (fs) {
	case 8000:
		block = BAP_PCM_DATA_PLAY_SIZE_8K;
		break;
	case 16000:
		block = BAP_PCM_DATA_PLAY_SIZE_16K;
		break;
	case 24000:
		block = BAP_PCM_DATA_PLAY_SIZE_24K;
		break;
	case 32000:
		block = BAP_PCM_DATA_PLAY_SIZE_32K;
		break;
	case 48000:
		block = BAP_PCM_DATA_PLAY_SIZE_48K;
		break;
	default:
		printk("wrong frequency\n");
		return;
	}

	pcm_buffer_size = BAP_PCM_DECODER_BUFFER_SIZE - (BAP_PCM_DECODER_BUFFER_SIZE % block);
}

#else

void audio_buf_reset(uint32_t fs)
{
	ARG_UNUSED(fs);
}

void audio_feed_pcm_lc3(const uint8_t *data, size_t len, uint8_t channels)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(channels);
}

#endif
