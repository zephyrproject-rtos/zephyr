/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This is not exhaustive functional testing of the CMSIS-NN library.
 *
 * Individual tests have been pulled from CMSIS/NN/Tests/UnitTest to
 * validate the integration of CMSIS-NN and Zephyr
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include "arm_nnfunctions.h"

#define REPEAT_NUM 3

#define AVGPOOLING_2_OUT_CH             5
#define AVGPOOLING_2_IN_CH              5
#define AVGPOOLING_2_INPUT_W            12
#define AVGPOOLING_2_INPUT_H            1
#define AVGPOOLING_2_DST_SIZE           60
#define AVGPOOLING_2_INPUT_SIZE         60
#define AVGPOOLING_2_OUT_ACTIVATION_MIN -128
#define AVGPOOLING_2_OUT_ACTIVATION_MAX 127
#define AVGPOOLING_2_INPUT_BATCHES      1
#define AVGPOOLING_2_FILTER_X           3
#define AVGPOOLING_2_FILTER_Y           1
#define AVGPOOLING_2_STRIDE_X           1
#define AVGPOOLING_2_STRIDE_Y           2
#define AVGPOOLING_2_PAD_X              1
#define AVGPOOLING_2_PAD_Y              0
#define AVGPOOLING_2_OUTPUT_W           12
#define AVGPOOLING_2_OUTPUT_H           1

const int8_t avgpooling_2_input[60] = {
	80, 16, -80, -96, 96, -64, -112, -112, 48, 16, -80, -80, 80, 64, -80,
	16, 48, -112, 0, 48, 96, -80, -112, -64, -32, -16, -112, -64, -64, 80,
	-96, -112, -16, -80, -80, -112, -64, -48, 16, 64, 32, 48, 16, 64, 16,
	-48, -64, -32, -80, 64, -48, -32, -32, -112, 32, 32, -112, -96, -96, 48
};

const int8_t avgpooling_2_output_ref[60] = {
	8, -48, -96, -24, 56, -21, -59, -37, 5, 11, -43, -48, -48, 37, -5,
	11, -37, -48, 0, -21, 32, -48, -96, -43, 32, -5, -101, -64, -69, -11,
	-75, -96, -43, -43, 21, -59, -43, -16, 0, 0, -43, -27, -21, 0, 48,
	-21, -16, -16, -43, 37, -21, -69, -53, -96, 48, -8, -72, -64, -104, 40
};

ZTEST(cmsis_nn, test_avgpool)
{
	q7_t output[AVGPOOLING_2_DST_SIZE] = { 0 };

	cmsis_nn_context ctx;
	cmsis_nn_pool_params pool_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;

	input_dims.n = AVGPOOLING_2_INPUT_BATCHES;
	input_dims.w = AVGPOOLING_2_INPUT_W;
	input_dims.h = AVGPOOLING_2_INPUT_H;
	input_dims.c = AVGPOOLING_2_IN_CH;
	filter_dims.w = AVGPOOLING_2_FILTER_X;
	filter_dims.h = AVGPOOLING_2_FILTER_Y;
	output_dims.w = AVGPOOLING_2_OUTPUT_W;
	output_dims.h = AVGPOOLING_2_OUTPUT_H;
	output_dims.c = AVGPOOLING_2_OUT_CH;

	pool_params.padding.w = AVGPOOLING_2_PAD_X;
	pool_params.padding.h = AVGPOOLING_2_PAD_Y;
	pool_params.stride.w = AVGPOOLING_2_STRIDE_X;
	pool_params.stride.h = AVGPOOLING_2_STRIDE_Y;

	pool_params.activation.min = AVGPOOLING_2_OUT_ACTIVATION_MIN;
	pool_params.activation.max = AVGPOOLING_2_OUT_ACTIVATION_MAX;

	ctx.size = arm_avgpool_s8_get_buffer_size(AVGPOOLING_2_OUTPUT_W, AVGPOOLING_2_IN_CH);
	ctx.buf = malloc(ctx.size);

	arm_status result = arm_avgpool_s8(&ctx, &pool_params, &input_dims, avgpooling_2_input,
					   &filter_dims, &output_dims, output);

	free(ctx.buf);

	zassert_equal(ARM_MATH_SUCCESS, result, "");
	zassert_mem_equal(avgpooling_2_output_ref, output, sizeof(output), "");
}

