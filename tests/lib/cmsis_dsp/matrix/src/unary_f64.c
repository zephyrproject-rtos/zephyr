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

#include "unary_f64.pat"

#define SNR_ERROR_THRESH_INV	((float32_t)120)
#define REL_ERROR_THRESH_INV	(1.0e-6)
#define ABS_ERROR_THRESH_INV	(1.0e-5)

#define NUM_MATRICES_INV	ARRAY_SIZE(in_inv_dims)
#define MAX_MATRIX_DIM		(40)

static void test_arm_mat_inverse_f64(void)
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
	for (index = 0; index < NUM_MATRICES_INV; index++) {
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
			SNR_ERROR_THRESH_INV),
		ASSERT_MSG_SNR_LIMIT_EXCEED);

	zassert_true(
		test_close_error_f64(length, output, (float64_t *)ref_inv,
			ABS_ERROR_THRESH_INV, REL_ERROR_THRESH_INV),
		ASSERT_MSG_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(tmp1);
	free(output);
}

void test_matrix_unary_f64(void)
{
	ztest_test_suite(matrix_unary_f64,
		ztest_unit_test(test_arm_mat_inverse_f64)
		);

	ztest_run_test_suite(matrix_unary_f64);
}
