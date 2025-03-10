/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/test_common.h"

#include "u32.pat"

#define REL_ERROR_THRESH	(1e-8)

#define DIMS_IN			(dims[0])
#define DIMS_VEC		(dims[1])
#define DIMS_BIT_VEC		(dims[2])

#define OP_DICE			(0)
#define OP_HAMMING		(1)
#define OP_JACCARD		(2)
#define OP_KULSINSKI		(3)
#define OP_ROGERSTANIMOTO	(4)
#define OP_RUSSELLRAO		(5)
#define OP_SOKALMICHENER	(6)
#define OP_SOKALSNEATH		(7)
#define OP_YULE			(8)

ZTEST_SUITE(distance_u32, NULL, NULL, NULL, NULL, NULL);

static void test_arm_distance(int op, const uint16_t *dims,
	const uint32_t *input1, const uint32_t *input2, const uint32_t *ref)
{
	size_t index;
	const size_t length = DIMS_IN;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(length * sizeof(float32_t));
	zassert_not_null(output, ASSERT_MSG_BUFFER_ALLOC_FAILED);

	/* Enumerate input */
	for (index = 0; index < length; index++) {
		float32_t val = 0.0f;

		/* Run test function */
		switch (op) {
		case OP_DICE:
			val = arm_dice_distance(input1, input2, DIMS_VEC);
			break;
		case OP_HAMMING:
			val = arm_hamming_distance(input1, input2, DIMS_VEC);
			break;
		case OP_JACCARD:
			val = arm_jaccard_distance(input1, input2, DIMS_VEC);
			break;
		case OP_KULSINSKI:
			val = arm_kulsinski_distance(input1, input2, DIMS_VEC);
			break;
		case OP_ROGERSTANIMOTO:
			val = arm_rogerstanimoto_distance(
					input1, input2, DIMS_VEC);
			break;
		case OP_RUSSELLRAO:
			val = arm_russellrao_distance(
					input1, input2, DIMS_VEC);
			break;
		case OP_SOKALMICHENER:
			val = arm_sokalmichener_distance(
					input1, input2, DIMS_VEC);
			break;
		case OP_SOKALSNEATH:
			val = arm_sokalsneath_distance(
					input1, input2, DIMS_VEC);
			break;
		case OP_YULE:
			val = arm_yule_distance(input1, input2, DIMS_VEC);
			break;
		default:
			zassert_unreachable("invalid operation");
		}

		/* Store output value */
		output[index] = val;

		/* Increment pointers */
		input1 += DIMS_BIT_VEC;
		input2 += DIMS_BIT_VEC;
	}

	/* Validate output */
	zassert_true(
		test_rel_error_f32(length, output, (float32_t *)ref,
			REL_ERROR_THRESH),
		ASSERT_MSG_REL_ERROR_LIMIT_EXCEED);

	/* Free output buffer */
	free(output);
}

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, dice, OP_DICE, in_dims,
	in_com1, in_com2, ref_dice);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, hamming, OP_HAMMING, in_dims,
	in_com1, in_com2, ref_hamming);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, jaccard, OP_JACCARD, in_dims,
	in_com1, in_com2, ref_jaccard);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, kulsinski, OP_KULSINSKI, in_dims,
	in_com1, in_com2, ref_kulsinski);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, rogerstanimoto, OP_ROGERSTANIMOTO, in_dims,
	in_com1, in_com2, ref_rogerstanimoto);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, russellrao, OP_RUSSELLRAO, in_dims,
	in_com1, in_com2, ref_russellrao);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, sokalmichener, OP_SOKALMICHENER, in_dims,
	in_com1, in_com2, ref_sokalmichener);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, sokalsneath, OP_SOKALSNEATH, in_dims,
	in_com1, in_com2, ref_sokalsneath);

DEFINE_TEST_VARIANT5(distance_u32,
	arm_distance, yule, OP_YULE, in_dims,
	in_com1, in_com2, ref_yule);
