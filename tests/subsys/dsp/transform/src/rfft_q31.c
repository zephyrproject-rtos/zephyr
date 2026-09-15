/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/dsp.h>
#include <zephyr/dsp/utils.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "common/test_common.h"

#define TEST_PI          3.14159265358979323846f
/*
 * The Q31 transform is computed in fixed point with per-stage downscaling, so a moderate
 * SNR threshold is used when cross-checking against the (trusted) f32 transform. A correct
 * transform comfortably exceeds this; a broken one (wrong scaling/ordering) collapses to
 * near 0 dB.
 */
#define SNR_ERROR_THRESH ((float32_t)60)

/*
 * The Q31 real FFT does not use the packed layout of the float transform: bins 0 to
 * fft_len/2 hold the non-redundant half of the spectrum, and the scalar implementation
 * additionally writes the conjugate-symmetric upper half, so the output buffer must always
 * be 2*fft_len Q31 values wide. Only the non-redundant half is inspected here, as that is
 * all the MVE implementation writes. The input buffer is modified by the transform.
 *
 * A forward/inverse round trip is not exercised: both directions of the Q31 transform carry
 * a 1/fft_len scaling, so a round trip lands roughly 1/fft_len^2 down. The forward transform
 * is instead cross-checked against the f32 transform, whose scaling is documented and exact.
 */

