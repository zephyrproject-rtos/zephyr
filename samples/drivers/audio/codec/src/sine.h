/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_AUDIO_CODEC_SINE_H_
#define ZEPHYR_SAMPLES_DRIVERS_AUDIO_CODEC_SINE_H_

#include <stddef.h>
#include <stdint.h>

/*
 * Integer-only sine tone generator.
 *
 * A 256-entry signed 16-bit quarter-symmetric lookup table is walked with a
 * 16.16 fixed-point phase accumulator and linear interpolation between the two
 * nearest entries, so any frequency can be produced at any sample rate without
 * floating point and without a baked PCM table.
 */

#define SINE_POINTS    256
#define SINE_MASK      (SINE_POINTS - 1)
#define SINE_FRAC_BITS 16U

/* One sine period, peak 16384 (about -6 dBFS). */
static const int16_t sine_table[SINE_POINTS] = {
	     0,    402,    804,   1205,   1606,   2006,   2404,   2801,
	  3196,   3590,   3981,   4370,   4756,   5139,   5520,   5897,
	  6270,   6639,   7005,   7366,   7723,   8076,   8423,   8765,
	  9102,   9434,   9760,  10080,  10394,  10702,  11003,  11297,
	 11585,  11866,  12140,  12406,  12665,  12916,  13160,  13395,
	 13623,  13842,  14053,  14256,  14449,  14635,  14811,  14978,
	 15137,  15286,  15426,  15557,  15679,  15791,  15893,  15986,
	 16069,  16143,  16207,  16261,  16305,  16340,  16364,  16379,
	 16384,  16379,  16364,  16340,  16305,  16261,  16207,  16143,
	 16069,  15986,  15893,  15791,  15679,  15557,  15426,  15286,
	 15137,  14978,  14811,  14635,  14449,  14256,  14053,  13842,
	 13623,  13395,  13160,  12916,  12665,  12406,  12140,  11866,
	 11585,  11297,  11003,  10702,  10394,  10080,   9760,   9434,
	  9102,   8765,   8423,   8076,   7723,   7366,   7005,   6639,
	  6270,   5897,   5520,   5139,   4756,   4370,   3981,   3590,
	  3196,   2801,   2404,   2006,   1606,   1205,    804,    402,
	     0,   -402,   -804,  -1205,  -1606,  -2006,  -2404,  -2801,
	 -3196,  -3590,  -3981,  -4370,  -4756,  -5139,  -5520,  -5897,
	 -6270,  -6639,  -7005,  -7366,  -7723,  -8076,  -8423,  -8765,
	 -9102,  -9434,  -9760, -10080, -10394, -10702, -11003, -11297,
	-11585, -11866, -12140, -12406, -12665, -12916, -13160, -13395,
	-13623, -13842, -14053, -14256, -14449, -14635, -14811, -14978,
	-15137, -15286, -15426, -15557, -15679, -15791, -15893, -15986,
	-16069, -16143, -16207, -16261, -16305, -16340, -16364, -16379,
	-16384, -16379, -16364, -16340, -16305, -16261, -16207, -16143,
	-16069, -15986, -15893, -15791, -15679, -15557, -15426, -15286,
	-15137, -14978, -14811, -14635, -14449, -14256, -14053, -13842,
	-13623, -13395, -13160, -12916, -12665, -12406, -12140, -11866,
	-11585, -11297, -11003, -10702, -10394, -10080,  -9760,  -9434,
	 -9102,  -8765,  -8423,  -8076,  -7723,  -7366,  -7005,  -6639,
	 -6270,  -5897,  -5520,  -5139,  -4756,  -4370,  -3981,  -3590,
	 -3196,  -2801,  -2404,  -2006,  -1606,  -1205,   -804,   -402,
};

struct sine_gen {
	uint32_t phase;     /* 16.16 fixed-point index into sine_table */
	uint32_t phase_inc; /* phase step per output frame */
};

/*
 * Set up gen to produce freq_hz at sample_rate_hz. sample_rate_hz must be
 * non-zero and larger than twice freq_hz to stay below Nyquist.
 */
static inline void sine_gen_init(struct sine_gen *gen, uint32_t freq_hz, uint32_t sample_rate_hz)
{
	gen->phase = 0U;
	gen->phase_inc =
		(uint32_t)(((uint64_t)freq_hz << SINE_FRAC_BITS) * SINE_POINTS / sample_rate_hz);
}

/* Return the next 16-bit sample and advance the phase. */
static inline int16_t sine_gen_next(struct sine_gen *gen)
{
	uint32_t idx = (gen->phase >> SINE_FRAC_BITS) & SINE_MASK;
	int32_t frac = (int32_t)(gen->phase & ((1U << SINE_FRAC_BITS) - 1U));
	int32_t s0 = sine_table[idx];
	int32_t s1 = sine_table[(idx + 1U) & SINE_MASK];

	gen->phase += gen->phase_inc;

	return (int16_t)(s0 + (((s1 - s0) * frac) >> SINE_FRAC_BITS));
}

/*
 * Fill dst with frames interleaved 16-bit samples, the same value written to
 * each of channels lanes per frame. dst holds frames * channels int16_t.
 */
static inline void sine_gen_fill_s16(struct sine_gen *gen, int16_t *dst, size_t frames,
				     uint8_t channels)
{
	size_t f;
	uint8_t c;

	for (f = 0; f < frames; f++) {
		int16_t sample = sine_gen_next(gen);

		for (c = 0; c < channels; c++) {
			*dst++ = sample;
		}
	}
}

#endif /* ZEPHYR_SAMPLES_DRIVERS_AUDIO_CODEC_SINE_H_ */
