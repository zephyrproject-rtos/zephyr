/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/cordic.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <math.h>

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>

#include <zephyr/drivers/dma.h>
#include <zephyr/devicetree/dma.h>

#include <stm32_ll_cordic.h>

#include <test_cordic_api_helpers.h>


#define CORDIC_NODE	DT_NODELABEL(cordic)
#define CORDIC_LL_DEV	((CORDIC_TypeDef *)DT_REG_ADDR(CORDIC_NODE))

static const struct device *const cordic_dma_wr_dev = \
	DEVICE_DT_GET_OR_NULL(DT_DMAS_CTLR_BY_NAME(CORDIC_NODE, cordic_write));
static const uint32_t cordic_dma_wr_channel = DT_DMAS_CELL_BY_NAME_OR(CORDIC_NODE, cordic_write, channel, 0);
static const uint32_t cordic_dma_wr_slot = DT_DMAS_CELL_BY_NAME_OR(CORDIC_NODE, cordic_write, slot, 0);

static const struct device *const cordic_dma_rd_dev = \
	DEVICE_DT_GET_OR_NULL(DT_DMAS_CTLR_BY_NAME(CORDIC_NODE, cordic_read));
static const uint32_t cordic_dma_rd_channel = DT_DMAS_CELL_BY_NAME_OR(CORDIC_NODE, cordic_read, channel, 0);
static const uint32_t cordic_dma_rd_slot = DT_DMAS_CELL_BY_NAME_OR(CORDIC_NODE, cordic_read, slot, 0);

static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);
static atomic_t test_cordic_read_result_callback_call_cnt = ATOMIC_INIT(0);

#define RESULTS_FROM_CALLBACK_LEN	2  /* Max number of results per CORDIC function */
static int results_from_callback[RESULTS_FROM_CALLBACK_LEN];

/* The value of the error is the smallest value satisfying error over all the tests */
#define RESIDUAL_ERROR	((double)1e-4)


void test_cordic_read_result_callback(const struct device *dev,
									  const int *result,
									  const size_t result_len)
{
	ARG_UNUSED(dev);

	for(size_t i = 0; i < result_len && i < RESULTS_FROM_CALLBACK_LEN; i++) {
		results_from_callback[i] = result[i];
	}

	atomic_inc(&test_cordic_read_result_callback_call_cnt);
}


static uint32_t ll_scale2scale(const uint32_t ll_scale) {
	uint32_t scale = 0;

	switch(ll_scale) {
		case LL_CORDIC_SCALE_0:
			scale = CORDIC_DATA_SCALE_0;
			break;
		case LL_CORDIC_SCALE_1:
			scale = CORDIC_DATA_SCALE_1;
			break;
		case LL_CORDIC_SCALE_2:
			scale = CORDIC_DATA_SCALE_2;
			break;
		case LL_CORDIC_SCALE_3:
			scale = CORDIC_DATA_SCALE_3;
			break;
		case LL_CORDIC_SCALE_4:
			scale = CORDIC_DATA_SCALE_4;
			break;
		case LL_CORDIC_SCALE_5:
			scale = CORDIC_DATA_SCALE_5;
			break;
		case LL_CORDIC_SCALE_6:
			scale = CORDIC_DATA_SCALE_6;
			break;
		case LL_CORDIC_SCALE_7:
			scale = CORDIC_DATA_SCALE_7;
			break;
		default:
			scale = CORDIC_DATA_SCALE_0; // invalid value
			break;
	}

	return scale;
}

/**
 * @brief Perform CORDIC conversion from float to fixed point.
 * This is a helper function considering specific mapping
 * of CORDIC functions to results and their formats.
 */
