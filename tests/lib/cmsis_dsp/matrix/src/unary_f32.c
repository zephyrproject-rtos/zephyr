/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "unary_f32.pat"

#define SNR_ERROR_THRESH	((float32_t)120)
#define REL_ERROR_THRESH	(1.0e-5)
#define ABS_ERROR_THRESH	(1.0e-5)

#define SNR_ERROR_THRESH_INV	((float32_t)67)
#define REL_ERROR_THRESH_INV	(1.0e-3)
#define ABS_ERROR_THRESH_INV	(1.0e-3)

#define SNR_ERROR_THRESH_CHOL	((float32_t)92)
#define REL_ERROR_THRESH_CHOL	(1.0e-5)
#define ABS_ERROR_THRESH_CHOL	(5.0e-4)

#define NUM_MATRICES		(ARRAY_SIZE(in_dims) / 2)
#define MAX_MATRIX_DIM		(40)

#define OP2_ADD			(0)
#define OP2_SUB			(1)
#define OP1_SCALE		(0)
#define OP1_TRANS		(1)
#define OP2V_VEC_MULT		(0)
#define OP1C_CMPLX_TRANS	(0)

static void test_op2(int op, const uint32_t *ref, size_t length)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	float32_t *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_in2;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float32_t));
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
		       rows * columns * sizeof(float32_t));

		memcpy(mat_in2.pData, in_com2,
		       rows * columns * sizeof(float32_t));

		/* Run test function */
		switch (op) {
		case OP2_ADD:
			status = arm_mat_add_f32(&mat_in1, &mat_in2,
						 &mat_out);
			break;
		case OP2_SUB:
			status = arm_mat_sub_f32(&mat_in1, &mat_in2,
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
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

DEFINE_TEST_VARIANT3(op2, arm_mat_add_f32, OP2_ADD,
	ref_add, ARRAY_SIZE(ref_add));
DEFINE_TEST_VARIANT3(op2, arm_mat_sub_f32, OP2_SUB,
	ref_sub, ARRAY_SIZE(ref_sub));

static void test_op1(int op, const uint32_t *ref, size_t length,
	bool transpose)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	float32_t *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float32_t));
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
		       rows * columns * sizeof(float32_t));

		/* Run test function */
		switch (op) {
		case OP1_SCALE:
			status = arm_mat_scale_f32(&mat_in1, 0.5f, &mat_out);
			break;
		case OP1_TRANS:
			status = arm_mat_trans_f32(&mat_in1, &mat_out);
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
		test_snr_error_f32(length, output, (float32_t *)ref,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

DEFINE_TEST_VARIANT4(op1, arm_mat_scale_f32, OP1_SCALE,
	ref_scale, ARRAY_SIZE(ref_scale), false);
DEFINE_TEST_VARIANT4(op1, arm_mat_trans_f32, OP1_TRANS,
	ref_trans, ARRAY_SIZE(ref_trans), true);

static void test_arm_mat_inverse_f32(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_inv);
	uint16_t *dims = (uint16_t *)in_inv_dims;
	float32_t *input, *tmp1, *output;
	arm_status status;
	uint16_t rows, columns;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input = (float32_t *)in_inv;
	mat_in1.pData = tmp1;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < ARRAY_SIZE(in_inv_dims); index++) {
		rows = columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = mat_out.numRows = rows;
		mat_in1.numCols = mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData,
		       input, rows * columns * sizeof(float32_t));

		/* Run test function */
		status = arm_mat_inverse_f32(&mat_in1, &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment pointers */
		input += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_inv,
			SNR_ERROR_THRESH_INV),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref_inv,
			ABS_ERROR_THRESH_INV, REL_ERROR_THRESH_INV),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

