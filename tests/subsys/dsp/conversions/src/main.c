/* Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/conversions.h>
#include <zephyr/ztest.h>

#define Q_WITHIN         2
#define Q31_FLOAT_WITHIN ((float)Q_WITHIN / (float)INT32_MAX)
#define Q15_FLOAT_WITHIN ((float)Q_WITHIN / (float)INT16_MAX)
#define Q7_FLOAT_WITHIN  ((float)Q_WITHIN / (float)INT8_MAX)

#define float_multiplier(type) ((INT64_C(1) << (8 * sizeof(type) - 1)) - 1)

#define setup_q_to_float(type, ...)                                                                \
	const float expected[] = {__VA_ARGS__};                                                    \
	type##_t src[ARRAY_SIZE(expected)];                                                        \
	float fdst[ARRAY_SIZE(expected)];                                                          \
	type##_t qdst[ARRAY_SIZE(expected)];                                                       \
	for (int i = 0; i < ARRAY_SIZE(expected); ++i) {                                           \
		src[i] = (type##_t)(expected[i] * (float)float_multiplier(type##_t));              \
	}                                                                                          \
	zdsp_##type##_to_float(src, 0, fdst, ARRAY_SIZE(src));                                     \
	(zdsp_float_to_##type)(expected, 0, qdst, ARRAY_SIZE(src))

ZTEST_SUITE(zdsp_conversions, NULL, NULL, NULL, NULL, NULL);

ZTEST(zdsp_conversions, test_q31_to_float)
{
	setup_q_to_float(q31, 0.120000001f, 0.77f, -0.5f, -0.97f);
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i], fdst[i], Q31_FLOAT_WITHIN);
		zassert_within(src[i], qdst[i], Q_WITHIN);
	}

	/* Set the shift to 2 (assumes q scale is 4) then back to Q format */
	zdsp_q31_to_float(src, 2, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q31(fdst, 2, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] * 4.0f, fdst[i], Q31_FLOAT_WITHIN * 2.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN);
	}

	/* Set the shift to -3 (assumes q scale is 1/8) then back to Q format */
	zdsp_q31_to_float(src, -3, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q31(fdst, -3, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] / 8.0f, fdst[i], Q31_FLOAT_WITHIN * 4.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN);
	}
}

ZTEST(zdsp_conversions, test_q15_to_float)
{
	setup_q_to_float(q15, 0.32f, 0.665f, -0.111f, -0.463f);
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i], fdst[i], Q15_FLOAT_WITHIN);
		zassert_within(src[i], qdst[i], Q_WITHIN);
	}

	/* Set the shift to 2 (assumes q scale is 4) then back to Q format */
	zdsp_q15_to_float(src, 2, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q15(fdst, 2, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] * 4.0f, fdst[i], Q15_FLOAT_WITHIN * 2.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN * 2);
	}

	/* Set the shift to -3 (assumes q scale is 1/8) then back to Q format */
	zdsp_q15_to_float(src, -3, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q15(fdst, -3, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] / 8.0f, fdst[i], Q15_FLOAT_WITHIN * 4.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN * 4);
	}
}

ZTEST(zdsp_conversions, test_q7_to_float)
{
	setup_q_to_float(q7, 0.008f, 0.7384f, -0.5547f, -0.2399f);
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i], fdst[i], Q7_FLOAT_WITHIN);
		zassert_within(src[i], qdst[i], 2);
	}

	/* Set the shift to 2 (assumes q scale is 4) then back to Q format */
	zdsp_q7_to_float(src, 2, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q7(fdst, 2, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] * 4.0f, fdst[i], Q7_FLOAT_WITHIN * 2.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN * 2);
	}

	/* Set the shift to -3 (assumes q scale is 1/8) then back to Q format */
	zdsp_q7_to_float(src, -3, fdst, ARRAY_SIZE(src));
	zdsp_float_to_q7(fdst, -3, qdst, ARRAY_SIZE(src));
	for (int i = 0; i < ARRAY_SIZE(src); ++i) {
		zassert_within(expected[i] / 8.0f, fdst[i], Q7_FLOAT_WITHIN * 4.0f);
		zassert_within(src[i], qdst[i], Q_WITHIN * 4);
	}
}
