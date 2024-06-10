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

#define AVGPOOLING_2_OUT_CH		5
#define AVGPOOLING_2_IN_CH		5
#define AVGPOOLING_2_INPUT_W		12
#define AVGPOOLING_2_INPUT_H		1
#define AVGPOOLING_2_DST_SIZE		60
#define AVGPOOLING_2_INPUT_SIZE		60
#define AVGPOOLING_2_OUT_ACTIVATION_MIN -128
#define AVGPOOLING_2_OUT_ACTIVATION_MAX 127
#define AVGPOOLING_2_INPUT_BATCHES	1
#define AVGPOOLING_2_FILTER_X		3
#define AVGPOOLING_2_FILTER_Y		1
#define AVGPOOLING_2_STRIDE_X		1
#define AVGPOOLING_2_STRIDE_Y		2
#define AVGPOOLING_2_PAD_X		1
#define AVGPOOLING_2_PAD_Y		0
#define AVGPOOLING_2_OUTPUT_W		12
#define AVGPOOLING_2_OUTPUT_H		1

const int8_t avgpooling_2_input[60] = {
	-82, -104, 10,	-28, -52, -51, -66, 52,	 124, -74, -21,	 4,  37,   -7,	-33,
	102, 110,  24,	52,  121, 13,  -55, -79, -92, -35, -103, 86, 95,   46,	32,
	-24, -123, 120, 29,  -77, -97, -69, -68, 58,  38,  3,	 3,  79,   -47, 112,
	-52, -113, -46, 107, 68,  83,  -70, 91,	 14,  113, 74,	 73, -103, -98, 25};

const int8_t avgpooling_2_output_ref[60] = {
	-67, -85, 31, 48,  -63, -51, -55, 33,  30, -53, 10,  16,  38,  56,  5,
	31,  20,  -6, -16, 18,	4,   47,  13,  2,  39,	-38, -31, 45,  -6,  -27,
	-75, -35, 49, 44,  -2,	-39, -63, 44,  13, 24,	-49, -60, -12, 39,  73,
	11,  -60, 41, 25,  98,	35,  -37, -19, 8,  69,	79,  2,	  -6,  -42, 69};

ZTEST(cmsis_nn, test_avgpool)
{
	int8_t output[AVGPOOLING_2_DST_SIZE] = {0};

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

	arm_cmsis_nn_status result = arm_avgpool_s8(&ctx,
						   &pool_params,
						   &input_dims,
						   avgpooling_2_input,
						   &filter_dims,
						   &output_dims,
						   output);

	free(ctx.buf);

	zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
	zassert_mem_equal(avgpooling_2_output_ref, output, sizeof(output), "");
}

#define CONV_4_OUT_CH		  3
#define CONV_4_IN_CH		  3
#define CONV_4_INPUT_W		  5
#define CONV_4_INPUT_H		  5
#define CONV_4_DST_SIZE		  36
#define CONV_4_INPUT_SIZE	  75
#define CONV_4_OUT_ACTIVATION_MIN -109
#define CONV_4_OUT_ACTIVATION_MAX 127
#define CONV_4_INPUT_BATCHES	  3
#define CONV_4_FILTER_X		  2
#define CONV_4_FILTER_Y		  3
#define CONV_4_STRIDE_X		  2
#define CONV_4_STRIDE_Y		  2
#define CONV_4_PAD_X		  0
#define CONV_4_PAD_Y		  0
#define CONV_4_OUTPUT_W		  2
#define CONV_4_OUTPUT_H		  2
#define CONV_4_INPUT_OFFSET	  128
#define CONV_4_OUTPUT_OFFSET	  -128
#define CONV_4_DILATION_X	  1
#define CONV_4_DILATION_Y	  1

const int32_t conv_4_biases[3] = {13175, 9050, 18215};

const int8_t conv_4_weights[54] = {
	-25, -83, -74,	105, 30,  118, -32, 127, 34,  127, -112, 39, -43, 104, 41,  -124, 115, 5,
	42,  -48, -119, 93,  17,  57,  41,  -41, -42, 23,  127,	 18, 70,  -99, 71,  67,	  83,  76,
	-50, 98,  66,	64,  127, -6,  -77, -48, -26, 45,  77,	 1,  81,  27,  124, -103, 37,  36};

