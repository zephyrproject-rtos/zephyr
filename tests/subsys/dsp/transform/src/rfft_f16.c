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

static void build_signal(float16_t *buf, uint16_t fft_len)
{
	for (uint16_t n = 0; n < fft_len; n++) {
		float32_t t = (float32_t)n / (float32_t)fft_len;

		buf[n] = (float16_t)(0.4f * sinf(2.0f * TEST_PI * 3.0f * t) +
				     0.2f * cosf(2.0f * TEST_PI * 9.0f * t) +
				     0.1f * sinf(2.0f * TEST_PI * 13.0f * t));
	}
}

/*
 * Forward real FFT of a DC (constant) signal: the packed output stores the DC term in
 * element 0 and the Nyquist term in element 1. Only the DC term is non-zero (== fft_len,
 * the float forward transform is unscaled).
 */
static void test_zdsp_rfft_fast_f16_dc(uint16_t fft_len)
{
	struct zdsp_rfft_fast_instance_f16 inst;
	float16_t *in;
	float16_t *out;

	in = malloc(fft_len * sizeof(float16_t));
	out = calloc(fft_len + 2, sizeof(float16_t));
	zassert_not_null(in, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(out, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	for (uint16_t n = 0; n < fft_len; n++) {
		in[n] = (float16_t)1.0f;
	}

	zassert_equal(zdsp_rfft_fast_init_f16(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_fast_f16(&inst, in, out, 0);

	zassert_within((float32_t)out[0], (float32_t)fft_len, BIN_ABS_THRESH(fft_len),
		       "DC term incorrect");
	zassert_within((float32_t)out[1], 0.0f, BIN_ABS_THRESH(fft_len), "Nyquist term not zero");
	for (uint16_t k = 2; k < fft_len; k++) {
		zassert_within((float32_t)out[k], 0.0f, BIN_ABS_THRESH(fft_len),
			       "non-DC bin not zero");
	}

	free(in);
	free(out);
}

/*
 * Forward transform followed by inverse transform reconstructs the original signal (the
 * float inverse real FFT is normalized by 1/fft_len). The transform functions modify their
 * input buffers, so working copies are used.
 */
static void test_zdsp_rfft_fast_f16_roundtrip(uint16_t fft_len)
{
	struct zdsp_rfft_fast_instance_f16 inst;
	float16_t *sig;
	float16_t *work;
	float16_t *spec;
	float16_t *recon;

	sig = malloc(fft_len * sizeof(float16_t));
	work = malloc(fft_len * sizeof(float16_t));
	spec = calloc(fft_len + 2, sizeof(float16_t));
	recon = malloc(fft_len * sizeof(float16_t));
	zassert_not_null(sig, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(work, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(spec, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	zassert_not_null(recon, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	build_signal(sig, fft_len);
	memcpy(work, sig, fft_len * sizeof(float16_t));

	zassert_equal(zdsp_rfft_fast_init_f16(&inst, fft_len), ZDSP_TRANSFORM_STATUS_OK,
		      ASSERT_MSG_INCORRECT_COMP_RESULT);
	zdsp_rfft_fast_f16(&inst, work, spec, 0);  /* forward */
	zdsp_rfft_fast_f16(&inst, spec, recon, 1); /* inverse (normalized by 1/fft_len) */

	zassert_true(test_snr_error_f16(fft_len, recon, sig, SNR_ERROR_THRESH),
		     ASSERT_MSG_SNR_LIMIT_EXCEED);

	free(sig);
	free(work);
	free(spec);
	free(recon);
}

#define DEFINE_RFFT_F16_TESTS(len)                                                                 \
	ZTEST(transform_rfft_f16, test_dc_##len)                                                   \
	{                                                                                          \
		test_zdsp_rfft_fast_f16_dc(len);                                                   \
	}                                                                                          \
	ZTEST(transform_rfft_f16, test_roundtrip_##len)                                            \
	{                                                                                          \
		test_zdsp_rfft_fast_f16_roundtrip(len);                                            \
	}

DEFINE_RFFT_F16_TESTS(32)
DEFINE_RFFT_F16_TESTS(64)
DEFINE_RFFT_F16_TESTS(128)
DEFINE_RFFT_F16_TESTS(256)

ZTEST_SUITE(transform_rfft_f16, NULL, NULL, NULL, NULL, NULL);
