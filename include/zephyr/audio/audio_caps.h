/*
 * Copyright 2025, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common Audio Capabilities Definitions
 *
 * This file contains common audio format and sample rate definitions used across
 * all audio subsystems (DMIC, I2S, Audio Codec, etc.)
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_CAPS_H_
#define ZEPHYR_INCLUDE_AUDIO_CAPS_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup audio_bit_width Audio Bit Width Definitions
 * @brief Common audio bit width definitions - can be used across DMIC, I2S, Audio Codec APIs
 * @{
 */
/** @brief 8 bit width */
#define AUDIO_BIT_WIDTH_8  BIT(0)
/** @brief 16 bit width */
#define AUDIO_BIT_WIDTH_16 BIT(1)
/** @brief 18 bit width */
#define AUDIO_BIT_WIDTH_18 BIT(2)
/** @brief 20 bit width */
#define AUDIO_BIT_WIDTH_20 BIT(3)
/** @brief 24 bit width */
#define AUDIO_BIT_WIDTH_24 BIT(4)
/** @brief 32 bit width */
#define AUDIO_BIT_WIDTH_32 BIT(5)
/** @brief 64 bit width */
#define AUDIO_BIT_WIDTH_64 BIT(6)
/** @} */

/**
 * @defgroup audio_sample_rate Audio Sample Rate Definitions
 * @brief Common audio sample rate definitions - can be used across DMIC, I2S, Audio Codec APIs
 * @{
 */
/** @brief 7.35 kHz sample rate */
#define AUDIO_SAMPLE_RATE_7350   BIT(0)
/** @brief 8 kHz sample rate */
#define AUDIO_SAMPLE_RATE_8000   BIT(1)
/** @brief 11.025 kHz sample rate */
#define AUDIO_SAMPLE_RATE_11025  BIT(2)
/** @brief 12 kHz sample rate */
#define AUDIO_SAMPLE_RATE_12000  BIT(3)
/** @brief 14.7 kHz sample rate */
#define AUDIO_SAMPLE_RATE_14700  BIT(4)
/** @brief 16 kHz sample rate */
#define AUDIO_SAMPLE_RATE_16000  BIT(5)
/** @brief 20 kHz sample rate */
#define AUDIO_SAMPLE_RATE_20000  BIT(6)
/** @brief 22.05 kHz sample rate */
#define AUDIO_SAMPLE_RATE_22050  BIT(7)
/** @brief 24 kHz sample rate */
#define AUDIO_SAMPLE_RATE_24000  BIT(8)
/** @brief 29.4 kHz sample rate */
#define AUDIO_SAMPLE_RATE_29400  BIT(9)
/** @brief 32 kHz sample rate */
#define AUDIO_SAMPLE_RATE_32000  BIT(10)
/** @brief 44.1 kHz sample rate */
#define AUDIO_SAMPLE_RATE_44100  BIT(11)
/** @brief 48 kHz sample rate */
#define AUDIO_SAMPLE_RATE_48000  BIT(12)
/** @brief 50 kHz sample rate */
#define AUDIO_SAMPLE_RATE_50000  BIT(13)
/** @brief 50.4 kHz sample rate */
#define AUDIO_SAMPLE_RATE_50400  BIT(14)
/** @brief 64 kHz sample rate */
#define AUDIO_SAMPLE_RATE_64000  BIT(15)
/** @brief 88.2 kHz sample rate */
#define AUDIO_SAMPLE_RATE_88200  BIT(16)
/** @brief 96 kHz sample rate */
#define AUDIO_SAMPLE_RATE_96000  BIT(17)
/** @brief 100.8 kHz sample rate */
#define AUDIO_SAMPLE_RATE_100800 BIT(18)
/** @brief 128 kHz sample rate */
#define AUDIO_SAMPLE_RATE_128000 BIT(19)
/** @brief 176.4 kHz sample rate */
#define AUDIO_SAMPLE_RATE_176400 BIT(20)
/** @brief 192 kHz sample rate */
#define AUDIO_SAMPLE_RATE_192000 BIT(21)
/** @brief 352.8 kHz sample rate */
#define AUDIO_SAMPLE_RATE_352800 BIT(22)
/** @brief 384 kHz sample rate */
#define AUDIO_SAMPLE_RATE_384000 BIT(23)
/** @brief 705.6 kHz sample rate */
#define AUDIO_SAMPLE_RATE_705600 BIT(24)
/** @brief 768 kHz sample rate */
#define AUDIO_SAMPLE_RATE_768000 BIT(25)
/** @} */

/**
 * @struct audio_caps
 * @brief Audio capabilities structure
 *
 * Structure that describes the audio capabilities of a device, including
 * supported channels, sample rates, bit widths, buffering requirements,
 * and data layout format.
 */
struct audio_caps {
	/** Minimum supported number of channels */
	uint8_t min_total_channels;
	/** Maximum supported number of channels */
	uint8_t max_total_channels;
	/** Bitwise OR of supported PCM sample rates */
	uint32_t supported_sample_rates;
	/** Bitwise OR of supported PCM bit width */
	uint32_t supported_bit_widths;
	/** Minimum number of data buffers required */
	uint8_t min_num_buffers;
	/** Minimum supported frame interval in microseconds */
	uint32_t min_frame_interval;
	/** Maximum supported frame interval in microseconds */
	uint32_t max_frame_interval;
	/** The layout of channels within a buffer: interleaved (LRLRLRLR) and non-interleaved
	 * (LLLLRRRR) */
	bool interleaved;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_AUDIO_CAPS_H_ */
