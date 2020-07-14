/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "f32.pat"

#define DECLARE_COMMON_VARS(in_dims, in_param) \
	const int16_t *dims = in_dims; \
	const float32_t *params = (const float32_t *)in_param; \
	const int32_t classes[2] = { dims[1], dims[2] }; \
	const uint16_t sample_count = dims[3]; \
	const uint16_t vec_dims = dims[4]; \
	const uint16_t svec_count = dims[5]; \
	const float32_t intercept = \
		params[svec_count + (vec_dims * svec_count)]; \
	const float32_t *svec = params; \
	const float32_t *dual_coeff = params + (vec_dims * svec_count)

#define DECLARE_POLY_VARS() \
	const uint16_t degree = dims[6]; \
	const float32_t coeff0 = \
		params[svec_count + (vec_dims * svec_count) + 1]; \
	const float32_t gamma = \
		params[svec_count + (vec_dims * svec_count) + 2]

#define DECLARE_RBF_VARS() \
	const float32_t gamma = \
		params[svec_count + (vec_dims * svec_count) + 1]

#define DECLARE_SIGMOID_VARS() \
	const float32_t coeff0 = \
		params[svec_count + (vec_dims * svec_count) + 1]; \
	const float32_t gamma = \
		params[svec_count + (vec_dims * svec_count) + 2]

static void test_arm_svm_linear_predict_f32(void)
{
	DECLARE_COMMON_VARS(in_linear_dims, in_linear_param);

	arm_svm_linear_instance_f32 inst;
	size_t index;
	const size_t length = ARRAY_SIZE(ref_linear);
	const float32_t *input = (const float32_t *)in_linear_val;
	int32_t *output, *output_buf;

	/* Initialise instance */
	arm_svm_linear_init_f32(&inst, svec_count, vec_dims,
		intercept, dual_coeff, svec, classes);

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(int32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (index = 0; index < sample_count; index++) {
		/* Run test function */
		arm_svm_linear_predict_f32(&inst, input, output);

		/* Increment pointers */
		input += vec_dims;
		output++;
	}

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output_buf, ref_linear),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output_buf);
}

static void test_arm_svm_polynomial_predict_f32(void)
{
	DECLARE_COMMON_VARS(in_polynomial_dims, in_polynomial_param);
	DECLARE_POLY_VARS();

	arm_svm_polynomial_instance_f32 inst;
	size_t index;
	const size_t length = ARRAY_SIZE(ref_polynomial);
	const float32_t *input = (const float32_t *)in_polynomial_val;
	int32_t *output, *output_buf;

	/* Initialise instance */
	arm_svm_polynomial_init_f32(
		&inst, svec_count, vec_dims,
		intercept, dual_coeff, svec, classes,
		degree, coeff0, gamma);

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(int32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (index = 0; index < sample_count; index++) {
		/* Run test function */
		arm_svm_polynomial_predict_f32(&inst, input, output);

		/* Increment pointers */
		input += vec_dims;
		output++;
	}

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output_buf, ref_polynomial),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output_buf);
}

static void test_arm_svm_rbf_predict_f32(void)
{
	DECLARE_COMMON_VARS(in_rbf_dims, in_rbf_param);
	DECLARE_RBF_VARS();

	arm_svm_rbf_instance_f32 inst;
	size_t index;
	const size_t length = ARRAY_SIZE(ref_rbf);
	const float32_t *input = (const float32_t *)in_rbf_val;
	int32_t *output, *output_buf;

	/* Initialise instance */
	arm_svm_rbf_init_f32(
		&inst, svec_count, vec_dims,
		intercept, dual_coeff, svec, classes, gamma);

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(int32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (index = 0; index < sample_count; index++) {
		/* Run test function */
		arm_svm_rbf_predict_f32(&inst, input, output);

		/* Increment pointers */
		input += vec_dims;
		output++;
	}

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output_buf, ref_rbf),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output_buf);
}

static void test_arm_svm_sigmoid_predict_f32(void)
{
	DECLARE_COMMON_VARS(in_sigmoid_dims, in_sigmoid_param);
	DECLARE_SIGMOID_VARS();

	arm_svm_sigmoid_instance_f32 inst;
	size_t index;
	const size_t length = ARRAY_SIZE(ref_sigmoid);
	const float32_t *input = (const float32_t *)in_sigmoid_val;
	int32_t *output, *output_buf;

	/* Initialise instance */
	arm_svm_sigmoid_init_f32(
		&inst, svec_count, vec_dims,
		intercept, dual_coeff, svec, classes, coeff0, gamma);

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(int32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (index = 0; index < sample_count; index++) {
		/* Run test function */
		arm_svm_sigmoid_predict_f32(&inst, input, output);

		/* Increment pointers */
		input += vec_dims;
		output++;
	}

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output_buf, ref_sigmoid),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output_buf);
}

static void test_arm_svm_oneclass_predict_f32(void)
{
	DECLARE_COMMON_VARS(in_oneclass_dims, in_oneclass_param);
	DECLARE_RBF_VARS();

	arm_svm_rbf_instance_f32 inst;
	size_t index;
	const size_t length = ARRAY_SIZE(ref_oneclass);
	const float32_t *input = (const float32_t *)in_oneclass_val;
	int32_t *output, *output_buf;

	/* Initialise instance */
	arm_svm_rbf_init_f32(
		&inst, svec_count, vec_dims,
		intercept, dual_coeff, svec, classes, gamma);

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(int32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate samples */
	for (index = 0; index < sample_count; index++) {
		/* Run test function */
		arm_svm_rbf_predict_f32(&inst, input, output);

		/* Increment pointers */
		input += vec_dims;
		output++;
	}

	/* Validate output */
	zassert_true(
		test_equal_q31(length, output_buf, ref_oneclass),
		ASSERT_MSG_INCORRECT_COMP_RESULT);

	/* Free output buffer */
	free(output_buf);
}

void test_svm_f32(void)
{
	ztest_test_suite(svm_f32,
		ztest_unit_test(test_arm_svm_linear_predict_f32),
		ztest_unit_test(test_arm_svm_polynomial_predict_f32),
		ztest_unit_test(test_arm_svm_rbf_predict_f32),
		ztest_unit_test(test_arm_svm_sigmoid_predict_f32),
		ztest_unit_test(test_arm_svm_oneclass_predict_f32)
		);

	ztest_run_test_suite(svm_f32);
}
