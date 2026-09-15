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

#define TEST_PI          3.14159265358979323846f
/*
 * The Q15 transform is computed in fixed point with per-stage downscaling, so a moderate
 * SNR threshold is used when cross-checking against the (trusted) f32 transform. A correct
 * transform comfortably exceeds this; a broken one (wrong scaling/ordering) collapses to
 * near 0 dB.
 */
#define SNR_ERROR_THRESH ((float32_t)20)
#define DC_ABS_THRESH    ((q15_t)200)

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
 * A DC (constant real) input must, after the 1/fft_len forward scaling, leave bin 0 equal
 * to the input value and all other bins near zero.
 */
static void test_zdsp_cfft_q15_dc(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q15 inst;
	const q15_t dc = 0x4000; /* 0.5 in Q1.15 */
	q15_t *buf;

	buf = malloc(2 * fft_len * sizeof(q15_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		buf[2 * n] = dc;
		buf[2 * n + 1] = 0;
	}

	zassert_equal(zdsp_cfft_init_q15(&inst, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_q15(&inst, buf, 0, 1);

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
static void test_zdsp_cfft_q15_tone(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q15 inst;
	const uint16_t k = 2;
	q15_t *buf;
	int64_t max_mag2 = -1;
	uint16_t max_bin = 0;

	buf = malloc(2 * fft_len * sizeof(q15_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t phase = 2.0f * TEST_PI * k * n / fft_len;

		buf[2 * n] = f32_to_q15(0.4f * cosf(phase));
		buf[2 * n + 1] = f32_to_q15(0.4f * sinf(phase));
	}

	zassert_equal(zdsp_cfft_init_q15(&inst, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_q15(&inst, buf, 0, 1);

	for (uint16_t m = 0; m < fft_len; m++) {
		int64_t re = buf[2 * m];
		int64_t im = buf[2 * m + 1];
		int64_t mag2 = re * re + im * im;

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, k, "spectral peak in wrong bin (got %u, expected %u)", max_bin, k);

	free(buf);
}

/*
 * Cross-check the Q15 forward transform against the f32 transform on the same signal. The
 * Q15 forward transform is downscaled by 1/fft_len, so the f32 reference is divided by
 * fft_len before comparison.
 */
static void test_zdsp_cfft_q15_vs_f32(uint16_t fft_len)
{
	struct zdsp_cfft_instance_q15 inst_q15;
	struct zdsp_cfft_instance_f32 inst_f32;
	size_t len = 2 * fft_len;
	q15_t *qbuf;
	float32_t *fbuf;
	float32_t *qout;
	float32_t *ref;

	qbuf = malloc(len * sizeof(q15_t));
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
		qbuf[2 * n] = f32_to_q15(re);
		qbuf[2 * n + 1] = f32_to_q15(im);
	}

	zassert_equal(zdsp_cfft_init_q15(&inst_q15, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zassert_equal(zdsp_cfft_init_f32(&inst_f32, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);

	zdsp_cfft_q15(&inst_q15, qbuf, 0, 1);
	zdsp_cfft_f32(&inst_f32, fbuf, 0, 1);

	for (size_t i = 0; i < len; i++) {
		qout[i] = (float32_t)qbuf[i] / 32768.0f;
		ref[i] = fbuf[i] / (float32_t)fft_len;
	}

	zassert_true(test_snr_error_f32(len, qout, ref, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(qbuf);
	free(fbuf);
	free(qout);
	free(ref);
}

#define DEFINE_CFFT_Q15_TESTS(len)                                                                 \
	ZTEST(transform_cfft_q15, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_cfft_q15_dc(len);                                                        \
	}                                                                                          \
	ZTEST(transform_cfft_q15, test_tone_##len)                                                 \
	{                                                                                          \
		test_zdsp_cfft_q15_tone(len);                                                      \
	}                                                                                          \
	ZTEST(transform_cfft_q15, test_vs_f32_##len)                                               \
	{                                                                                          \
		test_zdsp_cfft_q15_vs_f32(len);                                                    \
	}

DEFINE_CFFT_Q15_TESTS(16)
DEFINE_CFFT_Q15_TESTS(32)
DEFINE_CFFT_Q15_TESTS(64)
DEFINE_CFFT_Q15_TESTS(128)

ZTEST_SUITE(transform_cfft_q15, NULL, NULL, NULL, NULL, NULL);
