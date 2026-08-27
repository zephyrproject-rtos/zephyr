/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PCM level analysis: per-channel peak/clip measurement and dBFS conversion.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_PCM_LEVEL_H_
#define ZEPHYR_DRIVERS_AUDIO_PCM_LEVEL_H_

#include <stddef.h>
#include <stdint.h>

/** Full-scale amplitude (Q15). */
#define PCM_LEVEL_FS_N   (1U << 15)
/** Full-scale power, the 0 dBFS reference. */
#define PCM_LEVEL_FS_SQ  (1ULL << 30)
/** A sample at or above this magnitude (~ -0.01 dBFS) counts as clipping. */
#define PCM_LEVEL_CLIP_N (PCM_LEVEL_FS_N - (PCM_LEVEL_FS_N >> 10))

/** Per-channel measurements for one block, from PCM normalized to Q15 full scale. */
struct pcm_level_meas {
	int32_t peak_n; /**< Peak |sample|, 0..#PCM_LEVEL_FS_N. */
	uint32_t clips; /**< Samples at or above the clip threshold. */
};

/**
 * @brief Analyze one interleaved channel of a PCM block.
 *
 * Computes the peak amplitude and clip count for a single channel, with samples normalized to Q15
 * full scale.
 *
 * On invalid input @p m is set to a zeroed measurement and @c -EINVAL is returned.
 *
 * @param buf          Interleaved PCM block.
 * @param size         Size of @p buf in bytes.
 * @param pcm_width    Sample width in bits: 8, 16, 24 (packed, little-endian) or 32.
 * @param num_channels Number of interleaved channels.
 * @param channel      0-based channel index to analyse.
 * @param[out] m       Measurement result.
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid parameters.
 */
int pcm_level_analyze(const void *buf, size_t size, uint8_t pcm_width, uint8_t num_channels,
		      uint8_t channel, struct pcm_level_meas *m);

/**
 * @brief Convert a PCM power level to dBFS.
 *
 * @param power Squared Q15 amplitude
 *
 * @return Level in tenths of a dBFS (always <= 0), or @c INT32_MIN ("-inf") for digital silence.
 */
int32_t pcm_level_power_dbfs_tenths(uint64_t power);

#endif /* ZEPHYR_DRIVERS_AUDIO_PCM_LEVEL_H_ */
