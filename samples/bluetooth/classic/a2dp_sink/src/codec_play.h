/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CODEC_PLAY_H_
#define ZEPHYR_INCLUDE_CODEC_PLAY_H_

int codec_play_init(void);

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels);

void codec_play_start(void);

void codec_play_stop(void);

void codec_keep_play(void);

#endif