#define CONV_4_OUT_CH                   3
#define CONV_4_IN_CH                    3
#define CONV_4_INPUT_W                  5
#define CONV_4_INPUT_H                  5
#define CONV_4_DST_SIZE                 36
#define CONV_4_INPUT_SIZE               75
#define CONV_4_OUT_ACTIVATION_MIN       -128
#define CONV_4_OUT_ACTIVATION_MAX       127
#define CONV_4_INPUT_BATCHES            3
#define CONV_4_INPUT_OFFSET             0
#define CONV_4_OUTPUT_OFFSET            0
#define CONV_4_FILTER_X                 2
#define CONV_4_FILTER_Y                 3
#define CONV_4_STRIDE_X                 2
#define CONV_4_STRIDE_Y                 2
#define CONV_4_PAD_X                    0
#define CONV_4_PAD_Y                    0
#define CONV_4_OUTPUT_W                 2
#define CONV_4_OUTPUT_H                 2

const int32_t conv_4_biases[3] = { 2699, -5398, -2699 };

const q7_t conv_4_weights[54] = {
	-127, 64, 64, -64, 0, 0, 64, -64, 0, -64, 64, 64, 64, -127,
	64, 0, -127, -64, 64, 64, -64, -64, -64, -64, -64, 0, 0, 64,
	64, 64, 0, 0, 0, -127, -64, -127, -127, 0, 0, 0, 0, -127,
	-127, -127, -127, 64, -127, 64, 64, 0, 0, -64, -127, 64
};

const q7_t conv_4_input[225] = {
	42, -85, -85, 0, 42, 42, -42, -42, -42, -85, 42, 42, -42, -42, -85,
	0, -85, 0, 42, -42, 0, -42, 42, -42, -42, 42, -42, 42, -85, -42,
	-85, -42, 0, -42, -42, -42, 42, -85, -42, -42, -42, 0, -42, 0, 0,
	0, 42, -42, 42, 0, -42, 0, 0, -85, 0, 42, 42, 0, 42, 42, -85, 42,
	42, -85, -42, 0, -85, 42, -42, -85, -42, -85, 42, 42, -85, -85, 42,
	42, 42, -85, 42, -85, -42, -42, 0, -42, -85, -85, 42, -85, 0, -85,
	42, 42, 0, 42, 42, 42, 42, -85, 42, -85, -42, 0, 42, 0, 0, -85, -42,
	0, -85, 0, 42, -85, -42, 0, -42, 0, 42, -42, -42, -85, 0, -85, -42,
	-85, 0, 42, -85, -85, -85, -85, 0, -85, 42, 42, 0, -42, -85, -85, 0,
	-42, 0, 0, -85, -85, -42, 42, -85, -42, -42, 42, -85, 0, 42, 0, -85,
	0, 0, 42, 42, -85, -85, -85, 0, 42, 0, 0, 42, -85, -85, 42, -85, -42,
	-42, 0, -85, -85, 42, -85, 0, -85, -42, -85, 42, 0, 42, 42, 0, -85,
	0, 0, 0, 0, 0, -42, -85, 42, 0, -85, -42, 0, -42, 42, 42, -85, 0,
	42, 42, 0, -42, -85, -42, -85, 0, 42, -85, -85, -42, 42, -42, -42,
	-42, -42, 42
};

const int32_t conv_4_output_mult[3] = { 1629660588, 1629660588, 1629660588 };

const int32_t conv_4_output_shift[3] = { -11, -11, -11 };

const q7_t conv_4_output_ref[36] = {
	-2, 2, 2, 8, 0, 1, 1, 3, 7, -2, 11, 0, 8, 4, 4, 1, -1, -5,
	4, 5, 14, 2, 5, 7, -1, -2, 2, 5, -4, 11, -1, -2, 8, 4, 2, 0
};

