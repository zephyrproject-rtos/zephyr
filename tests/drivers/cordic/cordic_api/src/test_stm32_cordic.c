/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/cordic.h>
#include <zephyr/device.h>
#include <math.h>

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>

#include <stm32_ll_cordic.h>

#include "test_cordic_api_helpers.h"

#define CORDIC_NODE DT_NODELABEL(cordic)
#define CORDIC_LL_DEV ((CORDIC_TypeDef *)DT_REG_ADDR(CORDIC_NODE))

static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);

/* Test setup function */
static void *cordic_setup(void)
{
	zassert_true(device_is_ready(cordic_dev), "CORDIC device not ready");
	return NULL;
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
			zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
				"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
				cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
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

			if (ll_fn == CORDIC_FUNC_SQUAREROOT || ll_fn == CORDIC_FUNC_ARCTANGENT) {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
				continue;
			} else {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_1,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_1, ll_scale);
			}
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
			if (ll_fn == LL_CORDIC_FUNCTION_HCOSINE || ll_fn == LL_CORDIC_FUNCTION_HSINE) {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_1,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_1, ll_scale);
				continue;
			} else {
				zassert_equal(ll_scale, LL_CORDIC_SCALE_0,
					"%s: CORDIC scale 0x%x != 0x%x (expected_scale != current_scale)",
					cordic_dev->name, LL_CORDIC_SCALE_0, ll_scale);
			}
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
		CORDIC_PRECISION_15,
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
		LL_CORDIC_PRECISION_15CYCLES,
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
 * @brief Test CORDIC Q31 Phase function
 */
ZTEST(stm32_cordic, test_phase_q31)
{
	/* Input arguments */
	const float args0[] = {1.0f, 0.5f, 0.5f}; // x - coordinate in radians
	const float args1[] = {0.0f, 0.5f, 0.25f}; // y - coordinate in radians

	/* Expected results */
	const float exp_res0[] = {0.0f, 0.785398f, 0.463648f}; // atan2(y,x)
	const float exp_res1[] = {1.0f, 0.707107f, 0.559017f}; // modulus: m = sqrt(x^2 + y^2)

	struct cordic_config config = {
		.function = CORDIC_FUNC_PHASE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};

	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	for(int i = 0; i < ARRAY_SIZE(args1); i++) {
		int args[2] = {F32_TO_Q31(args0[i]), F32_TO_Q31(args1[i])};
		int res[2];
		const size_t args_len = ARRAY_SIZE(args);
		const size_t res_len = ARRAY_SIZE(res);

		ret = cordic_compute(dev, args, res, args_len, res_len);
		zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

		float float_res[2] = {Q31_TO_F32(res[0]), Q31_TO_F32(res[1])};

		ARG_UNUSED(exp_res0);
		ARG_UNUSED(exp_res1);
		ARG_UNUSED(float_res);

		// ret = cordic_int2float(dev, res, float_res, res_len, res_len);
		// zassert_equal(ret, 0, "CORDIC int2float failed: %d", ret);

		// zassert_true(cordic_float_close(float_res[0], exp_res0[i], TOLERANCE_F32),
		// 	"CORDIC phase result out of range: val: %f expected: %f tolerance: %f",
		// 	(double)float_res[0], (double)exp_res0[i], (double)TOLERANCE_F32);

		// zassert_true(cordic_float_close(float_res[1], exp_res1[i], TOLERANCE_F32),
		// 	"CORDIC modulus result out of range: val: %f expected: %f tolerance: %f",
		// 	(double)float_res[1], (double)exp_res1[i], (double)TOLERANCE_F32);
	}
}

/**
 * @brief Test CORDIC Modulus function
 *
 * Calculates sqrt(x^2 + y^2).
 */
/*
ZTEST(stm32_cordic, test_modulus)
{
	const float args1[] = {0.456f, 0.123f, 1.0f, 0.0f}; // x
	const float args2[] = {0.123f, 0.456f, 0.0f, 1.0f}; // y
	const float exp_res1[] = {0.472298f, 0.472298f, 1.0f, 1.0f}; // modulus: m = sqrt(x^2 + y^2)
	const float exp_res2[] = {0.26346654f, 1.3073297f, 0.0f, 1.57079633f}; // phase angle: atan2(y,x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_MODULUS,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "sqrt(x^2 + y^2)",
		.fn2_str = "atan2(y,x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Square Root function
 */
/*
ZTEST(stm32_cordic, test_squareroot)
{
	const float args1[] = {0.027f, 0.25f, 0.74f};
	const float exp_res1[] = {0.164317f, 0.5f, 0.86023f};

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_SQUAREROOT,
			.precision = CORDIC_PRECISION_2,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "sqrt(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Natural Logarithm function
 */
