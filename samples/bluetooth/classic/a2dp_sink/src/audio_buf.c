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
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/sbc.h>
#include "audio_buf.h"

#if DT_HAS_ALIAS(i2s_codec_tx) && IS_ENABLED(CONFIG_I2S) && IS_ENABLED(CONFIG_AUDIO_CODEC)

/* The whole PCM buffer (save the audio data after decoding) size */
#define A2DP_SBC_DECODER_PCM_BUFFER_SIZE (A2DP_SBC_DATA_PLAY_SIZE_48K *\
					  CONFIG_A2DP_SBC_PCM_BUFFER_PLAY_COUNT)
uint32_t pcm_buffer_size; /* set it dynamically based on frequency */
/* The PCM data size for one sbc frame
 * (channel num: 2, sample size: 2, max subband: 8, max block length: 16)
 */
#define A2DP_SBC_ONE_FRAME_MAX_SIZE (2 * 2 * 16 * 8)
/* audio stream control variables */
static volatile uint32_t pcm_r;
static volatile uint32_t pcm_w;
static volatile uint32_t pcm_rm;
static volatile uint32_t pcm_w_count;
static volatile uint32_t pcm_r_count;
static volatile uint32_t pcm_rm_count;
static struct sbc_decoder decoder;
static volatile bool sbc_first_data;
static uint32_t sbc_expected_ts;
static uint16_t pcm_frame_buffer[A2DP_SBC_ONE_FRAME_MAX_SIZE / 2];
static __aligned(4) uint8_t decoded_pcm_buf[A2DP_SBC_DECODER_PCM_BUFFER_SIZE];

static uint32_t audio_pcm_buffer_free_size(void)
{
	uint32_t w_count = pcm_w_count;
	uint32_t rm_count = pcm_rm_count;

	if (w_count >= rm_count) {
		return pcm_buffer_size - (w_count - rm_count);
	} else {
		return pcm_buffer_size - ((uint32_t)(((uint64_t)w_count +
			(uint64_t)0x100000000u) - (uint64_t)rm_count));
	}
}

static uint32_t audio_add_pcm_data(uint8_t *data, uint32_t length)
{
	uint32_t free_space;

	free_space = audio_pcm_buffer_free_size();

	if (free_space < length) {
		length = free_space;
	}

	/* copy data to buffer */
	if ((pcm_w + length) <= pcm_buffer_size) {
		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data, length);
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0, length);
		}

		pcm_w += length;
	} else {
		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data, (pcm_buffer_size - pcm_w));
			memcpy(&decoded_pcm_buf[0], &data[(pcm_buffer_size - pcm_w)],
			       length - (pcm_buffer_size - pcm_w));
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0, (pcm_buffer_size - pcm_w));
			memset(&decoded_pcm_buf[0], 0, length - (pcm_buffer_size - pcm_w));
		}

		pcm_w = length - (pcm_buffer_size - pcm_w);
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
	static bool reach_threshold;
	uint32_t data_space;
	uint32_t w_count = pcm_w_count;
	uint32_t r_count = pcm_r_count;

	if (w_count >= r_count) {
		data_space = w_count - r_count;
	} else {
		data_space = (uint32_t)(((uint64_t)w_count +
			(uint64_t)0x100000000u) - (uint64_t)r_count);
	}

	if (!reach_threshold) {
		if (data_space > CONFIG_A2DP_BOARD_CODEC_PLAY_THRESHOLD * pcm_buffer_size / 100) {
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

void audio_process_sbc_buf(uint8_t sbc_hdr, uint8_t *data, size_t len, uint16_t seq_num,
			   uint32_t ts, uint8_t channel_num)
{
	const void *in_data;
	size_t samples_count;
	size_t out_size;
	size_t samples_lost;
	uint8_t num_frames;
	int err;

	samples_lost = 0;

	if (!sbc_first_data) {
		sbc_first_data = true;
		sbc_expected_ts = ts;
	} else {
		if (sbc_expected_ts != ts) {
			if (sbc_expected_ts < ts) {
				samples_lost = ts - sbc_expected_ts;
			}
		}
	}

	if (samples_lost != 0) {
		(void)audio_add_pcm_data(NULL, samples_lost * 2U * channel_num);
		sbc_expected_ts = sbc_expected_ts + samples_lost;
	}

	num_frames = BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_GET(sbc_hdr);

	samples_count = 0;
	in_data = (void *)data;
	for (uint8_t i = 0; i < num_frames; ++i) {
		if (audio_pcm_buffer_free_size() == 0) {
			/* if no enough space, don't need decode. */
			continue;
		}

		out_size = sizeof(pcm_frame_buffer);
		err = sbc_decode(&decoder, &in_data, &len, pcm_frame_buffer, &out_size);

		if (err == 0) {
			audio_add_pcm_data((uint8_t *)pcm_frame_buffer, out_size);
		} else {
			printk("decode err\n");
			break;
		}

		samples_count += (out_size / 2 / channel_num);
	}

	sbc_expected_ts = ts + samples_count;
}

void audio_buf_reset(uint32_t fs)
{
	pcm_r = 0;
	pcm_w = 0;
	pcm_rm = 0;
	pcm_w_count = 0;
	pcm_r_count = 0;
	pcm_rm_count = 0;

	sbc_setup_decoder(&decoder);

	if (fs == 48000) {
		pcm_buffer_size = A2DP_SBC_DECODER_PCM_BUFFER_SIZE;
	} else if (fs == 44100) {
		pcm_buffer_size = A2DP_SBC_DECODER_PCM_BUFFER_SIZE -
				  A2DP_SBC_DECODER_PCM_BUFFER_SIZE % A2DP_SBC_DATA_PLAY_SIZE_44_1K;
	} else {
		printk("wrong frequency\n");
	}

	sbc_first_data = false;
}

#else

void audio_buf_reset(uint32_t fs)
{
}

void audio_process_sbc_buf(uint8_t sbc_hdr, uint8_t *data, size_t len, uint16_t seq_num,
			   uint32_t ts, uint8_t channel_num)
{
}

#endif
