/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio definitions and utilities header file.
 */

#ifndef __MP_ZAUD_H__
#define __MP_ZAUD_H__

#include <zephyr/audio/audio_caps.h>

/**
 * @brief Supported MediaPipe sample rates (Hz)
 * @{
 */
/** @brief 8 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_8000  8000
/** @brief 16 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_16000 16000
/** @brief 32 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_32000 32000
/** @brief 44.1 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_44100 44100
/** @brief 48 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_48000 48000
/** @brief 96 kHz sample rate */
#define MP_ZAUD_SAMPLE_RATE_96000 96000
/** @} */

/**
 * @brief Supported MediaPipe bit widths
 * @{
 */
/** @brief 16 bit width */
#define MP_ZAUD_BIT_WIDTH_16 16
/** @brief 24 bit width */
#define MP_ZAUD_BIT_WIDTH_24 24
/** @brief 32 bit width */
#define MP_ZAUD_BIT_WIDTH_32 32
/** @} */

/**
 * @brief Convert audio sample rate mask to MediaPipe sample rate
 *
 * @param sample_rate_mask Audio driver sample rate mask
 * @return Corresponding MediaPipe sample rate value
 */
const uint32_t audio2mp_sample_rate(uint32_t sample_rate_mask);

/**
 * @brief Convert audio bit width mask to MediaPipe bit width
 *
 * @param bit_width_mask Audio driver bit width mask
 * @return Corresponding MediaPipe bit width value
 */
const uint32_t audio2mp_bit_width(uint32_t bit_width_mask);

#endif /* __MP_ZAUD_H__ */
