/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/dsp.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "common/test_common.h"

#define TEST_PI 3.14159265358979323846f

static q15_t f32_to_q15(float32_t x)
{
	int32_t v = (int32_t)lroundf(x * 32768.0f);

	if (v > INT16_MAX) {
		v = INT16_MAX;
	} else if (v < INT16_MIN) {
		v = INT16_MIN;
	}
	return (q15_t)v;
}

/*
 * The Q15 real FFT output uses a size-dependent Q format (it is not the packed float
 * format), so these tests validate structural properties of the forward transform that are
 * independent of the exact scaling: which bin carries the energy.
 *
 * The output buffer holds the full complex spectrum (fft_len complex bins, i.e. 2*fft_len
 * Q15 values, including the conjugate-symmetric upper half). The input buffer is modified
 * by the transform.
 */

/* A DC (constant) input concentrates all energy in bin 0. */
static void test_zdsp_rfft_q15_dc(uint16_t fft_len)
{
	struct zdsp_rfft_instance_q15 inst;
	q15_t *in;
	q15_t *out;
	int64_t max_mag2 = -1;
	uint16_t max_bin = 0;

	in = malloc(fft_len * sizeof(q15_t));
	out = malloc(2 * fft_len * sizeof(q15_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		in[n] = 0x4000; /* 0.5 in Q1.15 */
	}

	zassert_equal(zdsp_rfft_init_q15(&inst, fft_len, 0, 1), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_q15(&inst, in, out);

	/* Only bins 0 to fft_len / 2 are written; the rest of the buffer is untouched */
	for (uint16_t m = 0; m <= fft_len / 2; m++) {
		int64_t re = out[2 * m];
		int64_t im = out[2 * m + 1];
		int64_t mag2 = re * re + im * im;

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, 0, "DC energy not in bin 0 (got bin %u)", max_bin);
	zassert_true(out[0] > 0, "DC bin should be positive");

	free(in);
	free(out);
}

/*
 * A real cosine at bin @c k places its energy at bins k and fft_len-k. Restricting the
 * search to the first half [1, fft_len/2], the magnitude peak must be exactly bin k.
 */
static void test_zdsp_rfft_q15_tone(uint16_t fft_len)
{
	struct zdsp_rfft_instance_q15 inst;
	const uint16_t k = 3;
	q15_t *in;
	q15_t *out;
	int64_t max_mag2 = -1;
	uint16_t max_bin = 0;

	in = malloc(fft_len * sizeof(q15_t));
	out = malloc(2 * fft_len * sizeof(q15_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		in[n] = f32_to_q15(0.5f * cosf(2.0f * TEST_PI * k * n / fft_len));
	}

	zassert_equal(zdsp_rfft_init_q15(&inst, fft_len, 0, 1), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_q15(&inst, in, out);

	for (uint16_t m = 1; m <= fft_len / 2; m++) {
		int64_t re = out[2 * m];
		int64_t im = out[2 * m + 1];
		int64_t mag2 = re * re + im * im;

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, k, "spectral peak in wrong bin (got %u, expected %u)", max_bin, k);

	free(in);
	free(out);
}

#define DEFINE_RFFT_Q15_TESTS(len)                                                                 \
	ZTEST(transform_rfft_q15, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_rfft_q15_dc(len);                                                        \
	}                                                                                          \
	ZTEST(transform_rfft_q15, test_tone_##len)                                                 \
	{                                                                                          \
		test_zdsp_rfft_q15_tone(len);                                                      \
	}

DEFINE_RFFT_Q15_TESTS(32)
DEFINE_RFFT_Q15_TESTS(64)
DEFINE_RFFT_Q15_TESTS(128)
DEFINE_RFFT_Q15_TESTS(256)

ZTEST_SUITE(transform_rfft_q15, NULL, NULL, NULL, NULL, NULL);
