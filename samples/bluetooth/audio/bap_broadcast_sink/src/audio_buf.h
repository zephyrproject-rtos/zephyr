/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_BUF_H_
#define ZEPHYR_INCLUDE_AUDIO_BUF_H_

#include <stddef.h>
#include <stdint.h>

/* Played PCM block is 10 ms of stereo 16-bit samples. */
#define BAP_PCM_DATA_PLAY_SIZE(_fs) (((_fs) / 100U) * 2U * 2U)

#define BAP_PCM_DATA_PLAY_SIZE_8K  BAP_PCM_DATA_PLAY_SIZE(8000)
#define BAP_PCM_DATA_PLAY_SIZE_16K BAP_PCM_DATA_PLAY_SIZE(16000)
#define BAP_PCM_DATA_PLAY_SIZE_24K BAP_PCM_DATA_PLAY_SIZE(24000)
#define BAP_PCM_DATA_PLAY_SIZE_32K BAP_PCM_DATA_PLAY_SIZE(32000)
#define BAP_PCM_DATA_PLAY_SIZE_48K BAP_PCM_DATA_PLAY_SIZE(48000)

void audio_buf_reset(uint32_t fs);

/* Feed decoded LC3 PCM into the playback ring buffer. */
void audio_feed_pcm_lc3(const uint8_t *data, size_t len, uint8_t channels);

int audio_media_sync(uint8_t *data, uint16_t datalen);

void audio_get_pcm_data(uint8_t **data, uint32_t length);

#endif /* ZEPHYR_INCLUDE_AUDIO_BUF_H_ */
