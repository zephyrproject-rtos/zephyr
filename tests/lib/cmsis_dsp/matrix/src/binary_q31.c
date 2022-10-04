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

#include "binary_q31.pat"

#define SNR_ERROR_THRESH	((float32_t)100)
#define ABS_ERROR_THRESH_Q31	((q31_t)5)
#define ABS_ERROR_THRESH_Q63	((q63_t)(1 << 16))

#define NUM_MATRICES		(ARRAY_SIZE(in_dims) / 3)
#define MAX_MATRIX_DIM		(40)

#define OP2_MULT		(0)
#define OP2C_CMPLX_MULT		(0)

static void test_op2(int op, const q31_t *input1, const q31_t *input2,
	const q31_t *ref, size_t length)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	q31_t *tmp1, *tmp2, *output;
	uint16_t rows, internal, columns;
	arm_status status;

	arm_matrix_instance_q31 mat_in1;
	arm_matrix_instance_q31 mat_in2;
	arm_matrix_instance_q31 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q31_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q31_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	mat_in1.pData = tmp1;
	mat_in2.pData = tmp2;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < NUM_MATRICES; index++) {
		rows = *dims++;
		internal = *dims++;
		columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = rows;
		mat_in1.numCols = internal;

		mat_in2.numRows = internal;
		mat_in2.numCols = columns;

		mat_out.numRows = rows;
		mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, input1,
		       rows * internal * sizeof(q31_t));

		memcpy(mat_in2.pData, input2,
		       internal * columns * sizeof(q31_t));

		/* Run test function */
		switch (op) {
		case OP2_MULT:
			status = arm_mat_mult_q31(&mat_in1, &mat_in2,
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
		test_snr_error_q31(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(length, output, ref, ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

DEFINE_TEST_VARIANT5(
	op2, arm_mat_mult_q31, OP2_MULT,
	in_mult1, in_mult2, ref_mult,
	ARRAY_SIZE(ref_mult));

static void test_op2c(int op, const q31_t *input1, const q31_t *input2,
	const q31_t *ref, size_t length)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	q31_t *tmp1, *tmp2, *output;
	uint16_t rows, internal, columns;
	arm_status status;

	arm_matrix_instance_q31 mat_in1;
	arm_matrix_instance_q31 mat_in2;
	arm_matrix_instance_q31 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(2 * MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q31_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	tmp2 = malloc(2 * MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q31_t));
	zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(2 * length * sizeof(q31_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Initialise contexts */
	mat_in1.pData = tmp1;
	mat_in2.pData = tmp2;
	mat_out.pData = output;

	/* Iterate matrices */
	for (index = 0; index < NUM_MATRICES; index++) {
		rows = *dims++;
		internal = *dims++;
		columns = *dims++;

		/* Initialise matrix dimensions */
		mat_in1.numRows = rows;
		mat_in1.numCols = internal;

		mat_in2.numRows = internal;
		mat_in2.numCols = columns;

		mat_out.numRows = rows;
		mat_out.numCols = columns;

		/* Load matrix data */
		memcpy(mat_in1.pData, input1,
		       2 * rows * internal * sizeof(q31_t));

		memcpy(mat_in2.pData, input2,
		       2 * internal * columns * sizeof(q31_t));

		/* Run test function */
		switch (op) {
		case OP2C_CMPLX_MULT:
			status = arm_mat_cmplx_mult_q31(&mat_in1, &mat_in2,
							&mat_out);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Validate status */
		zassert_equal(status, ARM_MATH_SUCCESS,
			      ASSERT_MSG_INCORRECT_COMP_RESULT);

		/* Increment output pointer */
		mat_out.pData += (2 * rows * columns);
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q31(2 * length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q31(2 * length, output, ref,
			ABS_ERROR_THRESH_Q31),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(tmp2);
	free(output);
}

DEFINE_TEST_VARIANT5(
	op2c, arm_mat_cmplx_mult_q31, OP2C_CMPLX_MULT,
	in_cmplx_mult1, in_cmplx_mult2, ref_cmplx_mult,
	ARRAY_SIZE(ref_cmplx_mult) / 2);

void test_matrix_binary_q31(void)
{
	ztest_test_suite(matrix_binary_q31,
		ztest_unit_test(test_op2_arm_mat_mult_q31),
		ztest_unit_test(test_op2c_arm_mat_cmplx_mult_q31)
		);

	ztest_run_test_suite(matrix_binary_q31);
}
