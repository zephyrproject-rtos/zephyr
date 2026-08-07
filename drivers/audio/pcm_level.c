/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include "pcm_level.h"

#define PCM_LEVEL_LOG2_FRAC_BITS       16
#define PCM_LEVEL_LOG2_ONE             (1LL << PCM_LEVEL_LOG2_FRAC_BITS)
#define PCM_LEVEL_FS_SQ_LOG2           30
#define PCM_LEVEL_DB_SCALE             1000
#define PCM_LEVEL_DB_TENTHS_PER_OCTAVE 30103 /* round(30.103 * PCM_LEVEL_DB_SCALE) */

/* Fixed-point base-2 logarithm: returns log2(x) in Q16 for x >= 1. */
static int32_t log2_q16(uint64_t x)
{
	int32_t intpart = LOG2(x);
	int32_t frac = 0;
	uint64_t y;

	/* Normalize to a Q30 mantissa in [2^30, 2^31), i.e. [1.0, 2.0). */
	if (intpart >= 30) {
		y = x >> (intpart - 30);
	} else {
		y = x << (30 - intpart);
	}

	/* Recover the fractional bits of log2 by repeated squaring. */
	for (int i = 0; i < PCM_LEVEL_LOG2_FRAC_BITS; i++) {
		y = (y * y) >> 30;
		frac <<= 1;
		if (y >= (1ULL << 31)) {
			y >>= 1;
			frac |= 1;
		}
	}

	return (intpart << PCM_LEVEL_LOG2_FRAC_BITS) | frac;
}

int32_t pcm_level_power_dbfs_tenths(uint64_t power)
{
	const int64_t divisor = PCM_LEVEL_LOG2_ONE * PCM_LEVEL_DB_SCALE;
	const int64_t half = divisor / 2;
	int64_t l, num;

	if (power == 0U) {
		return INT32_MIN;
	}

	/* log2(power / FS_SQ) in Q16, <= 0. */
	l = (int64_t)log2_q16(power) - ((int64_t)PCM_LEVEL_FS_SQ_LOG2 << PCM_LEVEL_LOG2_FRAC_BITS);
	/* Octaves of power -> tenths of a dB, undoing the Q16 and DB_SCALE factors, rounded. */
	num = l * PCM_LEVEL_DB_TENTHS_PER_OCTAVE;
	return (int32_t)((num >= 0) ? (num + half) / divisor : (num - half) / divisor);
}

/*
 * Decode interleaved sample i and normalize it to Q15 full scale.
 */
static int32_t decode_sample_q15(const void *buf, size_t i, uint8_t pcm_width)
{
	switch (pcm_width) {
	case 8U:
		return (int32_t)((const int8_t *)buf)[i] << 8; /* 2^7 -> 2^15 */
	case 16U:
		return ((const int16_t *)buf)[i]; /* already Q15 */
	case 24U: {
		const uint8_t *p = &((const uint8_t *)buf)[i * 3U];
		uint32_t raw = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);

		return sign_extend(raw, 23) >> 8; /* sign-extend bit 23, then 2^23 -> 2^15 */
	}
	default:                                        /* 32-bit */
		return ((const int32_t *)buf)[i] >> 16; /* 2^31 -> 2^15 */
	}
}

int pcm_level_analyze(const void *buf, size_t size, uint8_t pcm_width, uint8_t num_channels,
		      uint8_t channel, struct pcm_level_meas *m)
{
	size_t num_samples;
	uint32_t clips = 0;
	int32_t peak = 0;

	m->peak_n = 0;
	m->clips = 0;

	/* Reject parameters that would divide by zero, loop forever or decode the wrong width. */
	if ((buf == NULL) || (num_channels == 0U) || (channel >= num_channels)) {
		return -EINVAL;
	}

	if ((pcm_width != 8U) && (pcm_width != 16U) && (pcm_width != 24U) && (pcm_width != 32U)) {
		return -EINVAL;
	}

	num_samples = size / (pcm_width / 8U);

	for (size_t i = channel; i < num_samples; i += num_channels) {
		int32_t x = decode_sample_q15(buf, i, pcm_width);
		int32_t mag = (x < 0) ? -x : x;

		peak = MAX(peak, mag);
		clips += (mag >= PCM_LEVEL_CLIP_N) ? 1U : 0U;
	}

	m->peak_n = peak;
	m->clips = clips;

	return 0;
}