static int stm32_cordic_float2fixed(const float *arg_in,
								    int *arg_out,
								    const size_t arg_in_len,
								    const size_t arg_out_len)
{
	const uint32_t ll_fn = LL_CORDIC_GetFunction(CORDIC_LL_DEV);
	const uint32_t ll_scale = LL_CORDIC_GetScale(CORDIC_LL_DEV);
	int ret = 0;

	float arg_in_tmp[arg_in_len];
	for(int i = 0; i < arg_in_len; i++) {
		arg_in_tmp[i] = arg_in[i];
	}

	switch(ll_fn) {
		case LL_CORDIC_FUNCTION_MODULUS:
			break;
		case LL_CORDIC_FUNCTION_PHASE:
			break;
		case LL_CORDIC_FUNCTION_COSINE:
		case LL_CORDIC_FUNCTION_SINE:
			arg_in_tmp[0] /= PI_F32;
			break;
		case LL_CORDIC_FUNCTION_SQUAREROOT:
			break;
		case LL_CORDIC_FUNCTION_HARCTANGENT:
		case LL_CORDIC_FUNCTION_ARCTANGENT:
		case LL_CORDIC_FUNCTION_NATURALLOG:
		case LL_CORDIC_FUNCTION_HCOSINE:
		case LL_CORDIC_FUNCTION_HSINE:
			for(int i = 0; i < arg_in_len; i++) {
				/* Divide by 2^scale */
				arg_in_tmp[i] /= (1 << ll_scale2scale(ll_scale));
			}
			break;
		default:
			return -EINVAL;
	}

	if(LL_CORDIC_GetOutSize(CORDIC_LL_DEV) == LL_CORDIC_INSIZE_32BITS) {
		for(int i = 0; i < arg_in_len; i++) {
			arg_out[i] = zdsp_f32_to_q31_shift(arg_in_tmp[i], 0);
		}
	} else {
		for(int i = 0; i < arg_in_len; i++) {
			arg_out[i] = zdsp_f32_to_q15_shift(arg_in_tmp[i], 0);
		}
	}

	switch(ll_fn) {
		case LL_CORDIC_FUNCTION_MODULUS:
			break;
		case LL_CORDIC_FUNCTION_PHASE:
			break;
		case LL_CORDIC_FUNCTION_COSINE:
			break;
		case LL_CORDIC_FUNCTION_SINE:
			break;
		case LL_CORDIC_FUNCTION_ARCTANGENT:
			break;
		case LL_CORDIC_FUNCTION_HARCTANGENT:
			break;
		case LL_CORDIC_FUNCTION_NATURALLOG:
			break;
		case LL_CORDIC_FUNCTION_SQUAREROOT:
			break;
		case LL_CORDIC_FUNCTION_HCOSINE:
			break;
		case LL_CORDIC_FUNCTION_HSINE:
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

/**
 * @brief Perform CORDIC conversion from fixed point to float.
 * This is a helper function considering specific mapping
 * of CORDIC functions to results and their formats.
 */
static int stm32_cordic_fixed2float(const int *arg_in,
								    float *arg_out,
								    const size_t arg_in_len,
								    const size_t arg_out_len)
{
	const uint32_t ll_fn = LL_CORDIC_GetFunction(CORDIC_LL_DEV);
	const uint32_t ll_scale = LL_CORDIC_GetScale(CORDIC_LL_DEV);
	int ret = 0;

	int arg_in_tmp[arg_in_len];
	for(int i = 0; i < arg_in_len; i++) {
		arg_in_tmp[i] = arg_in[i];
	}

	switch(ll_fn) {
		case LL_CORDIC_FUNCTION_MODULUS:
			break;
		case LL_CORDIC_FUNCTION_PHASE:
			break;
		case LL_CORDIC_FUNCTION_COSINE:
			break;
		case LL_CORDIC_FUNCTION_SINE:
			break;
		case LL_CORDIC_FUNCTION_ARCTANGENT:
			break;
		case LL_CORDIC_FUNCTION_HARCTANGENT:
			break;
		case LL_CORDIC_FUNCTION_NATURALLOG:
			break;
		case LL_CORDIC_FUNCTION_SQUAREROOT:
			break;
		case LL_CORDIC_FUNCTION_HCOSINE:
			break;
		case LL_CORDIC_FUNCTION_HSINE:
			break;
		default:
			return -EINVAL;
	}

	if(LL_CORDIC_GetOutSize(CORDIC_LL_DEV) == LL_CORDIC_INSIZE_32BITS) {
		for(int i = 0; i < arg_in_len; i++) {
			arg_out[i] = zdsp_q31_to_f32_shift(arg_in_tmp[i], 0);
		}
	} else {
		for(int i = 0; i < arg_in_len; i++) {
			arg_out[i] = zdsp_q15_to_f32_shift(arg_in_tmp[i], 0);
		}
	}

	switch(ll_fn) {
		case LL_CORDIC_FUNCTION_MODULUS:
			arg_out[1] = PI_F32 * arg_out[1];
			break;
		case LL_CORDIC_FUNCTION_PHASE:
			arg_out[0] = PI_F32 * arg_out[0];
			break;
		case LL_CORDIC_FUNCTION_COSINE:
			break;
		case LL_CORDIC_FUNCTION_SINE:
			break;
		case LL_CORDIC_FUNCTION_ARCTANGENT:
			for(int i = 0; i < arg_in_len; i++) {
				/* Multiply by 2^scale */
				arg_out[i] = PI_F32 * arg_out[i] * (1 << ll_scale2scale(ll_scale));
			}
			break;
		case LL_CORDIC_FUNCTION_NATURALLOG:
			for(int i = 0; i < arg_in_len; i++) {
				/* Multiply by 2^(scale+1) */
				arg_out[i] = arg_out[i] * (1 << (ll_scale2scale(ll_scale) + 1));
			}
			break;
		case LL_CORDIC_FUNCTION_SQUAREROOT:
			break;
		case LL_CORDIC_FUNCTION_HARCTANGENT:
		case LL_CORDIC_FUNCTION_HCOSINE:
		case LL_CORDIC_FUNCTION_HSINE:
			for(int i = 0; i < arg_in_len; i++) {
				/* Multiply by 2^scale */
				arg_out[i] = arg_out[i] * (1 << ll_scale2scale(ll_scale));
			}
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

static void before_fn(void* data)
{
	ARG_UNUSED(data);
	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
}

static void after_fn(void* data)
{
	ARG_UNUSED(data);
}

static void run_test_case(const float *args_in,
						  const size_t args_len,
						  const float *exp_res,
						  const size_t res_len,
						  const double residual_error)
{
	int args[args_len];
	int res[res_len];
	float res_float[res_len];
	int ret = 0;

	ret = stm32_cordic_float2fixed(args_in, args, args_len, args_len);
	zassert_equal(ret, 0, "CORDIC stm32_cordic_float2fixed failed: %d", ret);

	ret = cordic_compute(cordic_dev, args, args_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	ret = cordic_read_result(cordic_dev, res, res_len);
	zassert_equal(ret, 0, "cordic_read_result failed: %d", ret);

	ret = stm32_cordic_fixed2float(res, res_float, res_len, res_len);
	zassert_equal(ret, 0, "CORDIC stm32_cordic_fixed2float failed: %d", ret);

	for(int j = 0; j < res_len; j++) {
		zassert_true(cordic_float_close(res_float[j], exp_res[j], residual_error),
			"CORDIC result out of range: val: %f expected: %f residual error: %f",
			(double)res_float[j], (double)exp_res[j], (double)residual_error);
	}
}

/**
 * @brief Test CORDIC Configure function. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_function)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.scale = CORDIC_DATA_SCALE_3,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_function fn_list[] = {
		CORDIC_FUNC_COSINE,
		CORDIC_FUNC_SINE,
		CORDIC_FUNC_PHASE,
		CORDIC_FUNC_MODULUS,
		CORDIC_FUNC_ARCTANGENT,
		CORDIC_FUNC_HCOSINE,
		CORDIC_FUNC_HSINE,
		CORDIC_FUNC_HARCTANGENT,
		CORDIC_FUNC_NATURALLOG,
		CORDIC_FUNC_SQUAREROOT,
	};

	const uint32_t mapped_fn_list[] = {
		LL_CORDIC_FUNCTION_COSINE,
		LL_CORDIC_FUNCTION_SINE,
		LL_CORDIC_FUNCTION_PHASE,
		LL_CORDIC_FUNCTION_MODULUS,
		LL_CORDIC_FUNCTION_ARCTANGENT,
		LL_CORDIC_FUNCTION_HCOSINE,
		LL_CORDIC_FUNCTION_HSINE,
		LL_CORDIC_FUNCTION_HARCTANGENT,
		LL_CORDIC_FUNCTION_NATURALLOG,
		LL_CORDIC_FUNCTION_SQUAREROOT,
	};

	/* Iterate through all possible function configurations */
	for(int i = 0; i < ARRAY_SIZE(fn_list); i++) {
		config.function = fn_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "%s: CORDIC function configure failed: %d",
				cordic_dev->name, ret);
		const uint32_t ll_fn = LL_CORDIC_GetFunction(CORDIC_LL_DEV);
		zassert_equal(ll_fn, mapped_fn_list[i],
				"%s: CORDIC function 0x%x != 0x%x (expected_fn != current_fn)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, mapped_fn_list[i], ll_fn,
				CORDIC_LL_DEV->CSR);

		/* Verify number of argumernts an results */
		const uint32_t ll_nargs = LL_CORDIC_GetNbWrite(CORDIC_LL_DEV);
		const uint32_t ll_nres = LL_CORDIC_GetNbRead(CORDIC_LL_DEV);
		const uint32_t ll_in_width = LL_CORDIC_GetInSize(CORDIC_LL_DEV);
		const uint32_t ll_out_width = LL_CORDIC_GetOutSize(CORDIC_LL_DEV);

		const uint32_t ll_scale = LL_CORDIC_GetScale(CORDIC_LL_DEV);
		zassert_equal(ll_scale, LL_CORDIC_SCALE_3,
				"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
				cordic_dev->name, LL_CORDIC_SCALE_3, ll_scale);


		if(ll_fn == CORDIC_FUNC_COSINE || ll_fn == CORDIC_FUNC_SINE ||
		   ll_fn == CORDIC_FUNC_PHASE || ll_fn == CORDIC_FUNC_MODULUS) {
			const uint32_t nargs_expected = ll_in_width == LL_CORDIC_INSIZE_32BITS ?
							LL_CORDIC_NBWRITE_2 : LL_CORDIC_NBWRITE_1;
			const uint32_t nres_expected = ll_out_width == LL_CORDIC_OUTSIZE_32BITS ?
						       LL_CORDIC_NBREAD_2 : LL_CORDIC_NBREAD_1;
			zassert_equal(ll_nargs, nargs_expected,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nargs_expected, ll_nargs,
				CORDIC_LL_DEV->CSR);
			zassert_equal(ll_nres, nres_expected,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nres_expected, ll_nres,
				CORDIC_LL_DEV->CSR);

		} else if(ll_fn == CORDIC_FUNC_ARCTANGENT || ll_fn == CORDIC_FUNC_HARCTANGENT ||
			  ll_fn == CORDIC_FUNC_NATURALLOG || ll_fn == CORDIC_FUNC_SQUAREROOT) {
			zassert_equal(ll_nargs, LL_CORDIC_NBWRITE_1,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBWRITE_1, ll_nargs,
				CORDIC_LL_DEV->CSR);
			zassert_equal(ll_nres, LL_CORDIC_NBREAD_1,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBREAD_1, ll_nres,
				CORDIC_LL_DEV->CSR);
		} else {
			zassert_equal(ll_nargs, LL_CORDIC_NBWRITE_1,
				"%s: CORDIC nargs 0x%x != 0x%x (expected_nargs != current_nargs)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, LL_CORDIC_NBWRITE_1, ll_nargs,
				CORDIC_LL_DEV->CSR);
			const uint32_t nres_expected = ll_out_width == LL_CORDIC_OUTSIZE_32BITS ?
						       LL_CORDIC_NBREAD_2 : LL_CORDIC_NBREAD_1;
			zassert_equal(ll_nres, nres_expected,
				"%s: CORDIC nres 0x%x != 0x%x (expected_nres != current_nres)"
				": CORDIC.CSR: 0x%x",
				cordic_dev->name, nres_expected, ll_nres,
				CORDIC_LL_DEV->CSR);
	}
	}
}

/**
 * @brief Test CORDIC Configure precision. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_precision)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_precision prec_list[] = {
		CORDIC_PRECISION_1,
		CORDIC_PRECISION_2,
		CORDIC_PRECISION_3,
		CORDIC_PRECISION_4,
		CORDIC_PRECISION_5,
		CORDIC_PRECISION_6,
		CORDIC_PRECISION_7,
		CORDIC_PRECISION_8,
		CORDIC_PRECISION_9,
		CORDIC_PRECISION_10,
		CORDIC_PRECISION_11,
		CORDIC_PRECISION_12,
		CORDIC_PRECISION_13,
		CORDIC_PRECISION_14,
		CORDIC_PRECISION_4,
	};

	const uint32_t mapped_prec_list[] = {
		LL_CORDIC_PRECISION_1CYCLE,
		LL_CORDIC_PRECISION_2CYCLES,
		LL_CORDIC_PRECISION_3CYCLES,
		LL_CORDIC_PRECISION_4CYCLES,
		LL_CORDIC_PRECISION_5CYCLES,
		LL_CORDIC_PRECISION_6CYCLES,
		LL_CORDIC_PRECISION_7CYCLES,
		LL_CORDIC_PRECISION_8CYCLES,
		LL_CORDIC_PRECISION_9CYCLES,
		LL_CORDIC_PRECISION_10CYCLES,
		LL_CORDIC_PRECISION_11CYCLES,
		LL_CORDIC_PRECISION_12CYCLES,
		LL_CORDIC_PRECISION_13CYCLES,
		LL_CORDIC_PRECISION_14CYCLES,
		LL_CORDIC_PRECISION_4CYCLES,
	};

	/* Iterate through all possible precision configurations */
	for(int i = 0; i < ARRAY_SIZE(prec_list); i++) {
		config.precision = prec_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC precision configure failed: %d", ret);
		const uint32_t ll_prec = LL_CORDIC_GetPrecision(CORDIC_LL_DEV);
		zassert_equal(ll_prec, mapped_prec_list[i],
				"%s: CORDIC precision 0x%x != 0x%x (expected_prec != current_prec)",
				cordic_dev->name, mapped_prec_list[i], ll_prec);
	}
}

/**
 * @brief Test CORDIC Configure data width. This test is allowed to apply some
 * arbitrary configuration.
 */
ZTEST(stm32_cordic, test_cordic_configure_data_width)
{
	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};
	int ret = 0;

	const enum cordic_data_width data_width_list[] = {
		CORDIC_DATA_WIDTH_32BIT,
		CORDIC_DATA_WIDTH_16BIT,
	};

	const uint32_t mapped_in_width_list[] = {
		LL_CORDIC_INSIZE_32BITS,
		LL_CORDIC_INSIZE_16BITS,
	};

	/* Iterate through all possible input data width configurations for
	 * input size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.in_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC input data width configure failed: %d", ret);
		const uint32_t ll_in_width = LL_CORDIC_GetInSize(CORDIC_LL_DEV);
		zassert_equal(ll_in_width, mapped_in_width_list[i],
				"%s: CORDIC input size 0x%x != 0x%x (expected_in_width != current_in_width)",
				cordic_dev->name, mapped_in_width_list[i], ll_in_width);
	}

	const uint32_t mapped_out_width_list[] = {
		LL_CORDIC_OUTSIZE_32BITS,
		LL_CORDIC_OUTSIZE_16BITS,
	};

	/* Iterate through all possible input data width configurations for
	 * output size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.out_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC output data width configure failed: %d", ret);
		const uint32_t ll_out_width = LL_CORDIC_GetOutSize(CORDIC_LL_DEV);
		zassert_equal(ll_out_width, mapped_out_width_list[i],
				"%s: CORDIC output size 0x%x != 0x%x (expected_out_width != current_out_width)",
				cordic_dev->name, mapped_out_width_list[i], ll_out_width);
	}
}

/**
 * @brief Test CORDIC Phase function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_phase_q31)
{
	/* Input arguments */
	const float input_args[][2] = {
		{1.0f, 0.0f},
		{0.5f, 0.5f},
		{0.5f, 0.25f},
	}; // {x, y} coordinates in radians

	/* Expected results */
	const float exp_res[][2] = {
		{0.0f, 1.0f},
		{0.785398f, 0.707107f},
		{0.463648f, 0.559017f},
	}; // {atan2(y,x), modulus: m = sqrt(x^2 + y^2)}

	/* Q31 numbers format */
	struct cordic_config config = {
		.function = CORDIC_FUNC_PHASE,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Phase function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_phase_q15)
{
	/* Input arguments */
	const float input_args[][2] = {
		{1.0f, 0.0f},
		{0.5f, 0.5f},
		{0.5f, 0.25f},
	}; // {x, y} coordinates in radians

	/* Expected results */
	const float exp_res[][2] = {
		{0.0f, 1.0f},
		{0.785398f, 0.707107f},
		{0.463648f, 0.559017f},
	}; // {atan2(y,x), modulus: m = sqrt(x^2 + y^2)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_PHASE,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Modulus and Phase functions: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_modulus_q31)
{
	const float input_args[][2] = {
		{0.456f, 0.123f},
		{0.123f, 0.456f},
		{1.0f, 0.0f},
		{0.0f, 1.0f},
	}; // {x, y}
	const float exp_res[][2] = {
		{0.472298f, 0.26346653f},
		{0.472298f, 1.3073297f},
		{1.0f, 0.0f},
		{1.0f, 1.57079633f},
	}; // {modulus: m = sqrt(x^2 + y^2), phase angle: atan2(y,x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_MODULUS,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Modulus and Phase functions: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_modulus_q15)
{
	const float input_args[][2] = {
		{0.456f, 0.123f},
		{0.123f, 0.456f},
		{1.0f, 0.0f},
		{0.0f, 1.0f},
	}; // {x, y}
	const float exp_res[][2] = {
		{0.472298f, 0.26346653f},
		{0.472298f, 1.3073297f},
		{1.0f, 0.0f},
		{1.0f, 1.57079633f},
	}; // {modulus: m = sqrt(x^2 + y^2), phase angle: atan2(y,x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_MODULUS,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Square Root function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_sqrt_q31)
{
	const float input_args[][2] = {
		{0.027f},
		{0.25f},
		{0.74f},
	};
	const float exp_res[][2] = {
		{0.16431676725154984f},
		{0.5f},
		{0.8602325267042626f}
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Square Root function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_sqrt_q15)
{
	const float input_args[][2] = {
		{0.027f},
		{0.25f},
		{0.74f},
	};
	const float exp_res[][2] = {
		{0.16431677f},
		{0.5f},
		{0.86023253f}
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Natural Logarithm function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_ln_q31)
{
	const float input_args[][2] = {
		{0.107f},
		{0.5f},
		{0.95f},
	};
	const float exp_res[][2] = {
		{-2.23492644452f},
		{-0.69314718f},
		{-0.051293f},
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_NATURALLOG,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Natural Logarithm function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_ln_q15)
{
	const float input_args[][2] = {
		{0.107f},
		{0.5f},
		{0.95f},
	};
	const float exp_res[][2] = {
		{-2.23492644452f},
		{-0.69314718f},
		{-0.051293f},
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_NATURALLOG,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Cosine function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_cosine_q31)
{
	const float input_args[][2] = {
		{PI_F32/2.0f, 0.25f},
		{PI_F32, 0.5f},
		{PI_F32/4.0f, 0.1f},
	}; // {x - angle in radians, y - modulus}
	const float exp_res[][2] = {
		{0.0f, 0.25f},
		{-0.5f, 0.0f},
		{0.0707107f, 0.0707107f},
	}; // {y * cos(x), y * sin(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_0,  /* not applicable */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Cosine function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_cosine_q15)
{
	const float input_args[][2] = {
		{PI_F32/2.0f, 0.25f},
		{PI_F32, 0.5f},
		{PI_F32/4.0f, 0.1f},
	}; // {x - angle in radians, y - modulus}
	const float exp_res[][2] = {
		{0.0f, 0.25f},
		{-0.5f, 0.0f},
		{0.0707107f, 0.0707107f},
	}; // {y * cos(x), y * sin(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_0,  /* not applicable */
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Sine function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_sine_q31)
{
	const float input_args[][2] = {
		{PI_F32/2.0f, 0.25f},
		{PI_F32, 0.5f},
		{PI_F32/4.0f, 0.1f},
	}; // {x - angle in radians, y - modulus}
	const float exp_res[][2] = {
		{0.25f, 0.0f},
		{0.0f, -0.5f},
		{0.0707107f, 0.0707107f},
	}; // {y * sin(x), y * cos(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_0,  /* not applicable */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Sine function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_sine_q15)
{
	const float input_args[][2] = {
		{PI_F32/2.0f, 0.25f},
		{PI_F32, 0.5f},
		{PI_F32/4.0f, 0.1f},
	}; // {x - angle in radians, y - modulus}
	const float exp_res[][2] = {
		{0.25f, 0.0f},
		{0.0f, -0.5f},
		{0.0707107f, 0.0707107f},
	}; // {y * sin(x), y * cos(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_0,  /* not applicable */
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 2, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic cosine function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_hcosine_q31)
{
	const float input_args[][2] = {
		{-0.559f},
		{0.25f},
		{0.559f},
	}; // {x}
	const float exp_res[][2] = {
		{1.16035163f, -0.58857107f},
		{1.0314131f, 0.25261232f},
		{1.16035163f, 0.58857107f},
	}; // {cosh(x), sinh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HCOSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,  /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic cosine function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_hcosine_q15)
{
	const float input_args[][2] = {
		{-0.559f},
		{0.25f},
		{0.559f},
	}; // {x}
	const float exp_res[][2] = {
		{1.16035163f, -0.58857107f},
		{1.0314131f, 0.25261232f},
		{1.16035163f, 0.58857107f},
	}; // {cosh(x), sinh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HCOSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,  /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic sine function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_hsine_q31)
{
	const float input_args[][2] = {
		{-0.559f},
		{0.25f},
		{0.559f},
	}; // {x}
	const float exp_res[][2] = {
		{-0.58857107f, 1.16035163f},
		{0.25261232f, 1.0314131f},
		{0.58857107f, 1.16035163f},
	}; // {sinh(x), cosh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,  /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic sine function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_hsine_q15)
{
	const float input_args[][2] = {
		{-0.559f},
		{0.25f},
		{0.559f},
	}; // {x}
	const float exp_res[][2] = {
		{-0.58857107f, 1.16035163f},
		{0.25261232f, 1.0314131f},
		{0.58857107f, 1.16035163f},
	}; // {sinh(x), cosh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HSINE,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,  /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 2, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Arctangent function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_arctangent_q31)
{
	const float input_args[][2] = {
		{-1.0f},
		{0.5f},
		{1.0f},
	}; // {x}
	const float exp_res[][2] = {
		{-0.78539816339f},
		{0.4636476090f},
		{0.78539816339f},
	}; // {atan(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_ARCTANGENT,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Arctangent function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_arctangent_q15)
{
	const float input_args[][2] = {
		{-1.0f},
		{0.5f},
		{1.0f},
	}; // {x}
	const float exp_res[][2] = {
		{-0.78539816339f},
		{0.4636476090f},
		{0.78539816339f},
	}; // {atan(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_ARCTANGENT,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1,
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic Arctangent function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_harctangent_q31)
{
	const float input_args[][2] = {
		{-0.403f},
		{-0.1f},
		{0.0f},
		{0.25f},
		{0.403f}
	}; // {x}
	const float exp_res[][2] = {
		{-0.42722548f},
		{-0.10033535f},
		{0.0f},
		{0.25541281f},
		{0.42722548f}
	}; // {atanh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HARCTANGENT,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1, /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC Hyperbolic Arctangent function: Q15 numbers format.
 */
ZTEST(stm32_cordic, test_harctangent_q15)
{
	const float input_args[][2] = {
		{-0.403f},
		{-0.1f},
		{0.0f},
		{0.25f},
		{0.403f}
	}; // {x}
	const float exp_res[][2] = {
		{-0.42722548f},
		{-0.10033535f},
		{0.0f},
		{0.25541281f},
		{0.42722548f}
	}; // {atanh(x)}

	struct cordic_config config = {
		.function = CORDIC_FUNC_HARCTANGENT,
		.precision = CORDIC_PRECISION_4,
		.scale = CORDIC_DATA_SCALE_1, /* the only scale is 1 */
		.in_width = CORDIC_DATA_WIDTH_16BIT,
		.out_width = CORDIC_DATA_WIDTH_16BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC read zero-overhead mode
 */
ZTEST(stm32_cordic, test_read_zero_overhead)
{
	const float input_args[][2] = {
		{0.25f},
	};
	const float exp_res[][2] = {
		{0.5f},
	};

	const struct cordic_config_options stm32_options = {
		.data_mode = STM32_CORDIC_DATA_MODE_ZERO_OVERHEAD,
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
		.options = &stm32_options,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC read polling mode
 */
ZTEST(stm32_cordic, test_read_polling)
{
	const float input_args[][2] = {
		{0.49f},
		{0.25f},
	};
	const float exp_res[][2] = {
		{0.7f},
		{0.5f},
	};

	const struct cordic_config_options stm32_options = {
		.data_mode = STM32_CORDIC_DATA_MODE_POLLING,
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
		.options = &stm32_options,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(input_args); i++) {
		run_test_case(input_args[i], 1, exp_res[i], 1, RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC read interrupt mode
 */
ZTEST(stm32_cordic, test_read_interrupt)
{
	const float input_args[][1] = {
		{0.16f},
	};
	const float exp_res[][1] = {
		{0.4f},
	};

	const struct cordic_config_options stm32_options = {
		.data_mode = STM32_CORDIC_DATA_MODE_INTERRUPT,
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_4,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
		.options = &stm32_options,
		.callback = test_cordic_read_result_callback,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	const size_t args_len = 1;
	const size_t res_len = 1;

	int args[args_len];
	int res[res_len];
	float res_float[res_len];

	ret = stm32_cordic_float2fixed(input_args[0], args, args_len, args_len);
	zassert_equal(ret, 0, "CORDIC stm32_cordic_float2fixed failed: %d", ret);

	atomic_set(&test_cordic_read_result_callback_call_cnt, 0);

	ret = cordic_compute(cordic_dev, args, args_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	k_msleep(10); /* wait for the callback to be called */

	/* Verify callback call count */
	const atomic_t cb_call_cnt = atomic_get(&test_cordic_read_result_callback_call_cnt);
	zassert_equal(cb_call_cnt, 1, "Expected callback call count beeing 1, but it is %ld", cb_call_cnt);

	/* Verify result from callback */
	ret = stm32_cordic_fixed2float(results_from_callback, res_float,
								   RESULTS_FROM_CALLBACK_LEN, res_len);
	zassert_equal(ret, 0, "CORDIC stm32_cordic_fixed2float failed: %d", ret);

	for(int j = 0; j < res_len; j++) {
		zassert_true(cordic_float_close(res_float[j], exp_res[0][j], RESIDUAL_ERROR),
			"CORDIC result out of range: val: %f expected: %f residual error: %f",
			(double)res_float[j], (double)exp_res[0][j], (double)RESIDUAL_ERROR);
	}

	/* Verify result by read function are still in registers */
	ret = cordic_read_result(cordic_dev, res, res_len);
	zassert_equal(ret, 0, "cordic_read_result failed: %d", ret);

	ret = stm32_cordic_fixed2float(res, res_float, res_len, res_len);
	zassert_equal(ret, 0, "CORDIC stm32_cordic_fixed2float failed: %d", ret);

	for(int j = 0; j < res_len; j++) {
		zassert_true(cordic_float_close(res_float[j], exp_res[0][j], RESIDUAL_ERROR),
			"CORDIC result out of range: val: %f expected: %f residual error: %f",
			(double)res_float[j], (double)exp_res[0][j], (double)RESIDUAL_ERROR);
	}
}

/**
 * @brief Test CORDIC DMA Memory-to-Memory Sine function: Q31 numbers format.
 */
ZTEST(stm32_cordic, test_dma_mem2mem_sine_q31)
{
#define nsamp	4  /* number of samples */
#define nargs	2  /* number of arguments per sample */

	const float input_args[nsamp][nargs] = {
		/* {x - angle in radians, y - modulus} */
		{PI_F32/2.00f, 0.25f},
		{PI_F32, 0.50f},
		{PI_F32/4.0f, 0.10f},
		{PI_F32/7.0f, 0.15f},
	};

	const float exp_res[nsamp][nargs] = {
		/* {y * sin(x), y * cos(x)} */
		{0.25f * 1.0f, 0.25f *  0.0f},
		{0.50f * 0.0f, 0.50f * -1.0f},
		{0.10f * 0.707107f, 0.10f * 0.707107f},
		{0.15f * 0.433883f, 0.15f * 0.900968f},
	};

	const struct cordic_config_options stm32_options = {
		.data_mode = STM32_CORDIC_DATA_MODE_DMA,
	};

	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_4,
		.options = &stm32_options,
		.scale = CORDIC_DATA_SCALE_0,  /* not applicable for sine */
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	int ret = cordic_configure(cordic_dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	static int args_buff[nsamp][nargs];
	static int res_buff[nsamp][nargs];
	float res_float[nsamp][nargs];

	for(size_t ns = 0; ns < nsamp; ns++) {
		ret = stm32_cordic_float2fixed(input_args[ns], args_buff[ns], nargs, nargs);
		zassert_equal(ret, 0, "CORDIC stm32_cordic_float2fixed failed: %d", ret);
	}

	/* Configure write DMA channel: memory -> CORDIC WDATA (M2P) */
	static struct dma_block_config cordic_dma_wr_block = {
		.source_address = (uint32_t)args_buff,
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,

		.dest_address    = (uintptr_t)&CORDIC_LL_DEV->WDATA,  /* TODO: cordic api shall have wdata phys address getter */
		.dest_addr_adj   = DMA_ADDR_ADJ_NO_CHANGE,

		.block_size = nsamp * nargs * 4,  /* 4 bytes each */

		.source_reload_en = 0U,
		.dest_reload_en   = 0U,
	};

	static struct dma_config dma_wr_cfg = {
		.dma_slot            = cordic_dma_wr_slot,
		.channel_direction   = MEMORY_TO_PERIPHERAL,
		.source_data_size    = 4U,
		.dest_data_size      = 4U,
		// .source_burst_length = 4U,
		// .dest_burst_length   = 4U,
		.block_count         = 1U,
		// .channel_priority    = 0U,
		.head_block          = &cordic_dma_wr_block,
		.user_data           = (void *)cordic_dma_wr_dev,
		// .dma_callback
		/* No completion callback: the read channel drives overall completion */
	};

	/* Configure read DMA channel: CORDIC RDATA -> memory (P2M) */
	static struct dma_block_config cordic_dma_rd_block = {
		.source_address = (uintptr_t)&CORDIC_LL_DEV->RDATA,  /* TODO: cordic api shall have rdata phys address getter */
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,

		.dest_address = (uint32_t)res_buff,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,

		// .dest_address    = (uintptr_t)&CORDIC_LL_DEV->WDATA,  /* TODO: cordic api shall have wdata phys address getter */
		// .dest_addr_adj   = DMA_ADDR_ADJ_NO_CHANGE,

		.block_size = nsamp * nargs * 4,  /* 2 words of 4 bytes each */

		.source_reload_en = 0U,
		.dest_reload_en   = 0U,
	};

	static struct dma_config dma_rd_cfg = {
		.dma_slot            = cordic_dma_rd_slot,
		.channel_direction   = PERIPHERAL_TO_MEMORY,
		.source_data_size    = 4U,
		.dest_data_size      = 4U,
		// .source_burst_length = 4U,
		// .dest_burst_length   = 4U,
		.block_count         = 1U,
		// .channel_priority    = 0U,
		.head_block          = &cordic_dma_rd_block,
		.user_data           = (void *)cordic_dma_rd_dev,
		// .dma_callback
		/* No completion callback: the read channel drives overall completion */
	};

	ret = dma_stop(cordic_dma_wr_dev, cordic_dma_wr_channel);
	zassert_equal(ret, 0, "%s: DMA write stop failed: %d", cordic_dma_wr_dev->name, ret);

	ret = dma_stop(cordic_dma_rd_dev, cordic_dma_rd_channel);
	zassert_equal(ret, 0, "%s: DMA read stop failed: %d", cordic_dma_rd_dev->name, ret);

	ret = dma_config(cordic_dma_rd_dev, cordic_dma_rd_channel, &dma_rd_cfg);
	zassert_equal(ret, 0, "%s: DMA read config failed: %d", cordic_dma_rd_dev->name, ret);

	ret = dma_config(cordic_dma_wr_dev, cordic_dma_wr_channel, &dma_wr_cfg);
	zassert_equal(ret, 0, "%s: DMA write config failed: %d", cordic_dma_wr_dev->name, ret);

	ret = dma_start(cordic_dma_rd_dev, cordic_dma_rd_channel);
	zassert_equal(ret, 0, "%s: DMA read start failed: %d", cordic_dma_rd_dev->name, ret);

	ret = dma_start(cordic_dma_wr_dev, cordic_dma_wr_channel);
	zassert_equal(ret, 0, "%s: DMA write start failed: %d", cordic_dma_wr_dev->name, ret);

	k_msleep(1); /* wait for the callback to be called */

	ret = dma_stop(cordic_dma_wr_dev, cordic_dma_wr_channel);
	zassert_equal(ret, 0, "%s: DMA write stop failed: %d", cordic_dma_wr_dev->name, ret);

	ret = dma_stop(cordic_dma_rd_dev, cordic_dma_rd_channel);
	zassert_equal(ret, 0, "%s: DMA read stop failed: %d", cordic_dma_rd_dev->name, ret);

	/* Convert results from fixed point format to floating point */
	for(size_t ns = 0; ns < nsamp; ns++) {
		ret = stm32_cordic_fixed2float(res_buff[ns], res_float[ns], nargs, nargs);
		zassert_equal(ret, 0, "CORDIC stm32_cordic_fixed2float failed: %d", ret);
	}

	/* Verify computed results against expected ones*/
	for(size_t ns = 0; ns < nsamp; ns++) {
		for(size_t na = 0; na < nargs; na++) {
			zassert_true(cordic_float_close(res_float[ns][na], exp_res[ns][na], RESIDUAL_ERROR),
				"CORDIC result (%d,%d) out of range: val: %f expected: %f residual error: %f",
				ns, na, (double)res_float[ns][na], (double)exp_res[ns][na], (double)RESIDUAL_ERROR);
		}
	}
}

ZTEST_SUITE(stm32_cordic, NULL, NULL, before_fn, after_fn, NULL);
