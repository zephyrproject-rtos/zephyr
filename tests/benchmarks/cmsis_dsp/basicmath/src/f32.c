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
#include "../../common/benchmark_common.h"

#define PATTERN_LENGTH	(256)

static const uint32_t input1[256] = {
	0xbe7615dd, 0x3e0a5b1e, 0x3e9425e8, 0x3f433216,
	0x3e816b4e, 0xbe143e49, 0x3e877d60, 0xbd29de75,
	0xbebc69ec, 0x3e687466, 0xbe2e1b66, 0x3e0346d2,
	0xbe48345c, 0x3f2f2dc5, 0xbf229368, 0xbdd18caa,
	0xbae856c5, 0xbe779abe, 0xbe6e7711, 0x3caa302e,
	0x3e85375c, 0xbe15bd57, 0xbd92253b, 0xbed2cedf,
	0xbf2cbc1e, 0x3e2a4449, 0xbdd0693a, 0x3e16b249,
	0xbe256ea3, 0x3ed42777, 0x3e895be0, 0x3de3e095,
	0xbee09687, 0xbd84a2da, 0x3d966cc5, 0xbd97ea7a,
	0xbea4b437, 0xbd0cbb5d, 0x3df36637, 0xbe20fa46,
	0x3de5a59f, 0xbedc8418, 0x3ce7901e, 0x3e870ef3,
	0xbeeaed98, 0x3e69fc9d, 0x3bef5be4, 0xbe71aa2c,
	0x3dfc9df4, 0x3e14edd4, 0xbe90e704, 0x3df1ade8,
	0xbecb71a1, 0xbee60066, 0x3d8ed065, 0xbe66f074,
	0x3f1d1d4a, 0x3eb0ee33, 0x3e5b4d66, 0x3e80ac40,
	0xbea71b63, 0xbebfaf5c, 0x3eb7c5e8, 0x3c286fe6,
	0xbe4542ec, 0xbdf4c1cb, 0xbe8a2982, 0x3ebf99e4,
	0xbeb9b986, 0x3e91869e, 0x3e3126c8, 0x3e678642,
	0x3e83bfc5, 0x3e2fb9b5, 0xbe9f1f35, 0xbe412ac1,
	0x3ee470f5, 0xbf1ba9c4, 0x3e8ba6eb, 0x3daf6728,
	0x3e379687, 0x3db5db1c, 0xbe0454cc, 0x3e8ffa21,
	0xbed3eb22, 0xbd8c756b, 0xbd37f46f, 0x3f1acc52,
	0x3e6761ef, 0x3dc8f4c2, 0xbb86a0cb, 0xbed741fd,
	0x3ed334d2, 0x3e591fee, 0x3da630be, 0xbd465007,
	0xbe2e6781, 0x3e354ba2, 0x3ee78e9f, 0x3e38e1fc,
	0xbd5b45af, 0xbdc42924, 0xbe487972, 0x3d42fc8c,
	0x3d39ddea, 0xbf800000, 0x3e47bc81, 0xbe6d3ea6,
	0xbe346a5d, 0x3e4bc4aa, 0x3ece4e6a, 0x3ef730a6,
	0x3d323126, 0xbe1f69b2, 0xbf2c5298, 0x3e851dca,
	0x3dd99be4, 0x3ee352ed, 0x3e300faf, 0x3deb85b2,
	0xbef35a4d, 0xbea2e18e, 0x3e0c1422, 0x3ded684f,
	0xbd9d3a04, 0xbe748c80, 0xbd49d3fc, 0x3eae4b91,
	0xbdd13f56, 0x3ec00687, 0xbe9dd67b, 0xbee078e5,
	0xbea7ecd6, 0xbe31f0fb, 0x3c9fab4c, 0xbf5b1cab,
	0xbe3127e2, 0xbd89aa70, 0x3e7bd318, 0x3c387d43,
	0xbd84e04b, 0x3ecab9d8, 0x3edb926f, 0xbec88560,
	0xbe168183, 0x3e50ee4c, 0x3d15fc44, 0x3df7a4f6,
	0xbbeab7df, 0xbd89602e, 0xbe46f8cf, 0xbca78eee,
	0x3e95f997, 0xbeef9320, 0xbb719415, 0x3ed7afd5,
	0x3e00886c, 0xbd8e2cbe, 0x3da33a7c, 0x3da3200f,
	0xbebf7183, 0x3d365e99, 0xbe09538e, 0xbcf48bc6,
	0xbd51b1b8, 0x3d017f4c, 0x3e2b81bb, 0xbed81367,
	0xbe069cd5, 0x3e0c8740, 0xbd6860f3, 0x3dae7824,
	0x3ea05b45, 0x3ce7c1f1, 0x3ef90c86, 0x3cae676d,
	0xbeb633be, 0x3d91a2ac, 0x3d302b7f, 0x3e0439b3,
	0xbd9b621c, 0x3e988c26, 0xbdaba1d2, 0x3e3893fd,
	0x3ed57561, 0xbeaa3c3b, 0xbd0fd35d, 0xbe971a92,
	0x3f068098, 0x3e97eccf, 0x3d7fb7e7, 0x3eb0e749,
	0x3e861bd2, 0x3db8027d, 0x3e86a3ab, 0xbe9b4617,
	0xbc060bb8, 0x3c973710, 0xbc94e165, 0x3edcc8e4,
	0xbe856ffe, 0x3eae969f, 0x3dc64eb0, 0x3f0d4886,
	0xbb5951f3, 0xbd545f5a, 0x3c8020ec, 0xbec63ba0,
	0xbd226696, 0xbbf74b24, 0x3e789184, 0xbe779dfb,
	0x3e27d468, 0xbe8d34a6, 0xbe5a31aa, 0xbe9347f4,
	0x3ec9bf45, 0x3dec7be4, 0x3e8573e7, 0x3dac8f5e,
	0x3e133275, 0x3efabbd4, 0x3e144acb, 0xbf10beb2,
	0xbe8bc904, 0xbd9e491d, 0x3e83271c, 0xbf625b5d,
	0x3e918f0c, 0xbe09bf4b, 0xbe8f8404, 0xbec1dd10,
	0x3dcc691b, 0x3decd44e, 0x3e52ef40, 0x3dc53970,
	0x3e5e7918, 0xbde1a7f5, 0xbe902c3f, 0x3e185dff,
	0x3f053faf, 0x3de7951b, 0x3e431459, 0xbdfd5fd5,
	0xbf08176e, 0xbf068074, 0x3eb79e7a, 0xbd1d80ed,
	0xbe5a7559, 0x3eb3aa3f, 0xbe8db06c, 0xbd90187e,
	0xbf03caa7, 0xbd92238d, 0xbe7cdc64, 0xbe2ac8aa
	};

