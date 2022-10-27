/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "unary_f64.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(1.0e-6)
#define ABS_ERROR_THRESH	(1.0e-5)

#define SNR_ERROR_THRESH_CHOL	((float32_t)270)
#define REL_ERROR_THRESH_CHOL	(1.0e-9)
#define ABS_ERROR_THRESH_CHOL	(1.0e-9)

#define NUM_MATRICES		(ARRAY_SIZE(in_dims) / 2)
#define MAX_MATRIX_DIM		(40)

#define OP2_SUB			(1)
#define OP1_TRANS		(1)

static void test_op2(int op, const uint64_t *ref, size_t length)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	float64_t *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_in2;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	mat_in1.pData = tmp1;
	mat_in2.pData = tmp2;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < NUM_MATRICES; index++) {
		rows = *dims++;
		columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_in2.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_in2.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, in_com1,
		       rows * columns * sizeof(float64_t));

		memcpy(mat_in2.pData, in_com2,
		       rows * columns * sizeof(float64_t));

		/* Run test function */
		switch (op) {
		case OP2_SUB:
			status = arm_mat_sub_f64(&mat_in1, &mat_in2,
						 &mat_out);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Validate status */
		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output, (float64_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output, (float64_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

DEFINE_TEST_VARIANT3(matrix_unary_f64,
	op2, arm_mat_sub_f64, OP2_SUB,
	ref_sub, ARRAY_SIZE(ref_sub));

static void test_op1(int op, const uint64_t *ref, size_t length,
	bool transpose)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	float64_t *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	mat_in1.pData = tmp1;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < NUM_MATRICES; index++) {
		rows = *dims++;
		columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = rows;
		mat_in1.numCols = columns;
		mat_out.numRows = transpose ? columns : rows;
		mat_out.numCols = transpose ? rows : columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, in_com1,
		       rows * columns * sizeof(float64_t));

		/* Run test function */
		switch (op) {
		case OP1_TRANS:
			status = arm_mat_trans_f64(&mat_in1, &mat_out);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Validate status */
		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output, (float64_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output, (float64_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

DEFINE_TEST_VARIANT4(matrix_unary_f64,
	op1, arm_mat_trans_f64, OP1_TRANS,
	ref_trans, ARRAY_SIZE(ref_trans), true);

ZTEST(matrix_unary_f64, test_arm_mat_inverse_f64)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_inv);
	uint16_t *dims = (uint16_t *)in_inv_dims;
	float64_t *input, *tmp1, *output;
	arm_status status;
	uint16_t rows, columns;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input = (float64_t *)in_inv;
	mat_in1.pData = tmp1;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < ARRAY_SIZE(in_inv_dims); index++) {
		rows = columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, input,
		       rows * columns * sizeof(float64_t));

		/* Run test function */
		status = arm_mat_inverse_f64(&mat_in1, &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment pointers */
		input += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output, (float64_t *)ref_inv,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output, (float64_t *)ref_inv,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

ZTEST(matrix_unary_f64, test_arm_mat_cholesky_f64)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_cholesky_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float64_t *input, *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input = (float64_t *)in_cholesky_dpo;
	mat_in1.pData = tmp1;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < ARRAY_SIZE(in_cholesky_dpo_dims); index++) {
		rows = columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData,
		       input, rows * columns * sizeof(float64_t));

		/* Run test function */
		status = arm_mat_cholesky_f64(&mat_in1, &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment pointers */
		input += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output, (float64_t *)ref_cholesky_dpo,
			SNR_ERROR_THRESH_CHOL),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output, (float64_t *)ref_cholesky_dpo,
			ABS_ERROR_THRESH_CHOL, REL_ERROR_THRESH_CHOL),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

ZTEST(matrix_unary_f64, test_arm_mat_solve_upper_triangular_f64)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_uptriangular_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float64_t *input1, *input2, *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_in2;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input1 = (float64_t *)in_uptriangular_dpo;
	input2 = (float64_t *)in_rnda_dpo;
	mat_in1.pData = tmp1;
	mat_in2.pData = tmp2;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < ARRAY_SIZE(in_cholesky_dpo_dims); index++) {
		rows = columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_in2.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_in2.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, input1,
		       rows * columns * sizeof(float64_t));

		memcpy(mat_in2.pData, input2,
		       rows * columns * sizeof(float64_t));

		/* Run test function */
		status = arm_mat_solve_upper_triangular_f64(&mat_in1, &mat_in2,
							    &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		input1 += (rows * columns);
		input2 += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output,
			(float64_t *)ref_uptriangular_dpo,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output,
			(float64_t *)ref_uptriangular_dpo,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

ZTEST(matrix_unary_f64, test_arm_mat_solve_lower_triangular_f64)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_lotriangular_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float64_t *input1, *input2, *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f64 mat_in1;
	arm_matrix_instance_f64 mat_in2;
	arm_matrix_instance_f64 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float64_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float64_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input1 = (float64_t *)in_lotriangular_dpo;
	input2 = (float64_t *)in_rnda_dpo;
	mat_in1.pData = tmp1;
	mat_in2.pData = tmp2;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < ARRAY_SIZE(in_cholesky_dpo_dims); index++) {
		rows = columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_in2.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_in2.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, input1,
		       rows * columns * sizeof(float64_t));

		memcpy(mat_in2.pData, input2,
		       rows * columns * sizeof(float64_t));

		/* Run test function */
		status = arm_mat_solve_lower_triangular_f64(&mat_in1, &mat_in2,
							    &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		input1 += (rows * columns);
		input2 += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f64(length, output,
			(float64_t *)ref_lotriangular_dpo,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output,
			(float64_t *)ref_lotriangular_dpo,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

/*
 * NOTE: arm_mat_ldlt_f64 tests are not implemented for now because they
 *       require on-device test pattern generation which defeats the purpose
 *       of on-device testing. Add these tests when the upstream testsuite is
 *       updated to use pre-generated test patterns.
 */

ZTEST_SUITE(matrix_unary_f64, NULL, NULL, NULL, NULL, NULL);