/*
ZTEST(stm32_cordic, test_natural_log)
{
	const float args1[] = {0.107f, 0.95f};
	const float exp_res1[] = {-2.2349267f, -0.051293f};

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_NATURALLOG,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = 0.1f,  // natural logarithm may have higher error
		.fn1_str = "ln(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Cosine function
 */
/*
ZTEST(stm32_cordic, test_cosine)
{
	const float args1[] = {PI_F32/2.0f, PI_F32, PI_F32/4.0f}; // x - angle in radians
	const float args2[] = {0.25f, 0.5f, 0.1f}; // y - modulus
	const float exp_res1[] = {0.0f, -0.5f, 0.0707107f}; // y * cos(x)
	const float exp_res2[] = {0.25f, 0.0f, 0.0707107f}; // y * sin(x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_COSINE,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "y * cos(x)",
		.fn2_str = "y * sin(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Sine function
 */
/*
ZTEST(stm32_cordic, test_sine)
{
	const float args1[] = {PI_F32/2.0f, PI_F32, PI_F32/4.0f}; // x - angle in radians
	const float args2[] = {0.25f, 0.5f, 0.1f}; // y - modulus
	const float exp_res1[] = {0.25f, 0.0f, 0.0707107f}; // y * sin(x)
	const float exp_res2[] = {0.0f, -0.5f, 0.0707107f}; // y * cos(x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_SINE,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "y * sin(x)",
		.fn2_str = "y * cos(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Hyperbolic Cosine function
 */
/*
ZTEST(stm32_cordic, test_hcosine)
{
	const float args1[] = {-0.559f, 0.25f, 0.559f}; // x
	const float exp_res1[] = {1.16035163f, 1.0314131f,  1.16035163f}; // cosh(x)
	const float exp_res2[] = {-0.58857107f, 0.25261232f, 0.58857107f}; // sinh(x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HCOSINE,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32 * 10.0f,  // lower tolerance since not precise
		.fn1_str = "cosh(x)",
		.fn2_str = "sinh(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Hyperbolic Sine function
 */
/*
ZTEST(stm32_cordic, test_hsine)
{
	const float args1[] = {-0.559f, 0.25f, 0.559f}; // x
	const float exp_res1[] = {-0.58857107f, 0.25261232f, 0.58857107f}; // sinh(x)
	const float exp_res2[] = {1.16035163f, 1.0314131f,  1.16035163f}; // cosh(x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HSINE,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = exp_res2,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32 * 10.0f,  // lower tolerance since not precise
		.fn1_str = "sinh(x)",
		.fn2_str = "cosh(x)",
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Arctangent function
 */
/*
ZTEST(stm32_cordic, test_arctangent)
{
	const float args1[] = {128.0f}; // x
	const float exp_res1[] = {1.562942f}; // atan(x)

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_ARCTANGENT,
			.precision = CORDIC_PRECISION_15,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = NULL,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "atan(x)",
		.fn2_str = NULL,
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

/**
 * @brief Test CORDIC Hyperbolic Arctangent function
 */
/*
ZTEST(stm32_cordic, test_harctangent)
{
	const float args1[] = {0.0f, 0.5f, 0.9f};
	const float args2[] = {0.0f, 0.0f, 0.0f};
	const float exp_res1[] = {0.0f, 0.5493061f, 1.4722193f};

	// A test for q31_t format
	cordic_run_config_t run_cfg = {
		.dev = DEVICE_DT_GET(CORDIC_NODE),
		.config = {
			.function = CORDIC_FUNC_HARCTANGENT,
			.precision = CORDIC_PRECISION_10,
			.in_width = CORDIC_DATA_WIDTH_32BIT,
			.out_width = CORDIC_DATA_WIDTH_32BIT,
		},
		.args1 = args1,
		.args2 = args2,
		.exp_res1 = exp_res1,
		.exp_res2 = NULL,
		.size = ARRAY_SIZE(args1),
		.tolerance = TOLERANCE_F32,
		.fn1_str = "atanh(x)",
		.fn2_str = NULL,
	};
	run_function_test(&run_cfg);

	// A test for q15_t format
	run_cfg.config.in_width = CORDIC_DATA_WIDTH_16BIT;
	run_cfg.config.out_width = CORDIC_DATA_WIDTH_16BIT;
	run_function_test(&run_cfg);
}
*/

ZTEST_SUITE(stm32_cordic, NULL, cordic_setup, NULL, NULL, NULL);
