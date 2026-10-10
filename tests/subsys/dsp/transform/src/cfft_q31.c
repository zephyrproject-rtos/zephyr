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
/* Same relative tolerance as the Q15 test: 200 LSB at the Q15 scale. */
#define DC_ABS_THRESH    ((q31_t)(200 * 65536))

/*
 * A DC (constant real) input must, after the 1/fft_len forward scaling, leave bin 0 equal
 * to the input value and all other bins near zero.
 */
static void test_zdsp_cfft_q31_dc(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q31 inst;
	const q31_t dc = 0x40000000; /* 0.5 in Q1.31 */
	q31_t *buf;

	buf = malloc(2 * fft_len * sizeof(q31_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		buf[2 * n] = dc;
		buf[2 * n + 1] = 0;
	}

	zassert_equal(zdsp_cfft_init_q31(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_q31(&inst, buf, 0, 1);

	zassert_within(buf[0], dc, DC_ABS_THRESH, "DC bin (real) incorrect");
	zassert_within(buf[1], 0, DC_ABS_THRESH, "DC bin (imag) incorrect");
	for (uint16_t k = 1; k < fft_len; k++) {
		zassert_within(buf[2 * k], 0, DC_ABS_THRESH, "non-DC bin (real) not zero");
		zassert_within(buf[2 * k + 1], 0, DC_ABS_THRESH, "non-DC bin (imag) not zero");
	}

	free(buf);
}

/*
 * A single complex exponential at bin @c k produces a single non-zero bin. The argmax of
 * the magnitude spectrum must be exactly @c k, independent of fixed-point scaling.
 */
static void test_zdsp_cfft_q31_tone(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q31 inst;
	const uint16_t k = 2;
	q31_t *buf;
	uint64_t max_mag2 = 0;
	uint16_t max_bin = 0;

	buf = malloc(2 * fft_len * sizeof(q31_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t phase = 2.0f * TEST_PI * k * n / fft_len;

		buf[2 * n] = zdsp_f32_to_q31_shift(0.4f * cosf(phase), 0);
		buf[2 * n + 1] = zdsp_f32_to_q31_shift(0.4f * sinf(phase), 0);
	}

	zassert_equal(zdsp_cfft_init_q31(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_q31(&inst, buf, 0, 1);

	/* Squares of two Q31 values sum to at most 2^63, so accumulate unsigned. */
	for (uint16_t m = 0; m < fft_len; m++) {
		int64_t re = buf[2 * m];
		int64_t im = buf[2 * m + 1];
		uint64_t mag2 = (uint64_t)(re * re) + (uint64_t)(im * im);

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, k, "spectral peak in wrong bin (got %u, expected %u)", max_bin, k);

	free(buf);
}

/*
 * Cross-check the Q31 forward transform against the f32 transform on the same signal. The
 * Q31 forward transform is downscaled by 1/fft_len, so the f32 reference is divided by
 * fft_len before comparison.
 */
static void test_zdsp_cfft_q31_vs_f32(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q31 inst_q31;
	struct zdsp_cfft_instance_f32 inst_f32;
	size_t len = 2 * fft_len;
	q31_t *qbuf;
	float32_t *fbuf;
	float32_t *qout;
	float32_t *ref;

	qbuf = malloc(len * sizeof(q31_t));
	fbuf = malloc(len * sizeof(float32_t));
	qout = malloc(len * sizeof(float32_t));
	ref = malloc(len * sizeof(float32_t));
	zassert_not_null(qbuf, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(fbuf, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(qout, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(ref, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t t = (float32_t)n / (float32_t)fft_len;
		float32_t re = 0.3f * sinf(2.0f * TEST_PI * 3.0f * t) +
			       0.1f * cosf(2.0f * TEST_PI * 6.0f * t);
		float32_t im = 0.2f * sinf(2.0f * TEST_PI * 4.0f * t);

		fbuf[2 * n] = re;
		fbuf[2 * n + 1] = im;
		qbuf[2 * n] = zdsp_f32_to_q31_shift(re, 0);
		qbuf[2 * n + 1] = zdsp_f32_to_q31_shift(im, 0);
	}

	zassert_equal(zdsp_cfft_init_q31(&inst_q31, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zassert_equal(zdsp_cfft_init_f32(&inst_f32, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	zdsp_cfft_q31(&inst_q31, qbuf, 0, 1);
	zdsp_cfft_f32(&inst_f32, fbuf, 0, 1);

	for (size_t i = 0; i < len; i++) {
		qout[i] = (float32_t)qbuf[i] / 2147483648.0f;
		ref[i] = fbuf[i] / (float32_t)fft_len;
	}

	zassert_true(test_snr_error_f32(len, qout, ref, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(qbuf);
	free(fbuf);
	free(qout);
	free(ref);
}

#define DEFINE_CFFT_Q31_TESTS(len)                                                                 \
	ZTEST(transform_cfft_q31, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_cfft_q31_dc(len);                                                        \
	}                                                                                          \
	ZTEST(transform_cfft_q31, test_tone_##len)                                                 \
	{                                                                                          \
		test_zdsp_cfft_q31_tone(len);                                                      \
	}                                                                                          \
	ZTEST(transform_cfft_q31, test_vs_f32_##len)                                               \
	{                                                                                          \
		test_zdsp_cfft_q31_vs_f32(len);                                                    \
	}

DEFINE_CFFT_Q31_TESTS(16)
DEFINE_CFFT_Q31_TESTS(32)
DEFINE_CFFT_Q31_TESTS(64)
DEFINE_CFFT_Q31_TESTS(128)

ZTEST_SUITE(transform_cfft_q31, NULL, NULL, NULL, NULL, NULL);
