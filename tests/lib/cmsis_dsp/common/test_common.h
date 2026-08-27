/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_LIB_CMSIS_DSP_COMMON_TEST_COMMON_H_
#define ZEPHYR_TESTS_LIB_CMSIS_DSP_COMMON_TEST_COMMON_H_

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/test_toolchain.h>
#include <stdlib.h>
#include <arm_math.h>
#ifdef CONFIG_CMSIS_DSP_FLOAT16
#include <arm_math_f16.h>
#endif

#include "math_helper.h"

#define ASSERT_MSG_BUFFER_ALLOC_FAILED		"buffer allocation failed"
#define ASSERT_MSG_SNR_LIMIT_EXCEED		"signal-to-noise ratio " \
						"error limit exceeded"
#define ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED	"absolute error limit exceeded"
#define ASSERT_MSG_REL_ERROR_LIMIT_EXCEED	"relative error limit exceeded"
#define ASSERT_MSG_ERROR_LIMIT_EXCEED		"error limit exceeded"
#define ASSERT_MSG_INCORRECT_COMP_RESULT	"incorrect computation result"

#define DEFINE_TEST_VARIANT1(suite, name, variant, a1)                                             \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1);                                                                   \
	}

#define DEFINE_TEST_VARIANT2(suite, name, variant, a1, a2)                                         \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2);                                                               \
	}

#define DEFINE_TEST_VARIANT3(suite, name, variant, a1, a2, a3)                                     \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2, a3);                                                           \
	}

#define DEFINE_TEST_VARIANT4(suite, name, variant, a1, a2, a3, a4)                                 \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2, a3, a4);                                                       \
	}

#define DEFINE_TEST_VARIANT5(suite, name, variant, a1, a2, a3, a4, a5)                             \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2, a3, a4, a5);                                                   \
	}

#define DEFINE_TEST_VARIANT6(suite, name, variant, a1, a2, a3, a4, a5, a6)                         \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2, a3, a4, a5, a6);                                               \
	}

#define DEFINE_TEST_VARIANT7(suite, name, variant, a1, a2, a3, a4, a5, a6, a7)                     \
	ZTEST(suite, test_##name##_##variant)                                                      \
	{                                                                                          \
		test_##name(a1, a2, a3, a4, a5, a6, a7);                                           \
	}

TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_UNUSED_FUNCTION)

