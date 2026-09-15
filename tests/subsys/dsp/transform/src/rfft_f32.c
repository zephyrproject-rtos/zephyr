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

#define TEST_PI          3.14159265358979323846f
#define SNR_ERROR_THRESH ((float32_t)100)

static void build_signal(float32_t *buf, uint16_t fft_len)
{
	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t t = (float32_t)n / (float32_t)fft_len;

		buf[n] = 0.4f * sinf(2.0f * TEST_PI * 3.0f * t) +
			 0.2f * cosf(2.0f * TEST_PI * 9.0f * t) +
			 0.1f * sinf(2.0f * TEST_PI * 13.0f * t);
	}
}

/*
 * Forward real FFT of a DC (constant) signal: the packed output stores the DC term in
 * element 0 and the Nyquist term in element 1. Only the DC term is non-zero (== fft_len,
 * the float forward transform is unscaled).
 */
static void test_zdsp_rfft_fast_f32_dc(uint16_t fft_len)
{
	struct zdsp_rfft_fast_instance_f32 inst;
	float32_t *in;
	float32_t *out;

	in = malloc(fft_len * sizeof(float32_t));
	out = calloc(fft_len + 2, sizeof(float32_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		in[n] = 1.0f;
	}

	zassert_equal(zdsp_rfft_fast_init_f32(&inst, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_fast_f32(&inst, in, out, 0);

	zassert_within(out[0], (float32_t)fft_len, 1.0e-2f * fft_len, "DC term incorrect");
	zassert_within(out[1], 0.0f, 1.0e-2f * fft_len, "Nyquist term not zero");
	for (uint16_t k = 2; k < fft_len; k++) {
		zassert_within(out[k], 0.0f, 1.0e-2f * fft_len, "non-DC bin not zero");
	}

	free(in);
	free(out);
}

/*
 * Forward transform followed by inverse transform reconstructs the original signal (the
 * float inverse real FFT is normalized by 1/fft_len). The transform functions modify their
 * input buffers, so working copies are used.
 */
static void test_zdsp_rfft_fast_f32_roundtrip(uint16_t fft_len)
{
	struct zdsp_rfft_fast_instance_f32 inst;
	float32_t *sig;
	float32_t *work;
	float32_t *spec;
	float32_t *recon;

	sig = malloc(fft_len * sizeof(float32_t));
	work = malloc(fft_len * sizeof(float32_t));
	spec = calloc(fft_len + 2, sizeof(float32_t));
	recon = malloc(fft_len * sizeof(float32_t));
	zassert_not_null(sig, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(work, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(spec, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(recon, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	build_signal(sig, fft_len);
	memcpy(work, sig, fft_len * sizeof(float32_t));

	zassert_equal(zdsp_rfft_fast_init_f32(&inst, fft_len), ZDSP_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_fast_f32(&inst, work, spec, 0);  /* forward */
	zdsp_rfft_fast_f32(&inst, spec, recon, 1); /* inverse (normalized by 1/fft_len) */

	zassert_true(test_snr_error_f32(fft_len, recon, sig, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(sig);
	free(work);
	free(spec);
	free(recon);
}

#define DEFINE_RFFT_F32_TESTS(len)                                                                 \
	ZTEST(transform_rfft_f32, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_rfft_fast_f32_dc(len);                                                   \
	}                                                                                          \
	ZTEST(transform_rfft_f32, test_roundtrip_##len)                                            \
	{                                                                                          \
		test_zdsp_rfft_fast_f32_roundtrip(len);                                            \
	}

DEFINE_RFFT_F32_TESTS(32)
DEFINE_RFFT_F32_TESTS(64)
DEFINE_RFFT_F32_TESTS(128)
DEFINE_RFFT_F32_TESTS(256)

ZTEST_SUITE(transform_rfft_f32, NULL, NULL, NULL, NULL, NULL);
