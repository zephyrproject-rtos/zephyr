/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AUDIO_IN_H_
#define AUDIO_IN_H_

#include <stddef.h>
#include <stdint.h>

/* Input stream: 48 kHz, 16-bit, interleaved stereo, delivered in 1 ms USB frames */
#define AUDIO_IN_RATE_HZ  48000U
#define AUDIO_IN_CHANNELS 2U

/* Full-speed UAC2 feedback is Q10.14 samples per 1 ms frame */
#define AUDIO_IN_FB_SHIFT   14U
#define AUDIO_IN_FB_NOMINAL ((AUDIO_IN_RATE_HZ / 1000U) << AUDIO_IN_FB_SHIFT)

/**
 * @brief Initialize the audio input path.
 *
 * @param rate_hz Broadcast sample rate; AUDIO_IN_RATE_HZ shall be a multiple of it.
 * @param frame_samples Samples per channel in one SDU (codec frame).
 *
 * @retval 0 Success
 * @retval -EINVAL Unsupported rate or frame size
 */
int audio_in_init(uint32_t rate_hz, size_t frame_samples);

/**
 * @brief Producer: push 48 kHz interleaved stereo PCM.
 *
 * Single producer context only (USB class thread or the emulated host timer).
 */
void audio_in_put(const int16_t *pcm, size_t frames);

/**
 * @brief Consumer: get one SDU frame per channel, paced by the ISO stream.
 *
 * Single consumer context only (encoder thread). Outputs silence while priming.
 */
void audio_in_get(int16_t *left, int16_t *right);

/** @brief Request a flush of buffered audio; any context. */
void audio_in_flush(void);

/** @brief Current feedback value in Q10.14 samples per 1 ms frame. */
uint32_t audio_in_feedback_get(void);

/** @brief Start the emulated USB host (CONFIG_AUDIO_INPUT_EMULATED). */
void audio_in_emulated_start(void);

#endif /* AUDIO_IN_H_ */
