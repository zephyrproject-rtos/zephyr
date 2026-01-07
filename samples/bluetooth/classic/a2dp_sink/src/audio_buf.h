/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_BUF_H_
#define ZEPHYR_INCLUDE_AUDIO_BUF_H_

/* The played audio data is 10ms data size */
#define A2DP_SBC_DATA_PLAY_SIZE_44_1K (441 * 2 * 2)
#define A2DP_SBC_DATA_PLAY_SIZE_48K (480 * 2 * 2)

void audio_buf_reset(uint32_t fs);

void audio_process_sbc_buf(uint8_t sbc_hdr, uint8_t *data, size_t len, uint16_t seq_num,
			   uint32_t ts, uint8_t channel_num);

int audio_media_sync(uint8_t *data, uint16_t datalen);

void audio_get_pcm_data(uint8_t **data, uint32_t length);
#endif
