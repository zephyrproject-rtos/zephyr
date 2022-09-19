/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math_f16.h>
#include "../../common/test_common.h"

#include "barycenter_f16.pat"

#define ABS_ERROR_THRESH	(1e-3)

ZTEST(support_barycenter_f16, test_arm_barycenter_f16)
{
	int test_index;
	const size_t length = ARRAY_SIZE(ref_barycenter);
	const uint16_t test_count = in_barycenter_dims[0];
	const uint16_t *dims = in_barycenter_dims + 1;
	const float16_t *input_val = (const float16_t *)in_barycenter_val;
	const float16_t *input_coeff = (const float16_t *)in_barycenter_coeff;
	float16_t *output, *output_buf;

	/* Allocate output buffer */
	output_buf = malloc(length * sizeof(float16_t));
	zassert_not_null(output_buf, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	output = output_buf;

	/* Enumerate tests */
	for (test_index = 0; test_index < test_count; test_index++) {
		const uint16_t vec_count = dims[0];
		const uint16_t vec_length = dims[1];

		/* Run test function */
		arm_barycenter_f16(
			input_val, input_coeff, output, vec_count, vec_length);

		/* Increment pointers */
		input_val += vec_count * vec_length;
		input_coeff += vec_count;
		output += vec_length;
		dims += 2;
	}

	/* Validate output */
	zassert_true(
		test_near_equal_f16(
			length, output_buf, (float16_t *)ref_barycenter,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output_buf);
}

ZTEST_SUITE(support_barycenter_f16, NULL, NULL, NULL, NULL, NULL);