static inline bool test_equal_f64(
	size_t length, const float64_t *a, const float64_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

static inline bool test_equal_f32(
	size_t length, const float32_t *a, const float32_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

#ifdef CONFIG_CMSIS_DSP_FLOAT16
static inline bool test_equal_f16(
	size_t length, const float16_t *a, const float16_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_CMSIS_DSP_FLOAT16 */

static inline bool test_equal_q63(
	size_t length, const q63_t *a, const q63_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

static inline bool test_equal_q31(
	size_t length, const q31_t *a, const q31_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

static inline bool test_equal_q15(
	size_t length, const q15_t *a, const q15_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

static inline bool test_equal_q7(
	size_t length, const q7_t *a, const q7_t *b)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (a[index] != b[index]) {
			return false;
		}
	}

	return true;
}

static inline bool test_near_equal_f64(
	size_t length, const float64_t *a, const float64_t *b,
	float64_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabs(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

static inline bool test_near_equal_f32(
	size_t length, const float32_t *a, const float32_t *b,
	float32_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabsf(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

#ifdef CONFIG_CMSIS_DSP_FLOAT16
static inline bool test_near_equal_f16(
	size_t length, const float16_t *a, const float16_t *b,
	float16_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabsf((float)a[index] - (float)b[index]) > (float)threshold) {
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_CMSIS_DSP_FLOAT16 */

static inline bool test_near_equal_q63(
	size_t length, const q63_t *a, const q63_t *b, q63_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (llabs(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

static inline bool test_near_equal_q31(
	size_t length, const q31_t *a, const q31_t *b, q31_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (abs(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

static inline bool test_near_equal_q15(
	size_t length, const q15_t *a, const q15_t *b, q15_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (abs(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

static inline bool test_near_equal_q7(
	size_t length, const q7_t *a, const q7_t *b, q7_t threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (abs(a[index] - b[index]) > threshold) {
			return false;
		}
	}

	return true;
}

static inline bool test_rel_error_f64(
	size_t length, const float64_t *a, const float64_t *b,
	float64_t threshold)
{
	size_t index;
	float64_t rel, delta, average;

	for (index = 0; index < length; index++) {
		delta = fabs(a[index] - b[index]);
		average = (fabs(a[index]) + fabs(b[index])) / 2.0;

		if (average != 0) {
			rel = delta / average;

			if (rel > threshold) {
				return false;
			}
		}
	}

	return true;
}

static inline bool test_rel_error_f32(
	size_t length, const float32_t *a, const float32_t *b,
	float32_t threshold)
{
	size_t index;
	float32_t rel, delta, average;

	for (index = 0; index < length; index++) {
		delta = fabsf(a[index] - b[index]);
		average = (fabsf(a[index]) + fabsf(b[index])) / 2.0f;

		if (average != 0) {
			rel = delta / average;

			if (rel > threshold) {
				return false;
			}
		}
	}

	return true;
}

#ifdef CONFIG_CMSIS_DSP_FLOAT16
static inline bool test_rel_error_f16(
	size_t length, const float16_t *a, const float16_t *b,
	float16_t threshold)
{
	size_t index;
	float32_t rel, delta, average;

	for (index = 0; index < length; index++) {
		delta = fabsf((float)a[index] - (float)b[index]);
		average = (fabsf((float)a[index]) + fabsf((float)b[index])) / 2.0f;

		if (average != 0) {
			rel = delta / average;

			if (rel > threshold) {
				return false;
			}
		}
	}

	return true;
}
#endif /* CONFIG_CMSIS_DSP_FLOAT16 */

static inline bool test_close_error_f64(
	size_t length, const float64_t *ref, const float64_t *val,
	float64_t abs_threshold, float64_t rel_threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabs(val[index] - ref[index]) >
			(abs_threshold + rel_threshold * fabs(ref[index]))) {
			return false;
		}
	}

	return true;
}

static inline bool test_close_error_f32(
	size_t length, const float32_t *ref, const float32_t *val,
	float32_t abs_threshold, float32_t rel_threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabsf(val[index] - ref[index]) >
			(abs_threshold + rel_threshold * fabsf(ref[index]))) {
			return false;
		}
	}

	return true;
}

#ifdef CONFIG_CMSIS_DSP_FLOAT16
static inline bool test_close_error_f16(
	size_t length, const float16_t *ref, const float16_t *val,
	float32_t abs_threshold, float32_t rel_threshold)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (fabsf((float)val[index] - (float)ref[index]) >
			(abs_threshold + rel_threshold * fabsf((float)ref[index]))) {
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_CMSIS_DSP_FLOAT16 */

static inline bool test_snr_error_f64(
	size_t length, const float64_t *a, const float64_t *b,
	float64_t threshold)
{
	float64_t snr;

	snr = arm_snr_f64(a, b, length);
	return (snr >= threshold);
}

static inline bool test_snr_error_f32(
	size_t length, const float32_t *a, const float32_t *b,
	float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_f32(a, b, length);
	return (snr >= threshold);
}

#ifdef CONFIG_CMSIS_DSP_FLOAT16
static inline bool test_snr_error_f16(
	size_t length, const float16_t *a, const float16_t *b,
	float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_f16(a, b, length);
	return (snr >= threshold);
}
#endif /* CONFIG_CMSIS_DSP_FLOAT16 */

static inline bool test_snr_error_q63(
	size_t length, const q63_t *a, const q63_t *b, float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_q63(a, b, length);
	return (snr >= threshold);
}

static inline bool test_snr_error_q31(
	size_t length, const q31_t *a, const q31_t *b, float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_q31(a, b, length);
	return (snr >= threshold);
}

static inline bool test_snr_error_q15(
	size_t length, const q15_t *a, const q15_t *b, float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_q15(a, b, length);
	return (snr >= threshold);
}

static inline bool test_snr_error_q7(
	size_t length, const q7_t *a, const q7_t *b, float32_t threshold)
{
	float32_t snr;

	snr = arm_snr_q7(a, b, length);
	return (snr >= threshold);
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_UNUSED_FUNCTION)

#endif /* ZEPHYR_TESTS_LIB_CMSIS_DSP_COMMON_TEST_COMMON_H_ */