const int8_t conv_4_input[225] = {
	82,   120,  -97, -44,  -118, 73,   4,	 -84,  -53,  -122, -15,	 77,   83,  43,	  37,
	85,   -11,  103, 45,   -69,  -12,  -8,	 21,   6,    -68,  -83,	 -15,  -99, 90,	  -62,
	95,   62,   -38, -32,  -35,  -105, -53,	 70,   112,  14,   -4,	 -33,  -26, -93,  -98,
	22,   -5,   22,	 -104, 57,   -92,  30,	 -62,  0,    -43,  -82,	 60,   99,  -83,  32,
	94,   49,   10,	 112,  -71,  -27,  -91,	 -79,  52,   -92,  -71,	 86,   -79, -15,  -80,
	-74,  -4,   76,	 -119, 91,   -23,  -12,	 -111, -72,  26,   11,	 64,   116, 38,	  99,
	125,  17,   6,	 -4,   46,   119,  113,	 -116, -125, 80,   -57,	 122,  75,  119,  -117,
	87,   -121, -70, -75,  -127, 16,   -124, -110, 10,   71,   29,	 27,   37,  -24,  52,
	28,   -100, 86,	 -75,  117,  -31,  -115, -86,  -122, 121,  -96,	 -118, 32,  111,  25,
	-90,  -8,   110, 37,   35,   124,  -123, 94,   -122, -114, 37,	 85,   -36, 53,	  -40,
	73,   -99,  27,	 10,   37,   41,   64,	 -97,  -123, 75,   0,	 -107, -72, 58,	  -100,
	17,   77,   114, 120,  -83,  -96,  75,	 -12,  -27,  3,	   35,	 85,   4,   119,  -20,
	28,   99,   104, -78,  -51,  -82,  -92,	 -40,  -116, 35,   -107, 39,   9,   -120, -50,
	-102, -114, 25,	 -77,  25,   7,	   64,	 110,  80,   -93,  -20,	 34,   115, 75,	  37,
	47,   16,   6,	 -92,  -25,  37,   69,	 82,   -61,  -100, -85,	 -51,  6,   -95,  58
};

const int32_t conv_4_output_mult[3] = {2039209398, 2005068758, 2023002003};

const int32_t conv_4_output_shift[3] = {-9, -9, -9};

const int8_t conv_4_output_ref[36] = {-5,   -39, -31, 20,  -37, -26, -109, -7,	-10, -51, -58, 48,
				      -100, -32, 24,  4,   69,	-38, -64,  65,	-34, 95,  -55, 39,
				      95,   -54, 27,  -49, 25,	-68, -109, -66, 72,  38,  -44, -40};