static const uint32_t input2[256] = {
	0x3efd7b68, 0xbf3396c6, 0x3ecf0d7f, 0xbce77d0c,
	0xbe8045d0, 0xbea8c5d4, 0x3e6a3aaa, 0xbeb9c2f5,
	0x3ce7f210, 0xbf0f4737, 0xbee71c87, 0xbdcd4ed3,
	0xbda07eb9, 0xbe22d09c, 0x3da60119, 0x3f2e0015,
	0x3cf9d138, 0x3ea7295d, 0x3e958f9e, 0x3ec76134,
	0x3caec34b, 0xbe0fc780, 0xbe34442b, 0x3ea30a9e,
	0xbd2f09ac, 0x3f0436b2, 0x3ea6974d, 0x3be04c86,
	0xbe67da27, 0x3f21e9d0, 0x3dc7b20f, 0x3f110e3f,
	0x3ee11ddc, 0x3ed16699, 0x3ecafd42, 0xbf4465ee,
	0x3e113928, 0xbea7fe43, 0xbf018159, 0xbba151c0,
	0xbd04ad55, 0xbeb940d9, 0x3d8f7268, 0xbe4b929e,
	0xbe3cb12a, 0x3edded21, 0x3ecb11e5, 0x3f24b3ab,
	0x3e5e8c42, 0xbe1606df, 0x3f0ea0db, 0xbe348348,
	0xbdea1459, 0x3e3dd968, 0x3eba6bd5, 0x3efca1b6,
	0x3e19c20c, 0x3ee2fa95, 0xbde65541, 0xbe9755e7,
	0x3d802e15, 0x3e1d2343, 0x3b1fc1ee, 0xbf067900,
	0x3c72e743, 0xbe95ee27, 0xbe8dffe3, 0xbec569ed,
	0xbecfe08d, 0x3e6d545e, 0x3e8b3467, 0x3eb5b621,
	0xbe1c1a0f, 0x3e071a92, 0xbe292f55, 0xbd841e3d,
	0x3eb4d636, 0x3e896277, 0x3f0d088e, 0x3e89c512,
	0xbe66e0d9, 0xbda5c7db, 0xbee26d88, 0xbf67e7cf,
	0x3e7b8eee, 0x3e878f80, 0xbe176f02, 0xbead74b7,
	0x3f40d7b0, 0x3e6684ec, 0xbdcf3d89, 0xbe2b185e,
	0x3ea0851a, 0x3e574528, 0xbf25ae3e, 0xbc1d8c89,
	0x3e563384, 0xbea6d40d, 0xbeac99c0, 0x3f229549,
	0x3e522113, 0xbf2c23b1, 0xbe9d1389, 0x3e4e85f4,
	0x3eb0eb91, 0xbf17b075, 0x3e9fa5d3, 0xbdc541ee,
	0xbe79ceb4, 0xbef0e805, 0x3d78e944, 0xbe84a108,
	0x3f04a4e5, 0xbe0d9ea0, 0xbe8ed7c7, 0xbe380a23,
	0xbde28369, 0x3f01dc75, 0xbef7f6f3, 0x3ea0f201,
	0xbf13519c, 0xbedb0feb, 0x3eac3cf7, 0xbe5f8009,
	0xbd57b158, 0x3e9d8a4e, 0xbe96c82e, 0x3f5bc10d,
	0x3cf6474c, 0x3e4364c3, 0xbeaa2ea4, 0xbd53f6ab,
	0xbf1fdc71, 0x3ee86194, 0xbf0132aa, 0xbee53cc6,
	0x3e82836b, 0xbe9a7a9a, 0xbe494f30, 0xbe24c1c6,
	0x3f76962b, 0x3e11f661, 0xbec6d557, 0xbdb3603e,
	0x3df40af7, 0x3f5229b9, 0x3e576be7, 0x3e5de873,
	0x3ecbb13e, 0x3d9d2b57, 0xbe58e633, 0xbe42c779,
	0x3ea4539f, 0xbec1326d, 0x3e9d7323, 0xbe05b725,
	0x3c814ef6, 0x3d1b483c, 0x3e036f96, 0x3e5d4cca,
	0xbf1511f1, 0x3dd2188e, 0x3e0c87b4, 0x3dd47a1e,
	0x3de1ed5e, 0xbf086942, 0x3f14ab85, 0xbed3a836,
	0x3db26a76, 0x3e7e0bf6, 0x3f59c9f4, 0xbee53819,
	0xbee72aeb, 0xbd08cc78, 0x3dc6bd55, 0x3e0d0cd2,
	0xbf00d3c6, 0x3ec9b7be, 0xbeef4f2a, 0xbe91f0f3,
	0x3e770521, 0xbd1015f5, 0x3f42a1bc, 0xbf31354a,
	0xbe277544, 0xbf21a07d, 0x3d16c40b, 0x3d9a5ef8,
	0xbd375473, 0xbe8a73e0, 0xbf04d275, 0x3e00e10c,
	0xbf04a1c1, 0x3e5ab3ef, 0xbe79fe0e, 0xbf0ff988,
	0xbea9a481, 0xbd7b414f, 0xbdc0561a, 0xbee876f9,
	0x3e96901f, 0xbf1922b3, 0xbe4696f0, 0xbec91e95,
	0xbcecd009, 0x3e27120e, 0x3d51f700, 0xbf724df3,
	0xbf16fe3f, 0xbe68fe8c, 0x3eec30d2, 0x3e8fd73f,
	0xbdb5a832, 0x3f3cb41d, 0xbf3582b0, 0xbe7d0a43,
	0x3e6e2b26, 0x3d924119, 0xbe64103c, 0x3e1094c3,
	0x3f130299, 0xbe9a77df, 0x3e62628d, 0xbf800000,
	0xbf1bc246, 0xbe9f54be, 0x3d9be53a, 0xbf1a9110,
	0x3c09e4b3, 0xbe43e5d1, 0x3dd67477, 0xbeebdf5a,
	0x3d445065, 0xbf24b81b, 0x3f442915, 0xbe439b36,
	0x3ebdb5d4, 0x3f3245c5, 0x3dde2d5d, 0x3dd499a6,
	0xbe16e5e8, 0xbda4dbc5, 0x3ea0851a, 0xbf187446,
	0xbf5e8c93, 0xbea10739, 0xbe22001f, 0xbe20c313,
	0xbeee242f, 0x3d076127, 0x3e7c303c, 0x3ed97950,
	0xbf6a57bf, 0xbe9b9d13, 0x3ddf83ce, 0xbec67d55
	};

