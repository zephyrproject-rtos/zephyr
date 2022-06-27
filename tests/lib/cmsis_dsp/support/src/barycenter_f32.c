/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "barycenter_f32.pat"

#define ABS_ERROR_THRESH	(1e-3)

void test_arm_barycenter_f32(void)
{
	int test_index;
	const size_t length = ARRAY_SIZE(ref_barycenter);
	const uint16_t test_count = in_barycenter_dims[0];
	const uint16_t *dims = in_barycenter_dims + 1;
	const float32_t *input_val = (const float32_t *)in_barycenter_val;
	const float32_t *input_coeff = (const float32_t *)in_barycenter_coeff;
	float32_t *output, *output_buf;

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(float32_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate tests */
	for (test_index = 0; test_index < test_count; test_index++) {
		const uint16_t vec_count = dims[0];
		const uint16_t vec_length = dims[1];

		/* Run test function */
		arm_barycenter_f32(
			input_val, input_coeff, output, vec_count, vec_length);

		/* Increment pointers */
		input_val += vec_count * vec_length;
		input_coeff += vec_count;
		output += vec_length;
		dims += 2;
	}

	/* Validate output */
	zassert_true(
		test_near_equal_f32(
			length, output_buf, (float32_t *)ref_barycenter,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output_buf);
}

void test_support_barycenter_f32(void)
{
	ztest_test_suite(support_barycenter_f32,
		ztest_unit_test(test_arm_barycenter_f32)
		);

	ztest_run_test_suite(support_barycenter_f32);
}