ZTEST(cmsis_nn, test_convolve)
{
	int8_t output[CONV_4_DST_SIZE] = {0};

	cmsis_nn_context ctx;
	cmsis_nn_conv_params conv_params;
	cmsis_nn_per_channel_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_dims output_dims;

	const int32_t *bias_data = conv_4_biases;
	const int8_t *kernel_data = conv_4_weights;
	const int8_t *input_data = conv_4_input;

	input_dims.n = CONV_4_INPUT_BATCHES;
	input_dims.w = CONV_4_INPUT_W;
	input_dims.h = CONV_4_INPUT_H;
	input_dims.c = CONV_4_IN_CH;
	filter_dims.w = CONV_4_FILTER_X;
	filter_dims.h = CONV_4_FILTER_Y;
	filter_dims.c = CONV_4_IN_CH;
	output_dims.w = CONV_4_OUTPUT_W;
	output_dims.h = CONV_4_OUTPUT_H;
	output_dims.c = CONV_4_OUT_CH;

	conv_params.padding.w = CONV_4_PAD_X;
	conv_params.padding.h = CONV_4_PAD_Y;
	conv_params.stride.w = CONV_4_STRIDE_X;
	conv_params.stride.h = CONV_4_STRIDE_Y;
	conv_params.dilation.w = CONV_4_DILATION_X;
	conv_params.dilation.h = CONV_4_DILATION_Y;

	conv_params.input_offset = CONV_4_INPUT_OFFSET;
	conv_params.output_offset = CONV_4_OUTPUT_OFFSET;
	conv_params.activation.min = CONV_4_OUT_ACTIVATION_MIN;
	conv_params.activation.max = CONV_4_OUT_ACTIVATION_MAX;
	quant_params.multiplier = (int32_t *)conv_4_output_mult;
	quant_params.shift = (int32_t *)conv_4_output_shift;

	int32_t buf_size = arm_convolve_s8_get_buffer_size(&input_dims, &filter_dims);

	ctx.buf = malloc(buf_size);
	ctx.size = 0;

	arm_cmsis_nn_status result = arm_convolve_s8(&ctx,
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
	zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
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
	zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
	zassert_mem_equal(conv_4_output_ref, output, sizeof(output), "");
}

#define STRIDE2PAD1_OUT_CH	       1
#define STRIDE2PAD1_IN_CH	       1
#define STRIDE2PAD1_INPUT_W	       7
#define STRIDE2PAD1_INPUT_H	       7
#define STRIDE2PAD1_DST_SIZE	       16
#define STRIDE2PAD1_INPUT_SIZE	       49
#define STRIDE2PAD1_OUT_ACTIVATION_MIN -128
#define STRIDE2PAD1_OUT_ACTIVATION_MAX 127
#define STRIDE2PAD1_INPUT_BATCHES      1
#define STRIDE2PAD1_FILTER_X	       3
#define STRIDE2PAD1_FILTER_Y	       3
#define STRIDE2PAD1_STRIDE_X	       2
#define STRIDE2PAD1_STRIDE_Y	       2
#define STRIDE2PAD1_PAD_X	       1
#define STRIDE2PAD1_PAD_Y	       1
#define STRIDE2PAD1_OUTPUT_W	       4
#define STRIDE2PAD1_OUTPUT_H	       4
#define STRIDE2PAD1_INPUT_OFFSET       128
#define STRIDE2PAD1_OUTPUT_OFFSET      -20
#define STRIDE2PAD1_DILATION_X	       1
#define STRIDE2PAD1_DILATION_Y	       1

const int32_t stride2pad1_biases[1] = {-9794};

const int8_t stride2pad1_weights[9] = {-54, 57, -19, -127, 87, 70, 74, -110, 66};

const int8_t stride2pad1_input[49] = {
	-91, -30, -57, -76, 32,	 -13, 14,   -96, 108, -4,  41,	48,  107, -68,	-101, 30,  95,
	95,  91,  -66, -80, 114, -49, 7,    -67, -35, -1,  -88, -77, -56, -103, 5,    -39, -118,
	-24, -32, 67,  11,  38,	 -16, -124, 44,	 -46, -92, -24, 108, 80,  -29,	-3};

const int32_t stride2pad1_output_mult[1] = {2033801520};

const int32_t stride2pad1_output_shift[1] = {-8};

const int8_t stride2pad1_output_ref[16] = {26, -11, 33,	 -25,  -96, -52, -78, -86,
					   33, -2,  -88, -113, -14, 0,	 -84, -27};

ZTEST(cmsis_nn, test_depthwise_convolve)
{
	int8_t output[STRIDE2PAD1_DST_SIZE] = {0};

	cmsis_nn_context ctx;
	cmsis_nn_dw_conv_params dw_conv_params;
	cmsis_nn_per_channel_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims = {0};
	cmsis_nn_dims output_dims;

	const int32_t *bias_data = stride2pad1_biases;
	const int8_t *kernel_data = stride2pad1_weights;
	const int8_t *input_data = stride2pad1_input;

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
	dw_conv_params.dilation.w = STRIDE2PAD1_DILATION_X;
	dw_conv_params.dilation.h = STRIDE2PAD1_DILATION_Y;

	dw_conv_params.ch_mult = 1;

	dw_conv_params.input_offset = STRIDE2PAD1_INPUT_OFFSET;
	dw_conv_params.output_offset = STRIDE2PAD1_OUTPUT_OFFSET;
	dw_conv_params.activation.min = STRIDE2PAD1_OUT_ACTIVATION_MIN;
	dw_conv_params.activation.max = STRIDE2PAD1_OUT_ACTIVATION_MAX;
	quant_params.multiplier = (int32_t *)stride2pad1_output_mult;
	quant_params.shift = (int32_t *)stride2pad1_output_shift;

	ctx.buf = NULL;
	ctx.size = 0;

	arm_cmsis_nn_status result = arm_depthwise_conv_s8(&ctx,
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
	zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
	zassert_mem_equal(stride2pad1_output_ref, output, sizeof(output), "");
}

#define FULLY_CONNECTED_MVE_0_OUT_CH		 9
#define FULLY_CONNECTED_MVE_0_IN_CH		 16
#define FULLY_CONNECTED_MVE_0_INPUT_W		 1
#define FULLY_CONNECTED_MVE_0_INPUT_H		 1
#define FULLY_CONNECTED_MVE_0_DST_SIZE		 9
#define FULLY_CONNECTED_MVE_0_INPUT_SIZE	 16
#define FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MIN -128
#define FULLY_CONNECTED_MVE_0_OUT_ACTIVATION_MAX 127
#define FULLY_CONNECTED_MVE_0_INPUT_BATCHES	 1
#define FULLY_CONNECTED_MVE_0_OUTPUT_MULTIPLIER	 1244038257
#define FULLY_CONNECTED_MVE_0_OUTPUT_SHIFT	 -9
#define FULLY_CONNECTED_MVE_0_ACCUMULATION_DEPTH 16
#define FULLY_CONNECTED_MVE_0_INPUT_OFFSET	 128
#define FULLY_CONNECTED_MVE_0_OUTPUT_OFFSET	 -26

const int32_t fully_connected_mve_0_biases[9] = {11295, -30752, -3196, 10489, -5120,
						 18598, 27393,	29746, 22967};

const int8_t fully_connected_mve_0_input[16] = {-43, 68,  79,	-12, -119, -56, -102, -46,
						107, -65, -109, -7,  92,   -99, -80,  -29};

const int8_t fully_connected_mve_0_output_ref[9] = {-9, -3, 26, 8, 3, -88, 75, 34, 5};

const int8_t fully_connected_mve_0_weights[144] = {
	37,  -46,  75,	 -33,  -52, -82,  -94,	64,   71,  65,	 64,  16,   -66, -5,   -65,  -44,
	82,  42,   84,	 105,  18,  79,	  -103, -75,  -95, 65,	 87,  103,  43,	 -25,  -66,  75,
	125, 40,   -34,	 24,   9,   -79,  4,	73,   98,  -75,	 42,  81,   18,	 -58,  -119, 92,
	0,   -72,  48,	 23,   -69, 11,	  -95,	-103, 66,  117,	 107, -96,  114, -29,  75,   -93,
	118, 66,   -19,	 83,   -14, 86,	  -110, 44,   37,  -9,	 17,  -107, 50,	 -116, -116, -27,
	-84, -126, -108, -127, -71, 8,	  81,	108,  -61, 126,	 69,  -45,  37,	 -78,  -102, -55,
	116, 112,  -111, -89,  -57, 82,	  -47,	22,   125, -84,	 97,  -9,   88,	 74,   -15,  118,
	-95, 112,  89,	 44,   -17, -112, -71,	-94,  1,   -117, 112, -92,  52,	 57,   -22,  80,
	-60, 95,   -106, -1,   -27, 105,  6,	123,  6,   96,	 126, -65,  -29, 103,  19,   -45};

ZTEST(cmsis_nn, test_fully_connected)
{
	int8_t output[FULLY_CONNECTED_MVE_0_DST_SIZE] = {0};

	cmsis_nn_context ctx;
	cmsis_nn_fc_params fc_params;
	cmsis_nn_per_tensor_quant_params quant_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_dims output_dims;

	const int32_t *bias_data = fully_connected_mve_0_biases;
	const int8_t *kernel_data = fully_connected_mve_0_weights;
	const int8_t *input_data = fully_connected_mve_0_input;

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
	arm_cmsis_nn_status result = arm_fully_connected_s8(&ctx,
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
	zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
	zassert_mem_equal(fully_connected_mve_0_output_ref, output, sizeof(output), "");
}

#define MAXPOOLING_2_OUT_CH		5
#define MAXPOOLING_2_IN_CH		5
#define MAXPOOLING_2_INPUT_W		12
#define MAXPOOLING_2_INPUT_H		1
#define MAXPOOLING_2_DST_SIZE		60
#define MAXPOOLING_2_INPUT_SIZE		60
#define MAXPOOLING_2_OUT_ACTIVATION_MIN -128
#define MAXPOOLING_2_OUT_ACTIVATION_MAX 127
#define MAXPOOLING_2_INPUT_BATCHES	1
#define MAXPOOLING_2_FILTER_X		3
#define MAXPOOLING_2_FILTER_Y		1
#define MAXPOOLING_2_STRIDE_X		1
#define MAXPOOLING_2_STRIDE_Y		2
#define MAXPOOLING_2_PAD_X		1
#define MAXPOOLING_2_PAD_Y		0
#define MAXPOOLING_2_OUTPUT_W		12
#define MAXPOOLING_2_OUTPUT_H		1

const int8_t maxpooling_2_input[60] = {
	75,  -52,  -42,	 -30, 56,  64,	 106, -36, 120, -3,  34,   -105, 69,   75,  -39,
	15,  93,   -71,	 39,  34,  -11,	 65,  22,  59,	106, 105,  45,	 -116, -75, 123,
	-65, 75,   -61,	 13,  -25, -123, 59,  110, -65, 86,  -108, -107, -17,  38,  27,
	-1,  -115, -123, 75,  -75, 68,	 52,  12,  -35, 116, -68,  22,	 15,   76,  -81};

const int8_t maxpooling_2_output_ref[60] = {
	75,  106, -36, 120, 56,	 75,  106, 69,	120, 56,  64,  106, 69,	 120, 34,
	34,  93,  69,  75,  106, 105, 93,  22,	59,  123, 105, 75,  22,	 59,  123,
	105, 75,  110, 13,  123, -65, 75,  110, 38,  86,  -1,  59,  110, 75,  86,
	68,  52,  12,  75,  116, 68,  52,  15,	76,  116, 68,  52,  15,	 76,  116};

ZTEST(cmsis_nn, test_max_pool)
{
	int8_t output[MAXPOOLING_2_DST_SIZE] = {0};

	cmsis_nn_context ctx;
	cmsis_nn_pool_params pool_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;

	const int8_t *input_data = maxpooling_2_input;

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
		arm_cmsis_nn_status result =
			arm_max_pool_s8(&ctx, &pool_params, &input_dims, input_data, &filter_dims,
					&output_dims, output);

		zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
		zassert_mem_equal(maxpooling_2_output_ref, output, sizeof(output), "");
	}
}

#define SOFTMAX_NUM_ROWS	 2
#define SOFTMAX_ROW_SIZE	 5
#define SOFTMAX_INPUT_MULT	 1077952640
#define SOFTMAX_INPUT_LEFT_SHIFT 19
#define SOFTMAX_DIFF_MIN	 -3968
#define SOFTMAX_DST_SIZE	 10

const int8_t softmax_input[10] = {101, 49, 6, -34, -75, -79, -38, 120, -55, 115};

const int8_t softmax_output_ref[10] = {-57, -70, -79, -86, -92, -94, -88, -54, -91, -56};

ZTEST(cmsis_nn, test_softmax)
{
	const int32_t num_rows = SOFTMAX_NUM_ROWS;
	const int32_t row_size = SOFTMAX_ROW_SIZE;
	const int32_t mult = SOFTMAX_INPUT_MULT;
	const int32_t shift = SOFTMAX_INPUT_LEFT_SHIFT;
	const int32_t diff_min = SOFTMAX_DIFF_MIN;
	const int8_t *input_data = softmax_input;
	int8_t output[SOFTMAX_DST_SIZE];

	for (int i = 0; i < REPEAT_NUM; i++) {
		arm_softmax_s8(input_data, num_rows, row_size, mult, shift, diff_min, output);
		zassert_mem_equal(softmax_output_ref, output, sizeof(output), "");
	}
}

#define SVDF_2_MULTIPLIER_IN	  1717987072
#define SVDF_2_MULTIPLIER_OUT	  1099511552
#define SVDF_2_SHIFT_1		  -3
#define SVDF_2_SHIFT_2		  -11
#define SVDF_2_IN_ACTIVATION_MIN  -32768
#define SVDF_2_IN_ACTIVATION_MAX  32767
#define SVDF_2_RANK		  2
#define SVDF_2_FEATURE_BATCHES	  10
#define SVDF_2_TIME_BATCHES	  2
#define SVDF_2_INPUT_SIZE	  7
#define SVDF_2_DST_SIZE		  15
#define SVDF_2_OUT_ACTIVATION_MIN -128
#define SVDF_2_OUT_ACTIVATION_MAX 127
#define SVDF_2_INPUT_BATCHES	  3
#define SVDF_2_INPUT_OFFSET	  0
#define SVDF_2_OUTPUT_OFFSET	  0

const int32_t svdf_2_biases[5] = {0, 0, 0, 0, 0};

const int16_t svdf_2_state[60] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int8_t svdf_2_weights_feature[70] = {
	27,   82,  -108, -127, 85,  3,	 -51, 32,  110, -6,  -14, -16,	31,  101,
	-122, 19,  76,	 74,   -80, 12,	 -22, -17, 10,	-28, 55,  109,	2,   -107,
	-4,   72,  -65,	 -59,  36,  -69, 105, -97, 25,	38,  110, -121, -88, -126,
	-14,  16,  -88,	 -66,  3,   -93, 69,  -64, 44,	103, 95,  -95,	68,  -46,
	106,  -31, -63,	 23,   -38, 36,	 -95, -43, 93,	77,  91,  -26,	33,  59};

const int16_t svdf_2_weights_time[20] = {-31, -88, -10, -72, -119, -6, -70, 63,	 -10, 93,
					 5,   42,  -6,	22,  6,	   51, 37,  -38, 5,   117};

const int8_t svdf_2_input_sequence[42] = {
	29,   81,   -38, 17,  -116, 43,	 119,  -127, 74,  115, 9,    118,  7,	 -56,
	-53,  -14,  -98, 60,  -128, 10,	 28,   -18,  12,  -28, -126, 87,   -115, -44,
	-123, -109, -59, -87, -69,  121, -128, -95,  -70, 2,   81,   -119, 84,	 -122};

const int8_t svdf_2_output_ref[15] = {-53, 45,	27, -24, -53, 26, -82, -38,
				      11,  -85, 94, -16, -32, 31, 4};

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

	const int8_t *weights_feature_data = svdf_2_weights_feature;
	const int16_t *weights_time_data = svdf_2_weights_time;

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
	int16_t *state_data = malloc(sizeof(svdf_2_state));
	const bool null_bias = check_null_bias(svdf_2_biases,
					       SVDF_2_DST_SIZE / SVDF_2_INPUT_BATCHES);

	for (int i = 0; i < REPEAT_NUM; i++) {
		memcpy(state_data, svdf_2_state, sizeof(svdf_2_state));
		for (int j = 0; j < number_inputs; j++) {
			memcpy(input_data, svdf_2_input_sequence + j * input_round_size,
			       input_round_size);
			arm_cmsis_nn_status result = arm_svdf_state_s16_s8(&input_ctx,
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
			zassert_equal(ARM_CMSIS_NN_SUCCESS, result, "");
		}

		zassert_mem_equal(svdf_2_output_ref, output_data, sizeof(output_data), "");
	}
	free(state_data);
	free(input_data);
	free(input_ctx.buf);
	free(output_ctx.buf);
}

ZTEST_SUITE(cmsis_nn, NULL, NULL, NULL, NULL, NULL);