ZTEST(cmsis_nn, test_convolve)
{
	q7_t output[CONV_4_DST_SIZE] = { 0 };

	cmsis_nn_context ctx;
	cmsis_nn_conv_params conv_params;
	cmsis_nn_per_channel_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_dims output_dims;

	const q31_t *bias_data = conv_4_biases;
	const q7_t *kernel_data = conv_4_weights;
	const q7_t *input_data = conv_4_input;

	input_dims.n = CONV_4_INPUT_BATCHES;
	input_dims.w = CONV_4_INPUT_W;
	input_dims.h = CONV_4_INPUT_H;
	input_dims.c = CONV_4_IN_CH;
	filter_dims.w = CONV_4_FILTER_X;
	filter_dims.h = CONV_4_FILTER_Y;
	output_dims.w = CONV_4_OUTPUT_W;
	output_dims.h = CONV_4_OUTPUT_H;
	output_dims.c = CONV_4_OUT_CH;

	conv_params.padding.w = CONV_4_PAD_X;
	conv_params.padding.h = CONV_4_PAD_Y;
	conv_params.stride.w = CONV_4_STRIDE_X;
	conv_params.stride.h = CONV_4_STRIDE_Y;

	conv_params.input_offset = CONV_4_INPUT_OFFSET;
	conv_params.output_offset = CONV_4_OUTPUT_OFFSET;
	conv_params.activation.min = CONV_4_OUT_ACTIVATION_MIN;
	conv_params.activation.max = CONV_4_OUT_ACTIVATION_MAX;
	quant_params.multiplier = (int32_t *)conv_4_output_mult;
	quant_params.shift = (int32_t *)conv_4_output_shift;

	int32_t buf_size = arm_convolve_s8_get_buffer_size(&input_dims, &filter_dims);

	ctx.buf = malloc(buf_size);
	ctx.size = 0;

	arm_status result = arm_convolve_s8(&ctx,
					    &conv_params,
					    &quant_params,
					    &input_dims,
					    input_data,
					    &filter_dims,
					    kernel_data,
					    &bias_dims,
					    bias_data,
					    &output_dims,
					    output);

	free(ctx.buf);
	zassert_equal(ARM_MATH_SUCCESS, result, "");
	zassert_mem_equal(conv_4_output_ref, output, sizeof(output), "");

	buf_size = arm_convolve_wrapper_s8_get_buffer_size(&conv_params, &input_dims,
							   &filter_dims, &output_dims);
	ctx.buf = malloc(buf_size);
	ctx.size = 0;

	result = arm_convolve_wrapper_s8(&ctx,
					 &conv_params,
					 &quant_params,
					 &input_dims,
					 input_data,
					 &filter_dims,
					 kernel_data,
					 &bias_dims,
					 bias_data,
					 &output_dims,
					 output);

	free(ctx.buf);
	zassert_equal(ARM_MATH_SUCCESS, result, "");
	zassert_mem_equal(conv_4_output_ref, output, sizeof(output), "");
}

#define STRIDE2PAD1_OUT_CH              1
#define STRIDE2PAD1_IN_CH               1
#define STRIDE2PAD1_INPUT_W             7
#define STRIDE2PAD1_INPUT_H             7
#define STRIDE2PAD1_DST_SIZE            16
#define STRIDE2PAD1_INPUT_SIZE          49
#define STRIDE2PAD1_OUT_ACTIVATION_MIN  -128
#define STRIDE2PAD1_OUT_ACTIVATION_MAX  127
#define STRIDE2PAD1_INPUT_BATCHES       1
#define STRIDE2PAD1_INPUT_OFFSET        128
#define STRIDE2PAD1_OUTPUT_OFFSET       0
#define STRIDE2PAD1_FILTER_X            3
#define STRIDE2PAD1_FILTER_Y            3
#define STRIDE2PAD1_STRIDE_X            2
#define STRIDE2PAD1_STRIDE_Y            2
#define STRIDE2PAD1_PAD_X               1
#define STRIDE2PAD1_PAD_Y               1
#define STRIDE2PAD1_OUTPUT_W            4
#define STRIDE2PAD1_OUTPUT_H            4

const int32_t stride2pad1_biases[1] = { 4318 };

const q7_t stride2pad1_weights[9] = { 42, 127, 127, 127, 42, 127, 85, 42, 85 };

