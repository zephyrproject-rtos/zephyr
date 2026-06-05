/*
 * Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/cordic.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <math.h>

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/utils.h>

#include "test_cordic_api_helpers.h"

#define CORDIC_NODE DT_NODELABEL(cordic)

/**
 * @brief This test case tests general config and function calls without checking the results.
 * The main goal is to verify that the API calls are working and do not cause any errors.
 * The results of the calculations are not checked against expected values in this test case,
 * as the focus is on ensuring that the API functions can be called successfully with various configurations.
 *
 * @note Corresponding platform tests shall implement more specific tests with expected values and tighter tolerances.
 */

/* Test setup function */
static void *cordic_setup(void)
{
	static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);

	zassert_true(device_is_ready(cordic_dev),
		     "CORDIC device %s not ready", cordic_dev->name);
	return NULL;
}

/**
 * @brief Test CORDIC Configure function. This test is allowed to apply some
 * arbitrary configuration.
 * @note: If a target platform does not support certain configurations,
 * it may better to create a separate test for the platform itself.
 */
ZTEST(cordic_api, test_cordic_configure)
{
	static const struct device *const cordic_dev = DEVICE_DT_GET(CORDIC_NODE);
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

	/* Iterate through all possible function configurations */
	for(int i = 0; i < ARRAY_SIZE(fn_list); i++) {
		config.function = fn_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "%s: CORDIC function configure failed: %d",
			      cordic_dev->name, ret);
	}

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

	/* Iterate through all possible precision configurations */
	for(int i = 0; i < ARRAY_SIZE(prec_list); i++) {
		config.precision = prec_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC precision configure failed: %d", ret);
	}

	const enum cordic_data_width data_width_list[] = {
		CORDIC_DATA_WIDTH_32BIT,
		CORDIC_DATA_WIDTH_16BIT,
	};

	/* Iterate through all possible input data width configurations for
	 * input size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.in_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC input data width configure failed: %d", ret);
	}

	/* Iterate through all possible input data width configurations for
	 * output size.
	 */
	for(int i = 0; i < ARRAY_SIZE(data_width_list); i++) {
		config.out_width = data_width_list[i];
		ret = cordic_configure(cordic_dev, &config);
		zassert_equal(ret, 0, "CORDIC output data width configure failed: %d", ret);
	}
}

/**
 * @brief Test CORDIC Phase function
 */
ZTEST(cordic_api, test_phase)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_PHASE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Modulus function
 */
ZTEST(cordic_api, test_modulus)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_MODULUS,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Square root function
 */
ZTEST(cordic_api, test_squareroot)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const int args[1] = {F32_TO_Q31(x[0])};
	int res[1];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_SQUAREROOT,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Natural Logarithm function
 */
ZTEST(cordic_api, test_naturallog)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const int args[1] = {F32_TO_Q31(x[0])};
	int res[1];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_NATURALLOG,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Cosine function
 */
ZTEST(cordic_api, test_cosine)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_COSINE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Sine function
 */
ZTEST(cordic_api, test_sine)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_SINE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Hyperbolic Cosine function
 */
ZTEST(cordic_api, test_hyperbolic_cosine)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_HCOSINE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Hyperbolic Sine function
 */
ZTEST(cordic_api, test_hyperbolic_sine)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const float y[] = {0.5f};
	const int args[2] = {F32_TO_Q31(x[0]), F32_TO_Q31(y[0])};
	int res[2];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_HSINE,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Arctangent function
 */
ZTEST(cordic_api, test_arctangent)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const int args[1] = {F32_TO_Q31(x[0])};
	int res[1];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_ARCTANGENT,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

/**
 * @brief Test CORDIC Hyperbolic Arctangent function
 */
ZTEST(cordic_api, test_hyperbolic_arctangent)
{
	/* Input arguments */
	const float x[] = {1.0f};
	const int args[1] = {F32_TO_Q31(x[0])};
	int res[1];
	const size_t args_len = ARRAY_SIZE(args);
	const size_t res_len = ARRAY_SIZE(res);

	struct cordic_config config = {
		.function = CORDIC_FUNC_HARCTANGENT,
		.precision = CORDIC_PRECISION_10,
		.in_width = CORDIC_DATA_WIDTH_32BIT,
		.out_width = CORDIC_DATA_WIDTH_32BIT,
	};
	const struct device *const dev = DEVICE_DT_GET(CORDIC_NODE);

	/* Q31 format */
	int ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);

	/* Q15 format */
	config.in_width = CORDIC_DATA_WIDTH_16BIT;
	config.out_width = CORDIC_DATA_WIDTH_16BIT;
	ret = cordic_configure(dev, &config);
	zassert_equal(ret, 0, "CORDIC configure failed: %d", ret);

	ret = cordic_compute(dev, args, res, args_len, res_len);
	zassert_equal(ret, 0, "cordic_compute failed: %d", ret);
}

ZTEST_SUITE(cordic_api, NULL, cordic_setup, NULL, NULL, NULL);