static void test_op2v(int op, const uint32_t *ref, size_t length)
{
	size_t index;
	const uint16_t *dims = in_dims;
	float32_t *tmp1, *vec, *output_buf, *output;
	uint16_t rows, internal;

	arm_matrix_instance_f32 mat_in1;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	vec = malloc(2 * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(vec, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	mat_in1.pData = tmp1;
	output = output_buf;

	/* Iterate matrices */
	for (index = 0; index < NUM_MATRICES; index++) {
		rows = *dims++;
		internal = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = rows;
		mat_in1.numCols = internal;

		/* Load matrix data */
		memcpy(mat_in1.pData, in_com1,
		       2 * rows * internal * sizeof(float32_t));
		memcpy(vec, in_vec1, 2 * internal * sizeof(float32_t));

		/* Run test function */
		switch (op) {
		case OP2V_VEC_MULT:
			arm_mat_vec_mult_f32(&mat_in1, vec, output);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Increment output pointer */
		output += rows;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output_buf, (float32_t *)ref,
				   SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output_buf, (float32_t *)ref,
				     ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(vec);
	free(output_buf);
}

DEFINE_TEST_VARIANT3(op2v, arm_mat_vec_mult_f32, OP2V_VEC_MULT,
	ref_vec_mult, ARRAY_SIZE(ref_vec_mult));

static void test_op1c(int op, const uint32_t *ref, size_t length, bool transpose)
{
	size_t index;
	const uint16_t *dims = in_dims;
	float32_t *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(2 * MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(2 * length * sizeof(float32_t));
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
		memcpy(mat_in1.pData,
		       in_cmplx1, 2 * rows * columns * sizeof(float32_t));

		/* Run test function */
		switch (op) {
		case OP1C_CMPLX_TRANS:
			status = arm_mat_cmplx_trans_f32(&mat_in1, &mat_out);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Validate status */
		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		mat_out.pData += 2 * (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(2 * length, output, (float32_t *)ref,
				   SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(2 * length, output, (float32_t *)ref,
				     ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

DEFINE_TEST_VARIANT4(op1c, arm_mat_cmplx_trans_f32, OP1C_CMPLX_TRANS,
	ref_cmplx_trans, ARRAY_SIZE(ref_cmplx_trans) / 2, true);

static void test_arm_mat_cholesky_f32(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_cholesky_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float32_t *input, *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input = (float32_t *)in_cholesky_dpo;
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
		       input, rows * columns * sizeof(float32_t));

		/* Run test function */
		status = arm_mat_cholesky_f32(&mat_in1, &mat_out);

		zassert_equal(status, ARM_MATH_SUCCESS,
			ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment pointers */
		input += (rows * columns);
		mat_out.pData += (rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_f32(length, output, (float32_t *)ref_cholesky_dpo,
			SNR_ERROR_THRESH_CHOL),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output, (float32_t *)ref_cholesky_dpo,
			ABS_ERROR_THRESH_CHOL, REL_ERROR_THRESH_CHOL),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

static void test_arm_mat_solve_upper_triangular_f32(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_uptriangular_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float32_t *input1, *input2, *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_in2;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input1 = (float32_t *)in_uptriangular_dpo;
	input2 = (float32_t *)in_rnda_dpo;
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
		       rows * columns * sizeof(float32_t));

		memcpy(mat_in2.pData, input2,
		       rows * columns * sizeof(float32_t));

		/* Run test function */
		status = arm_mat_solve_upper_triangular_f32(&mat_in1, &mat_in2,
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
		test_snr_error_f32(length, output,
			(float32_t *)ref_uptriangular_dpo,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output,
			(float32_t *)ref_uptriangular_dpo,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

static void test_arm_mat_solve_lower_triangular_f32(void)
{
	size_t index;
	size_t length = ARRAY_SIZE(ref_lotriangular_dpo);
	const uint16_t *dims = in_cholesky_dpo_dims;
	float32_t *input1, *input2, *tmp1, *tmp2, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_f32 mat_in1;
	arm_matrix_instance_f32 mat_in2;
	arm_matrix_instance_f32 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(float32_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = calloc(length, sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	input1 = (float32_t *)in_lotriangular_dpo;
	input2 = (float32_t *)in_rnda_dpo;
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
		       rows * columns * sizeof(float32_t));

		memcpy(mat_in2.pData, input2,
		       rows * columns * sizeof(float32_t));

		/* Run test function */
		status = arm_mat_solve_lower_triangular_f32(&mat_in1, &mat_in2,
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
		test_snr_error_f32(length, output,
			(float32_t *)ref_lotriangular_dpo,
			SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f32(length, output,
			(float32_t *)ref_lotriangular_dpo,
			ABS_ERROR_THRESH, REL_ERROR_THRESH),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

/*
 * NOTE: arm_mat_ldlt_f32 tests are not implemented for now because they
 *       require on-device test pattern generation which defeats the purpose
 *       of on-device testing. Add these tests when the upstream testsuite is
 *       updated to use pre-generated test patterns.
 */

void test_matrix_unary_f32(void)
{
	ztest_test_suite(matrix_unary_f32,
		ztest_unit_test(test_op2_arm_mat_add_f32),
		ztest_unit_test(test_op2_arm_mat_sub_f32),
		ztest_unit_test(test_op1_arm_mat_scale_f32),
		ztest_unit_test(test_op1_arm_mat_trans_f32),
		ztest_unit_test(test_arm_mat_inverse_f32),
		ztest_unit_test(test_op2v_arm_mat_vec_mult_f32),
		ztest_unit_test(test_op1c_arm_mat_cmplx_trans_f32),
		ztest_unit_test(test_arm_mat_cholesky_f32),
		ztest_unit_test(test_arm_mat_solve_upper_triangular_f32),
		ztest_unit_test(test_arm_mat_solve_lower_triangular_f32)
		);

	ztest_run_test_suite(matrix_unary_f32);
}
