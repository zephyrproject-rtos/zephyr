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

#include "unary_q7.pat"

#define SNR_ERROR_THRESH	((float32_t)20)
#define SNR_ERROR_THRESH_LOW	((float32_t)11)
#define ABS_ERROR_THRESH_Q7	((q7_t)2)

#define NUM_MATRICES		(ARRAY_SIZE(in_dims) / 2)
#define MAX_MATRIX_DIM		(47)

#define OP1_TRANS		(1)
#define OP2V_VEC_MULT		(0)

static void test_op1(int op, const q7_t *ref, size_t length, bool transpose)
{
	size_t index;
	uint16_t *dims = (uint16_t *)in_dims;
	q7_t *tmp1, *output;
	uint16_t rows, columns;
	arm_status status;

	arm_matrix_instance_q7 mat_in1;
	arm_matrix_instance_q7 mat_out;

	/* Allocate buffers */
	tmp1 = malloc(MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q7_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = malloc(length * sizeof(q7_t));
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
		memcpy(mat_in1.pData, in_com1, rows * columns * sizeof(q7_t));

		/* Run test function */
		switch (op) {
		case OP1_TRANS:
			status = arm_mat_trans_q7(&mat_in1, &mat_out);
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
		test_snr_error_q7(length, output, ref, SNR_ERROR_THRESH),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

DEFINE_TEST_VARIANT4(matrix_unary_q7,
	op1, arm_mat_trans_q7, OP1_TRANS,
	ref_trans, ARRAY_SIZE(ref_trans), true);

static void test_op2v(int op, const q7_t *ref, size_t length)
{
	size_t index;
	const uint16_t *dims = in_dims;
	q7_t *tmp1, *vec, *output_buf, *output;
	uint16_t rows, internal;

	arm_matrix_instance_q7 mat_in1;

	/* Allocate buffers */
	tmp1 = malloc(2 * MAX_MATRIX_DIM * MAX_MATRIX_DIM * sizeof(q7_t));
	zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	vec = malloc(2 * MAX_MATRIX_DIM * sizeof(q7_t));
	zassert_not_null(vec, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output_buf = malloc(length * sizeof(q7_t));
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
		       2 * rows * internal * sizeof(q7_t));
		memcpy(vec, in_vec1, 2 * internal * sizeof(q7_t));

		/* Run test function */
		switch (op) {
		case OP2V_VEC_MULT:
			arm_mat_vec_mult_q7(&mat_in1, vec, output);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Increment output pointer */
		output += rows;
	}

	/* Validate output */
	zassert_true(
		test_snr_error_q7(length, output_buf, ref, SNR_ERROR_THRESH_LOW),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_near_equal_q7(length, output_buf, ref, ABS_ERROR_THRESH_Q7),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(vec);
	free(output_buf);
}

DEFINE_TEST_VARIANT3(matrix_unary_q7,
	op2v, arm_mat_vec_mult_q7, OP2V_VEC_MULT,
	ref_vec_mult, ARRAY_SIZE(ref_vec_mult));

ZTEST_SUITE(matrix_unary_q7, NULL, NULL, NULL, NULL, NULL);