/* A DC (constant) input concentrates all energy in bin 0. */
static void test_zdsp_rfft_q31_dc(uint16_t fft_len)
{
	struct zdsp_rfft_instance_q31 inst;
	q31_t *in;
	q31_t *out;
	uint64_t max_mag2 = 0;
	uint16_t max_bin = 0;

	in = malloc(fft_len * sizeof(q31_t));
	out = malloc(2 * fft_len * sizeof(q31_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		/*
		 * 0.25, not the 0.5 the Q15 test uses: the split stage that
		 * assembles the real spectrum sums both halves, and in Q31 that
		 * sum wraps at half-scale DC instead of saturating.
		 */
		in[n] = 0x20000000;
	}

	zassert_equal(zdsp_rfft_init_q31(&inst, fft_len, 0, 1), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_q31(&inst, in, out);

	/* Bins 0 to fft_len / 2 are the non-redundant half of the spectrum */
	for (uint16_t m = 0; m <= fft_len / 2; m++) {
		int64_t re = out[2 * m];
		int64_t im = out[2 * m + 1];
		uint64_t mag2 = (uint64_t)(re * re) + (uint64_t)(im * im);

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, 0, "DC energy not in bin 0 (got bin %u)", max_bin);
	zassert_true(out[0] > 0, "DC bin should be positive (got %d)", out[0]);

	free(in);
	free(out);
}

/*
 * A real cosine at bin @c k places its energy at bins k and fft_len-k. Restricting the
 * search to the first half [1, fft_len/2], the magnitude peak must be exactly bin k.
 */
static void test_zdsp_rfft_q31_tone(uint16_t fft_len)
{
	struct zdsp_rfft_instance_q31 inst;
	const uint16_t k = 3;
	q31_t *in;
	q31_t *out;
	uint64_t max_mag2 = 0;
	uint16_t max_bin = 0;

	in = malloc(fft_len * sizeof(q31_t));
	out = malloc(2 * fft_len * sizeof(q31_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		in[n] = zdsp_f32_to_q31_shift(0.5f * cosf(2.0f * TEST_PI * k * n / fft_len), 0);
	}

	zassert_equal(zdsp_rfft_init_q31(&inst, fft_len, 0, 1), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_q31(&inst, in, out);

	for (uint16_t m = 1; m <= fft_len / 2; m++) {
		int64_t re = out[2 * m];
		int64_t im = out[2 * m + 1];
		uint64_t mag2 = (uint64_t)(re * re) + (uint64_t)(im * im);

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, k, "spectral peak in wrong bin (got %u, expected %u)", max_bin, k);

	free(in);
	free(out);
}

/*
 * Cross-check the Q31 forward transform against the f32 transform on the same signal. The
 * Q31 forward transform is downscaled by 1/fft_len, so the f32 reference is divided by
 * fft_len before comparison. The two transforms use different output layouts, so both are
 * unpacked into bins 0 to fft_len/2 as interleaved {real, imag} pairs: the f32 transform
 * packs the real DC term in element 0 and the real Nyquist term in element 1, while the
 * Q31 transform stores bin m at elements 2*m and 2*m+1.
 */
static void test_zdsp_rfft_q31_vs_f32(uint16_t fft_len)
{
	struct zdsp_rfft_instance_q31 inst_q31;
	struct zdsp_rfft_fast_instance_f32 inst_f32;
	/* Bins 0 to fft_len/2 inclusive, as interleaved {real, imag} pairs. */
	size_t len = fft_len + 2;
	q31_t *qin;
	q31_t *qspec;
	float32_t *fin;
	float32_t *fspec;
	float32_t *qout;
	float32_t *ref;

	qin = malloc(fft_len * sizeof(q31_t));
	qspec = malloc(2 * fft_len * sizeof(q31_t));
	fin = malloc(fft_len * sizeof(float32_t));
	fspec = calloc(fft_len + 2, sizeof(float32_t));
	qout = malloc(len * sizeof(float32_t));
	ref = malloc(len * sizeof(float32_t));
	zassert_not_null(qin, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(qspec, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(fin, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(fspec, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(qout, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(ref, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t t = (float32_t)n / (float32_t)fft_len;
		/* The DC term keeps bins 0 and fft_len/2 out of the noise. */
		float32_t x = 0.25f + 0.4f * sinf(2.0f * TEST_PI * 3.0f * t) +
			      0.2f * cosf(2.0f * TEST_PI * 9.0f * t);

		fin[n] = x;
		qin[n] = zdsp_f32_to_q31_shift(x, 0);
	}

	zassert_equal(zdsp_rfft_init_q31(&inst_q31, fft_len, 0, 1), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zassert_equal(zdsp_rfft_fast_init_f32(&inst_f32, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	zdsp_rfft_q31(&inst_q31, qin, qspec);
	zdsp_rfft_fast_f32(&inst_f32, fin, fspec, 0);

	for (size_t i = 0; i < len; i++) {
		qout[i] = (float32_t)qspec[i] / 2147483648.0f;
	}

	/* DC and Nyquist are real; the f32 transform packs them into elements 0 and 1. */
	ref[0] = fspec[0] / (float32_t)fft_len;
	ref[1] = 0.0f;
	ref[fft_len] = fspec[1] / (float32_t)fft_len;
	ref[fft_len + 1] = 0.0f;
	for (size_t i = 2; i < fft_len; i++) {
		ref[i] = fspec[i] / (float32_t)fft_len;
	}

	zassert_true(test_snr_error_f32(len, qout, ref, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(qin);
	free(qspec);
	free(fin);
	free(fspec);
	free(qout);
	free(ref);
}

#define DEFINE_RFFT_Q31_TESTS(len)                                                                 \
	ZTEST(transform_rfft_q31, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_rfft_q31_dc(len);                                                        \
	}                                                                                          \
	ZTEST(transform_rfft_q31, test_tone_##len)                                                 \
	{                                                                                          \
		test_zdsp_rfft_q31_tone(len);                                                      \
	}                                                                                          \
	ZTEST(transform_rfft_q31, test_vs_f32_##len)                                               \
	{                                                                                          \
		test_zdsp_rfft_q31_vs_f32(len);                                                    \
	}

DEFINE_RFFT_Q31_TESTS(32)
DEFINE_RFFT_Q31_TESTS(64)
DEFINE_RFFT_Q31_TESTS(128)
DEFINE_RFFT_Q31_TESTS(256)

ZTEST_SUITE(transform_rfft_q31, NULL, NULL, NULL, NULL, NULL);