void test_benchmark_vec_add_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_add_f32(
		(float32_t *)input1, (float32_t *)input2, output,
		PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_sub_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_sub_f32(
		(float32_t *)input1, (float32_t *)input2, output,
		PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_mult_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_mult_f32(
		(float32_t *)input1, (float32_t *)input2, output,
		PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_abs_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_abs_f32((float32_t *)input1, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_negate_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_negate_f32((float32_t *)input1, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_offset_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_offset_f32((float32_t *)input1, 1.0, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_scale_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(float32_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_scale_f32((float32_t *)input1, 1.0, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_dot_prod_f32(void)
{
	uint32_t irq_key, timestamp, timespan;
	float32_t output;

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_dot_prod_f32(
		(float32_t *)input1, (float32_t *)input2, PATTERN_LENGTH,
		&output);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

ztest_register_test_suite(basicmath_f32_benchmark, NULL,
			  ztest_unit_test(test_benchmark_vec_add_f32),
			  ztest_unit_test(test_benchmark_vec_sub_f32),
			  ztest_unit_test(test_benchmark_vec_mult_f32),
			  ztest_unit_test(test_benchmark_vec_abs_f32),
			  ztest_unit_test(test_benchmark_vec_negate_f32),
			  ztest_unit_test(test_benchmark_vec_offset_f32),
			  ztest_unit_test(test_benchmark_vec_scale_f32),
			  ztest_unit_test(test_benchmark_vec_dot_prod_f32));
