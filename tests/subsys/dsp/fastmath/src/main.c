/* Copyright (c) 2026 James Roy <rruuaanng@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/fastmath.h>
#include <zephyr/ztest.h>
#include <string.h>
#include <stdlib.h>

ZTEST(zdsp_fastmath, test_sine_cosine)
{
	float32_t f32 = 0.0f;
	q15_t q15 = 0x0;
	q31_t q31 = 0x0;
	q31_t x = (q31_t)0x0f5c28f0; /* 0.119999997 */

	/* Sine */
	f32 = zdsp_sin_f32(0.119999997f);
	q15 = zdsp_sin_q15(x);
	q31 = zdsp_sin_q31(x);
	zassert_true(abs(f32 - 0.119711f) <= 1e-8f);
	zassert_equal(q15, (q15_t)0x73e0);
	zassert_equal(q31, (q31_t)0x579ed29e);

	/* Cosine */
	f32 = zdsp_cos_f32(0.119999997f);
	q15 = zdsp_cos_q15(x);
	q31 = zdsp_cos_q31(x);
	zassert_true(abs(f32 - 0.992796f) <= 1e-8f);
	zassert_equal(q15, (q15_t)0xffffc9a0);
	zassert_equal(q31, (q31_t)0x5d4e66c0);
}

ZTEST_SUITE(zdsp_fastmath, NULL, NULL, NULL, NULL, NULL);
