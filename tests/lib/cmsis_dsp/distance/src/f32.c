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

#include "f32.pat"

#define ABS_ERROR_THRESH	(1e-3)

#define DIMS_IN			(dims[0])
#define DIMS_VEC		(dims[1])

#define OP_BRAYCURTIS		(0)
#define OP_CANBERRA		(1)
#define OP_CHEBYSHEV		(2)
#define OP_CITYBLOCK		(3)
#define OP_CORRELATION		(4)
#define OP_COSINE		(5)
#define OP_EUCLIDEAN		(6)
#define OP_JENSENSHANNON	(7)
#define OP_MINKOWSKI		(8)

ZTEST_SUITE(distance_f32, NULL, NULL, NULL, NULL, NULL);

static void test_arm_distance(int op, bool scratchy, const uint16_t *dims,
	const uint32_t *dinput1, const uint32_t *dinput2, const uint32_t *ref)
{
	size_t index;
	const size_t length = DIMS_IN;
	const float32_t *input1 = (const float32_t *)dinput1;
	const float32_t *input2 = (const float32_t *)dinput2;
	float32_t *output, *tmp1 = NULL, *tmp2 = NULL;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Allocate scratch buffers */
	if (scratchy) {
		tmp1 = malloc(DIMS_VEC * sizeof(float32_t));
		zassert_not_null(tmp1, ASSERT_MSG_BUFFER_ALLOC_FAILED);

		tmp2 = malloc(DIMS_VEC * sizeof(float32_t));
		zassert_not_null(tmp2, ASSERT_MSG_BUFFER_ALLOC_FAILED);
	}

	/* Enumerate input */
	for (index = 0; index < length; index++) {
		float32_t val;

		/* Load input values into the scratch buffers */
		if (scratchy) {
			memcpy(tmp1, input1, DIMS_VEC * sizeof(float32_t));
			memcpy(tmp2, input2, DIMS_VEC * sizeof(float32_t));
		}

		/* Run test function */
		switch (op) {
		case OP_BRAYCURTIS:
			val = arm_braycurtis_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_CANBERRA:
			val = arm_canberra_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_CHEBYSHEV:
			val = arm_chebyshev_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_CITYBLOCK:
			val = arm_cityblock_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_CORRELATION:
			val = arm_correlation_distance_f32(
					tmp1, tmp2, DIMS_VEC);
			break;
		case OP_COSINE:
			val = arm_cosine_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_EUCLIDEAN:
			val = arm_euclidean_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		case OP_JENSENSHANNON:
			val = arm_jensenshannon_distance_f32(
					input1, input2, DIMS_VEC);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Store output value */
		output[index] = val;

		/* Increment pointers */
		input1 += DIMS_VEC;
		input2 += DIMS_VEC;
	}

	/* Validate output */
	zassert_true(
		test_near_equal_f32(
			length, output, (float32_t *)ref, ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(output);
	free(tmp1);
	free(tmp2);
}

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, braycurtis, OP_BRAYCURTIS, false, in_dims,
	in_com1, in_com2, ref_braycurtis);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, canberra, OP_CANBERRA, false, in_dims,
	in_com1, in_com2, ref_canberra);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, chebyshev, OP_CHEBYSHEV, false, in_dims,
	in_com1, in_com2, ref_chebyshev);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, cityblock, OP_CITYBLOCK, false, in_dims,
	in_com1, in_com2, ref_cityblock);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, correlation, OP_CORRELATION, true, in_dims,
	in_com1, in_com2, ref_correlation);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, cosine, OP_COSINE, false, in_dims,
	in_com1, in_com2, ref_cosine);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, euclidean, OP_EUCLIDEAN, false, in_dims,
	in_com1, in_com2, ref_euclidean);

DEFINE_TEST_VARIANT6(distance_f32,
	arm_distance, jensenshannon, OP_JENSENSHANNON, false, in_dims,
	in_jen1, in_jen2, ref_jensenshannon);

ZTEST(distance_f32, test_arm_distance_minkowski)
{
	size_t index;
	const size_t length = in_dims_minkowski[0];
	const size_t dims_vec = in_dims_minkowski[1];
	const uint16_t *dims = in_dims_minkowski + 2;
	const float32_t *input1 = (const float32_t *)in_com1;
	const float32_t *input2 = (const float32_t *)in_com2;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Enumerate input */
	for (index = 0; index < length; index++) {
		/* Run test function */
		output[index] =
			arm_minkowski_distance_f32(
				input1, input2, dims[index], dims_vec);

		/* Increment pointers */
		input1 += dims_vec;
		input2 += dims_vec;
	}

	/* Validate output */
	zassert_true(
		test_near_equal_f32(length, output, (float32_t *)ref_minkowski,
			ABS_ERROR_THRESH),
		ASSERT_MSG_ABS_ERROR_LIMIT_EXCEED);

	/* Free buffers */
	free(output);
}