const q7_t stride2pad1_input[49] = {
	-26, -77, -26, -26, 25, -77, -77, -26, 25, -26, -77, -26, -26, -77, 25, -77, -26,
	-26, -77, -26, -77, -26, -77, -26, 25, -77, -26, -26, -26, 25, -26, -77, -77, -77,
	-26, 25, 25, -26, -77, -26, -26, -26, -26, -26, -77, -26, 25, -77, -26
};

const int32_t stride2pad1_output_mult[1] = { 2037075735 };

const int32_t stride2pad1_output_shift[1] = { -11 };

const q7_t stride2pad1_output_ref[16] = {
	15, 23, 22, 11, 27, 35, 39, 20, 31, 42, 29, 21, 28, 27, 27, 15
};

ZTEST(cmsis_nn, test_depthwise_convolve)
{
	q7_t output[STRIDE2PAD1_DST_SIZE] = { 0 };

	cmsis_nn_context ctx;
	cmsis_nn_dw_conv_params dw_conv_params;
	cmsis_nn_per_channel_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims = { 0 };
	cmsis_nn_dims output_dims;

	const q31_t *bias_data = stride2pad1_biases;
	const q7_t *kernel_data = stride2pad1_weights;
	const q7_t *input_data = stride2pad1_input;

	input_dims.n = STRIDE2PAD1_INPUT_BATCHES;
	input_dims.w = STRIDE2PAD1_INPUT_W;
	input_dims.h = STRIDE2PAD1_INPUT_H;
	input_dims.c = STRIDE2PAD1_IN_CH;
	filter_dims.w = STRIDE2PAD1_FILTER_X;
	filter_dims.h = STRIDE2PAD1_FILTER_Y;
	output_dims.w = STRIDE2PAD1_OUTPUT_W;
	output_dims.h = STRIDE2PAD1_OUTPUT_H;
	output_dims.c = STRIDE2PAD1_OUT_CH;

	dw_conv_params.padding.w = STRIDE2PAD1_PAD_X;
	dw_conv_params.padding.h = STRIDE2PAD1_PAD_Y;
	dw_conv_params.stride.w = STRIDE2PAD1_STRIDE_X;
	dw_conv_params.stride.h = STRIDE2PAD1_STRIDE_Y;
	dw_conv_params.ch_mult = 1;

	dw_conv_params.input_offset = STRIDE2PAD1_INPUT_OFFSET;
	dw_conv_params.output_offset = STRIDE2PAD1_OUTPUT_OFFSET;
	dw_conv_params.activation.min = STRIDE2PAD1_OUT_ACTIVATION_MIN;
	dw_conv_params.activation.max = STRIDE2PAD1_OUT_ACTIVATION_MAX;
	quant_params.multiplier = (int32_t *)stride2pad1_output_mult;
	quant_params.shift = (int32_t *)stride2pad1_output_shift;

	ctx.buf = NULL;
	ctx.size = 0;

	arm_status result = arm_depthwise_conv_s8(&ctx,
						  &dw_conv_params,
						  &quant_params,
						  &input_dims,
						  input_data,
						  &filter_dims,
						  kernel_data,
						  &bias_dims,
						  bias_data,
						  &output_dims,
						  output);

	free(ctx.buf);
	zassert_equal(ARM_MATH_SUCCESS, result, "");
	zassert_mem_equal(stride2pad1_output_ref, output, sizeof(output), "");
}

#define FULLY_CONNECTED_MVE_0_OUT_CH                    9
#define FULLY_CONNECTED_MVE_0_IN_CH                     16
#define FULLY_CONNECTED_MVE_0_INPUT_W                   1
#define FULLY_CONNECTED_MVE_0_INPUT_H                   1
#define FULLY_CONNECTED_MVE_0_DST_SIZE                  9
#define FULLY_CONNECTED_MVE_0_INPUT_SIZE                16
#define FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MIN        -128
#define FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MAX        127
#define FULLY_CONNECTED_MVE_0_INPUT_BATCHES             1
#define FULLY_CONNECTED_MVE_0_INPUT_OFFSET              3
#define FULLY_CONNECTED_MVE_0_OUTPUT_OFFSET             -2
#define FULLY_CONNECTED_MVE_0_OUTPUT_MULTIPLIER         1073741824
#define FULLY_CONNECTED_MVE_0_OUTPUT_SHIFT              1
#define FULLY_CONNECTED_MVE_0_ACCUMULATION_DEPTH        16

