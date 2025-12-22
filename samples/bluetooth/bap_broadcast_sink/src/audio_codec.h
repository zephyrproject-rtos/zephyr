/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample audio codec header
 *
 * This file declares the audio codec related functionality for the sample
 *
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_BAP_BROADCAST_SINK_AUDIO_CODEC_H
#define SAMPLE_BAP_BROADCAST_SINK_AUDIO_CODEC_H
#include <stddef.h>
#include <stdint.h>

int audio_codec_open(void);
int audio_codec_cfg(uint32_t samplerate);
uint32_t audio_codec_write_data(const uint8_t *data, uint32_t len);
int audio_codec_close(void);

#endif /* SAMPLE_BAP_BROADCAST_SINK_AUDIO_CODEC_H */
