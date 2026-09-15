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
#include "common/test_common.h"

#define TEST_PI                 3.14159265358979323846f
/*
 * Half precision carries an 11 bit significand, so the round trip is checked against a much
 * looser SNR threshold than the f32 transform. A correct transform stays far above this; a
 * broken one (wrong scaling/ordering) collapses to near 0 dB.
 */
#define SNR_ERROR_THRESH        ((float32_t)30)
/* Absolute tolerance on a spectrum whose bins are bounded by fft_len. */
#define BIN_ABS_THRESH(fft_len) (5.0e-2f * (float32_t)(fft_len))

/* Build a deterministic, bounded interleaved complex signal of @p fft_len samples. */
static void build_signal(float16_t *buf, uint16_t fft_len)
{
	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t t = (float32_t)n / (float32_t)fft_len;

		buf[2 * n] = (float16_t)(0.4f * sinf(2.0f * TEST_PI * 3.0f * t) +
					 0.2f * cosf(2.0f * TEST_PI * 7.0f * t));
		buf[2 * n + 1] = (float16_t)(0.3f * sinf(2.0f * TEST_PI * 5.0f * t));
	}
}

/*
 * Forward FFT of a DC (constant real) signal must place the whole signal energy in bin 0
 * (== fft_len, since the forward float transform is unscaled) and leave all other bins at
 * zero.
 */
static void test_zdsp_cfft_f16_dc(uint16_t fft_len)
{
	struct zdsp_cfft_instance_f16 inst;
	float16_t *buf;

	buf = malloc(2 * fft_len * sizeof(float16_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		buf[2 * n] = (float16_t)1.0f;
		buf[2 * n + 1] = (float16_t)0.0f;
	}

	zassert_equal(zdsp_cfft_init_f16(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_f16(&inst, buf, 0, 1);

	zassert_within((float32_t)buf[0], (float32_t)fft_len, BIN_ABS_THRESH(fft_len),
		       "DC bin (real) incorrect");
	zassert_within((float32_t)buf[1], 0.0f, BIN_ABS_THRESH(fft_len), "DC bin (imag) incorrect");
	for (uint16_t k = 1; k < fft_len; k++) {
		zassert_within((float32_t)buf[2 * k], 0.0f, BIN_ABS_THRESH(fft_len),
			       "non-DC bin (real) not zero");
		zassert_within((float32_t)buf[2 * k + 1], 0.0f, BIN_ABS_THRESH(fft_len),
			       "non-DC bin (imag) not zero");
	}

	free(buf);
}

/*
 * A single complex exponential at bin @c k produces a single non-zero spectral bin. The
 * argmax of the magnitude spectrum must therefore be exactly @c k. This check is
 * independent of any scaling.
 */
static void test_zdsp_cfft_f16_tone(uint16_t fft_len)
{
	struct zdsp_cfft_instance_f16 inst;
	const uint16_t k = 3;
	float16_t *buf;
	float32_t max_mag2 = -1.0f;
	uint16_t max_bin = 0;

	buf = malloc(2 * fft_len * sizeof(float16_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t phase = 2.0f * TEST_PI * k * n / fft_len;

		buf[2 * n] = (float16_t)(0.5f * cosf(phase));
		buf[2 * n + 1] = (float16_t)(0.5f * sinf(phase));
	}

	zassert_equal(zdsp_cfft_init_f16(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_f16(&inst, buf, 0, 1);

	for (uint16_t m = 0; m < fft_len; m++) {
		float32_t re = (float32_t)buf[2 * m];
		float32_t im = (float32_t)buf[2 * m + 1];
		float32_t mag2 = re * re + im * im;

		if (mag2 > max_mag2) {
			max_mag2 = mag2;
			max_bin = m;
		}
	}

	zassert_equal(max_bin, k, "spectral peak in wrong bin (got %u, expected %u)", max_bin, k);

	free(buf);
}

/*
 * Forward transform followed by the inverse transform must reconstruct the original
 * signal: the float inverse transform is normalized by 1/fft_len.
 */
static void test_zdsp_cfft_f16_roundtrip(uint16_t fft_len)
{
	struct zdsp_cfft_instance_f16 inst;
	size_t len = 2 * fft_len;
	float16_t *buf;
	float16_t *ref;

	buf = malloc(len * sizeof(float16_t));
	ref = malloc(len * sizeof(float16_t));
	zassert_not_null(buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(ref, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	build_signal(ref, fft_len);
	memcpy(buf, ref, len * sizeof(float16_t));

	zassert_equal(zdsp_cfft_init_f16(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_cfft_f16(&inst, buf, 0, 1); /* forward */
	zdsp_cfft_f16(&inst, buf, 1, 1); /* inverse (normalized by 1/fft_len) */

	zassert_true(test_snr_error_f16(len, buf, ref, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(buf);
	free(ref);
}

#define DEFINE_CFFT_F16_TESTS(len)                                                                 \
	ZTEST(transform_cfft_f16, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_cfft_f16_dc(len);                                                        \
	}                                                                                          \
	ZTEST(transform_cfft_f16, test_tone_##len)                                                 \
	{                                                                                          \
		test_zdsp_cfft_f16_tone(len);                                                      \
	}                                                                                          \
	ZTEST(transform_cfft_f16, test_roundtrip_##len)                                            \
	{                                                                                          \
		test_zdsp_cfft_f16_roundtrip(len);                                                 \
	}

DEFINE_CFFT_F16_TESTS(16)
DEFINE_CFFT_F16_TESTS(32)
DEFINE_CFFT_F16_TESTS(64)
DEFINE_CFFT_F16_TESTS(128)

ZTEST_SUITE(transform_cfft_f16, NULL, NULL, NULL, NULL, NULL);