const int32_t fully_connected_mve_0_biases[9] = { -1, 0, 0, 2, -1, -1, 1, -3, -4 };

const q7_t fully_connected_mve_0_input[16] = {
	-5, -3, -5, -3, -3, -6, -1, -5, -4, -3, -2, 0, -2, -1, -2, -6
};

const q7_t fully_connected_mve_0_output_ref[9] = { 0, -29, 33, -5, 28, -5, 19, -7, 16 };

const q7_t fully_connected_mve_0_weights[144] = {
	1, 0, -1, -3, -4, -3, 3, -2, 3, 3, 1, 2, -2, -4, -4, 2, 3, 2, 3, -1, -2, 2,
	-4, 0, 1, -3, -3, -3, 1, 1, -3, -4, -3, 3, 2, 3, 1, -4, 3, -3, -1, 3, 1, -2,
	2, 3, -4, -3, 2, -4, 0, 3, 0, -2, 0, -1, -2, 0, 3, -3, -1, -2, -3, -1, -4,
	1, 2, -1, -4, -4, 1, -3, -3, 2, 3, 1, -3, -2, -4, -3, -2, 2, 1, 1, 1, -2, 0,
	3, -3, -2, -1, -4, -2, 2, 1, -1, -4, 2, 2, 3, 3, 2, 0, -3, 2, 3, 0, 3, 3, -1,
	-4, -4, 0, 1, -4, -1, -3, 3, 2, 3, 2, -3, -1, -3, 0, 3, -2, -3, -2, 3, -4, 3,
	-1, -4, 2, 2, 3, 1, -1, 1, 0, -4, -2, -3
};

ZTEST(cmsis_nn, test_fully_connected)
{
	q7_t output[FULLY_CONNECTED_MVE_0_DST_SIZE] = { 0 };

	cmsis_nn_context ctx;
	cmsis_nn_fc_params fc_params;
	cmsis_nn_per_tensor_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_dims output_dims;

	const q31_t *bias_data = fully_connected_mve_0_biases;
	const q7_t *kernel_data = fully_connected_mve_0_weights;
	const q7_t *input_data = fully_connected_mve_0_input;

	input_dims.n = FULLY_CONNECTED_MVE_0_INPUT_BATCHES;
	input_dims.w = FULLY_CONNECTED_MVE_0_INPUT_W;
	input_dims.h = FULLY_CONNECTED_MVE_0_INPUT_H;
	input_dims.c = FULLY_CONNECTED_MVE_0_IN_CH;
	filter_dims.n = FULLY_CONNECTED_MVE_0_ACCUMULATION_DEPTH;
	filter_dims.c = FULLY_CONNECTED_MVE_0_OUT_CH;
	output_dims.n = FULLY_CONNECTED_MVE_0_INPUT_BATCHES;
	output_dims.c = FULLY_CONNECTED_MVE_0_OUT_CH;

	fc_params.input_offset = FULLY_CONNECTED_MVE_0_INPUT_OFFSET;
	fc_params.filter_offset = 0;
	fc_params.output_offset = FULLY_CONNECTED_MVE_0_OUTPUT_OFFSET;
	fc_params.activation.min = FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MIN;
	fc_params.activation.max = FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MAX;

	quant_params.multiplier = FULLY_CONNECTED_MVE_0_OUTPUT_MULTIPLIER;
	quant_params.shift = FULLY_CONNECTED_MVE_0_OUTPUT_SHIFT;

	int32_t buf_size = arm_fully_connected_s8_get_buffer_size(&filter_dims);

	ctx.buf = malloc(buf_size);
	ctx.size = buf_size;
	arm_status result = arm_fully_connected_s8(&ctx,
						   &fc_params,
						   &quant_params,
						   &input_dims,
						   input_data,
						   &filter_dims,
						   kernel_data,
						   &bias_dims,
						   bias_data,
						   &output_dims,
						   output);

	free(ctx.buf);
	zassert_equal(ARM_MATH_SUCCESS, result, "");
	zassert_mem_equal(fully_connected_mve_0_output_ref, output, sizeof(output), "");
}

#define MAXPOOLING_2_OUT_CH             5
#define MAXPOOLING_2_IN_CH              5
#define MAXPOOLING_2_INPUT_W            12
#define MAXPOOLING_2_INPUT_H            1
#define MAXPOOLING_2_DST_SIZE           60
#define MAXPOOLING_2_INPUT_SIZE         60
#define MAXPOOLING_2_OUT_ACTIVATION_MIN -128
#define MAXPOOLING_2_OUT_ACTIVATION_MAX 127
#define MAXPOOLING_2_INPUT_BATCHES      1
#define MAXPOOLING_2_FILTER_X           3
#define MAXPOOLING_2_FILTER_Y           1
#define MAXPOOLING_2_STRIDE_X           1
#define MAXPOOLING_2_STRIDE_Y           2
#define MAXPOOLING_2_PAD_X              1
#define MAXPOOLING_2_PAD_Y              0
#define MAXPOOLING_2_OUTPUT_W           12
#define MAXPOOLING_2_OUTPUT_H           1

const int8_t maxpooling_2_input[60] = {
	-16, 32, -16, -48, -16, 16, 64, 0, -112, 80, -64, 48, -64, 80, -16,
	-80, -96, 48, 32, 96, 64, 80, 16, -96, 32, -112, -16, -80, -48, 32,
	-64, -32, -16, 80, 48, -80, 96, -96, 64, -64, -112, 32, 96, -16, -16,
	96, 0, -16, -16, -32, 64, -96, 96, 96, -48, -64, -16, 32, 16, 64
};

const int8_t maxpooling_2_output_ref[60] = {
	16, 64, 0, -48, 80, 16, 64, 0, 80, 80, 16, 64, 48, 80, 96,
	64, 80, 48, 80, 96, 64, 80, 48, 32, 96, 64, 80, 16, 80, 48,
	-64, 96, -16, 80, 48, -64, 96, 96, 80, 48, 96, 96, 96, 64, -16,
	96, 32, 96, 96, -16, 96, 0, 96, 96, 64, 64, -16, 96, 96, 64
};

ZTEST(cmsis_nn, test_max_pool)
{
	q7_t output[MAXPOOLING_2_DST_SIZE] = { 0 };

	cmsis_nn_context ctx;
	cmsis_nn_pool_params pool_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;

	const q7_t *input_data = maxpooling_2_input;

	input_dims.n = MAXPOOLING_2_INPUT_BATCHES;
	input_dims.w = MAXPOOLING_2_INPUT_W;
	input_dims.h = MAXPOOLING_2_INPUT_H;
	input_dims.c = MAXPOOLING_2_IN_CH;
	filter_dims.w = MAXPOOLING_2_FILTER_X;
	filter_dims.h = MAXPOOLING_2_FILTER_Y;
	output_dims.w = MAXPOOLING_2_OUTPUT_W;
	output_dims.h = MAXPOOLING_2_OUTPUT_H;
	output_dims.c = MAXPOOLING_2_OUT_CH;

	pool_params.padding.w = MAXPOOLING_2_PAD_X;
	pool_params.padding.h = MAXPOOLING_2_PAD_Y;
	pool_params.stride.w = MAXPOOLING_2_STRIDE_X;
	pool_params.stride.h = MAXPOOLING_2_STRIDE_Y;

	pool_params.activation.min = MAXPOOLING_2_OUT_ACTIVATION_MIN;
	pool_params.activation.max = MAXPOOLING_2_OUT_ACTIVATION_MAX;

	for (int i = 0; i < REPEAT_NUM; i++) {
		arm_status result = arm_max_pool_s8(&ctx, &pool_params, &input_dims, input_data,
						    &filter_dims, &output_dims, output);

		zassert_equal(ARM_MATH_SUCCESS, result, "");
		zassert_mem_equal(maxpooling_2_output_ref, output, sizeof(output), "");
	}
}

#define SOFTMAX_NUM_ROWS                1
#define SOFTMAX_ROW_SIZE                5
#define SOFTMAX_INPUT_MULT              1077952576
#define SOFTMAX_INPUT_LEFT_SHIFT        23
#define SOFTMAX_DIFF_MIN                -248
#define SOFTMAX_DST_SIZE                5

const q7_t softmax_input[5] = { -80, -48, 16, 0, -96 };

const q7_t softmax_output_ref[5] = { -128, -125, 56, -60, -128 };

ZTEST(cmsis_nn, test_softmax)
{
	const int32_t num_rows = SOFTMAX_NUM_ROWS;
	const int32_t row_size = SOFTMAX_ROW_SIZE;
	const int32_t mult = SOFTMAX_INPUT_MULT;
	const int32_t shift = SOFTMAX_INPUT_LEFT_SHIFT;
	const int32_t diff_min = SOFTMAX_DIFF_MIN;
	const q7_t *input_data = softmax_input;
	int8_t output[SOFTMAX_DST_SIZE];

	for (int i = 0; i < REPEAT_NUM; i++) {
		arm_softmax_s8(input_data, num_rows, row_size, mult, shift, diff_min, output);
		zassert_mem_equal(softmax_output_ref, output, sizeof(output), "");
	}
}

#define SVDF_2_INPUT_OFFSET             0
#define SVDF_2_OUTPUT_OFFSET            0
#define SVDF_2_MULTIPLIER_IN            1347440720
#define SVDF_2_MULTIPLIER_OUT           1073741824
#define SVDF_2_SHIFT_1                  -4
#define SVDF_2_SHIFT_2                  1
#define SVDF_2_IN_ACTIVATION_MIN        -32767
#define SVDF_2_IN_ACTIVATION_MAX        32767
#define SVDF_2_RANK                     2
#define SVDF_2_FEATURE_BATCHES          10
#define SVDF_2_TIME_BATCHES             2
#define SVDF_2_INPUT_SIZE               7
#define SVDF_2_DST_SIZE                 15
#define SVDF_2_OUT_ACTIVATION_MIN       -128
#define SVDF_2_OUT_ACTIVATION_MAX       127
#define SVDF_2_INPUT_BATCHES            3

const int32_t svdf_2_biases[5] = { 0, 0, 0, 0, 0 };


const q15_t svdf_2_state[60] = {
	3, 1, -1, 2, 1, 4, 3, 2, 2, 1, 4, -1, -3, 3, 4, 3, 1, -1, 3, 2,
	0, -2, -1, -2, -1, -3, 0, -3, 4, 3, -1, 4, -4, -1, 2, 3, -4, -3, -2, 1,
	1, 4, 3, -2, -3, -2, 4, 0, -2, 1, -2, -3, -4, 2, 0, -2, -3, 0, -1, 0
};

const q7_t svdf_2_weights_feature[70] = {
	-4, 0, 2, -2, 1, 1, -1, 0, -1, 2, -1, 1, 1, 3, -3, -2, -2, 3,
	3, -3, 1, 2, 1, -4, 0, 2, -2, -1, 3, 1, 0, 0, 1, -2, 0, 2,
	1, 0, -1, 2, 3, -1, 3, -1, -1, -2, -4, -3, 1, 1, 2, -3, 3, -3,
	0, 0, 2, 0, 2, -1, -1, -3, -3, 1, 2, 2, 3, -2, 3, 1
};

const q15_t svdf_2_weights_time[20] = {
	-4, 3, 0, -3, -2, 0, 3, 0, -3, -2, 2, 1, -4, 3, 1, 0, 3, -2, 1, 1
};

const q7_t svdf_2_input_sequence[42] = {
	-51, 0, -26, 76, -102, -102, -76, 0, -51, -26, -51, -26, 51, 0,
	51, -102, 51, -102, -76, 51, 76, -26, 26, -51, -76, -26, -102, -76,
	-26, 26, 0, 51, 76, 0, 0, 26, -26, 76, -26, 76, 76, 26
};

const q7_t svdf_2_output_ref[15] = {
	80, -19, -61, 17, -17, -3, 6, 30, -84, -4, -24, -11, 35, -128, 19
};

static bool check_null_bias(const int32_t *bias, int32_t size)
{
	bool null_bias = true;

	for (int i = 0; i < size; i++) {
		if (bias[i] != 0) {
			null_bias = false;
			break;
		}
	}
	return null_bias;
}

ZTEST(cmsis_nn, test_svdf)
{
	cmsis_nn_context input_ctx;
	cmsis_nn_context output_ctx;
	cmsis_nn_svdf_params svdf_2_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims weights_feature_dims;
	cmsis_nn_dims weights_time_dims;
	cmsis_nn_dims state_dims;
	cmsis_nn_dims output_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_per_tensor_quant_params input_quant_params;
	cmsis_nn_per_tensor_quant_params output_quant_params;
	int8_t output_data[SVDF_2_DST_SIZE];

	const q7_t *weights_feature_data = svdf_2_weights_feature;
	const q15_t *weights_time_data = svdf_2_weights_time;

	input_dims.n = SVDF_2_INPUT_BATCHES;
	input_dims.h = SVDF_2_INPUT_SIZE;
	weights_feature_dims.n = SVDF_2_FEATURE_BATCHES;
	weights_time_dims.h = SVDF_2_TIME_BATCHES;

	input_quant_params.multiplier = SVDF_2_MULTIPLIER_IN;
	input_quant_params.shift = SVDF_2_SHIFT_1;
	output_quant_params.multiplier = SVDF_2_MULTIPLIER_OUT;
	output_quant_params.shift = SVDF_2_SHIFT_2;

	svdf_2_params.input_activation.min = SVDF_2_IN_ACTIVATION_MIN;
	svdf_2_params.input_activation.max = SVDF_2_IN_ACTIVATION_MAX;
	svdf_2_params.output_activation.min = SVDF_2_OUT_ACTIVATION_MIN;
	svdf_2_params.output_activation.max = SVDF_2_OUT_ACTIVATION_MAX;
	svdf_2_params.input_offset = SVDF_2_INPUT_OFFSET;
	svdf_2_params.output_offset = SVDF_2_OUTPUT_OFFSET;
	svdf_2_params.rank = SVDF_2_RANK;

	const int input_round_size = SVDF_2_INPUT_BATCHES * SVDF_2_INPUT_SIZE;
	const int number_inputs = sizeof(svdf_2_input_sequence) / input_round_size;
	const int32_t number_units = SVDF_2_FEATURE_BATCHES / SVDF_2_RANK;
	const int scratch_size = SVDF_2_INPUT_BATCHES * SVDF_2_FEATURE_BATCHES * sizeof(int32_t);
	const int scratch_size_out = SVDF_2_INPUT_BATCHES * number_units * sizeof(int32_t);

	input_ctx.buf = malloc(scratch_size);
	output_ctx.buf = malloc(scratch_size_out);

	int8_t *input_data = malloc(input_round_size);
	q15_t *state_data = malloc(sizeof(svdf_2_state));
	const bool null_bias = check_null_bias(svdf_2_biases,
					       SVDF_2_DST_SIZE / SVDF_2_INPUT_BATCHES);

	for (int i = 0; i < REPEAT_NUM; i++) {
		memcpy(state_data, svdf_2_state, sizeof(svdf_2_state));
		for (int j = 0; j < number_inputs; j++) {
			memcpy(input_data, svdf_2_input_sequence + j * input_round_size,
			       input_round_size);
			arm_status result = arm_svdf_s8(&input_ctx,
							&output_ctx,
							&svdf_2_params,
							&input_quant_params,
							&output_quant_params,
							&input_dims,
							input_data,
							&state_dims,
							state_data,
							&weights_feature_dims,
							weights_feature_data,
							&weights_time_dims,
							weights_time_data,
							&bias_dims,
							null_bias == true ? NULL : svdf_2_biases,
							&output_dims,
							output_data);
			zassert_equal(ARM_MATH_SUCCESS, result, "");
		}

		zassert_mem_equal(svdf_2_output_ref, output_data, sizeof(output_data), "");
	}
	free(state_data);
	free(input_data);
	free(input_ctx.buf);
	free(output_ctx.buf);
}

ZTEST_SUITE(cmsis_nn, NULL, NULL, NULL, NULL, NULL);
