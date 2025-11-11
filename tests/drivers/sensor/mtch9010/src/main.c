/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/dt-bindings/sensor/mtch9010.h>

#include "../drivers/sensor/microchip/mtch9010/mtch9010_priv.h"
#include "zephyr/drivers/sensor/mtch9010.h"

#define DUT_NODE DT_NODELABEL(dut)

ZTEST_SUITE(mtch9010_utility, NULL, NULL, NULL, NULL, NULL);

ZTEST(mtch9010_utility, test_result_decode)
{
	/* Basic Decode Tests */
	const char *test_pattern_1 = "12345\n\r";
	const char *test_pattern_2 = "10\n\r";
	const char *test_pattern_3 = "999 12405\n\r";
	const char *test_pattern_4 = "0 1234\n\r";
	const char *test_pattern_5 = "100 -99\n\r";

	/* Bad Decodes */
	const char *bad_decode_pattern_1 = "10\n\r";
	const char *bad_decode_pattern_2 = "655636\n\r";
	const char *bad_decode_pattern_3 = "-100\n\r";
	const char *bad_decode_pattern_4 = "100";
	const char *bad_decode_pattern_5 = "100\t\n";
	const char *bad_decode_pattern_6 = "a100\n\r";

	struct mtch9010_result test_result;

	/* Test Current decode */
	int ret = mtch9010_decode_char_buffer(test_pattern_1, MTCH9010_OUTPUT_FORMAT_CURRENT,
					      &test_result);

	zassert_equal(ret, 0, "Unable to decode test_pattern_1");
	zassert_equal(test_result.measurement, 12345, "Decoded value does not match expected");

	/* Test DELTA decode */
	ret = mtch9010_decode_char_buffer(test_pattern_2, MTCH9010_OUTPUT_FORMAT_DELTA,
					  &test_result);

	zassert_equal(ret, 0, "Unable to decode test_pattern_2");
	zassert_equal(test_result.delta, 10, "Decoded value does not match expected");

	/* Test Current and Delta decode */
	ret = mtch9010_decode_char_buffer(test_pattern_3, MTCH9010_OUTPUT_FORMAT_BOTH,
					  &test_result);

	zassert_equal(ret, 0, "Unable to decode test_pattern_3");
	zassert_equal(test_result.prev_measurement, 12345,
		      "Previous value does not match expected");
	zassert_equal(test_result.measurement, 999, "Decoded value does not match expected");
	zassert_equal(test_result.delta, 12405, "Decoded value does not match expected");

	/* Test MPLAB Data Visualizer Format (should fail) */
	ret = mtch9010_decode_char_buffer(
		test_pattern_4, MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER, &test_result);
	zassert_equal(ret, -ENOTSUP, "Incorrectly decoded test_pattern_4");

	/* Test Negative Delta */
	ret = mtch9010_decode_char_buffer(test_pattern_5, MTCH9010_OUTPUT_FORMAT_BOTH,
					  &test_result);
	zassert_equal(ret, 0, "Unable to decode test_pattern_5");
	zassert_equal(test_result.measurement, 100, "Decoded value does not match expected");
	zassert_equal(test_result.delta, -99, "Decoded value does not match expected");

	/* Test Bad Decode 1 - Incorrect format */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_1, MTCH9010_OUTPUT_FORMAT_BOTH,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_1");

	/* Test Bad Decode 2 - UINT16 Buffer Overflow */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_2, MTCH9010_OUTPUT_FORMAT_CURRENT,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_2");

	/* Test Bad Decode 3 - Negative Values */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_3, MTCH9010_OUTPUT_FORMAT_CURRENT,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_3");

	/* Test Bad Decode 4 - Missing Return */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_4, MTCH9010_OUTPUT_FORMAT_CURRENT,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_4");

	/* Test Bad Decode 5 - Invalid Return */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_5, MTCH9010_OUTPUT_FORMAT_CURRENT,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_5");

	/* Test Bad Decode 6 - Invalid Starting Character */
	ret = mtch9010_decode_char_buffer(bad_decode_pattern_6, MTCH9010_OUTPUT_FORMAT_CURRENT,
					  &test_result);
	zassert_equal(ret, -EINVAL, "Incorrectly decoded bad_decode_pattern_6");
}

struct mtch9010_config_fixture {
	const struct device *dev;
};

static void *mtch9010_setup(void)
{
	static struct mtch9010_config_fixture fixture = {
		.dev = DEVICE_DT_GET(DUT_NODE),
	};

	/* Verify we found a device */
	zassert_not_null(fixture.dev);

	/* Create the reference configuration */
	return &fixture;
}

ZTEST_SUITE(mtch9010_config, NULL, mtch9010_setup, NULL, NULL, NULL);

/* Check UART */
ZTEST_F(mtch9010_config, test_uart_init)
{
	const struct mtch9010_config *config = fixture->dev->config;

	/* Verify the boolean flag */
	if (config->uart_init) {
		zassert_true(DT_PROP_OR(DUT_NODE, mtch9010_uart_config_enable, false),
			     "UART Init was enabled, but was not set");
	} else {
		zassert_false(DT_PROP_OR(DUT_NODE, mtch9010_uart_config_enable, false),
			      "UART Init was disabled, but was set");
	}

	/* Verify the UART Bus Pointer */
	const struct device *bus = DEVICE_DT_GET_OR_NULL(DT_BUS(DUT_NODE));

	zassert_equal_ptr(bus, config->uart_dev, "UART Bus is not correctly assigned");
}

/* Check GPIO Assignments */
ZTEST_F(mtch9010_config, test_gpio_bindings)
{
	const struct mtch9010_config *config = fixture->dev->config;

	/* GPIOs to Test */
	const struct gpio_dt_spec mode_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_mode_gpios, {0});
	const struct gpio_dt_spec output_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_output_gpios, {0});
	const struct gpio_dt_spec lock_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_system_lock_gpios, {0});
	const struct gpio_dt_spec reset_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_reset_gpios, {0});
	const struct gpio_dt_spec wake_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_wake_gpios, {0});
	const struct gpio_dt_spec uart_en_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_uart_en_gpios, {0});
	const struct gpio_dt_spec cfg_en_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_cfg_en_gpios, {0});
	const struct gpio_dt_spec heartbeat_gpio =
		GPIO_DT_SPEC_GET_OR(DUT_NODE, mtch9010_heartbeat_gpios, {0});

	if (mode_gpio.port != NULL) {
		zassert_not_null(config->mode_gpio.port, "mode_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->mode_gpio.port,
				"mode_gpio is not NULL, but was not assigned");
	}

	if (output_gpio.port != NULL) {
		zassert_not_null(config->out_gpio.port, "output_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->out_gpio.port,
				"output_gpio is not NULL, but was not assigned");
	}

	if (lock_gpio.port != NULL) {
		zassert_not_null(config->lock_gpio.port, "lock_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->lock_gpio.port,
				"lock_gpio is not NULL, but was not assigned");
	}

	if (reset_gpio.port != NULL) {
		zassert_not_null(config->reset_gpio.port, "reset_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->reset_gpio.port,
				"reset_gpio is not NULL, but was not assigned");
	}

	if (wake_gpio.port != NULL) {
		zassert_not_null(config->wake_gpio.port, "wake_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->wake_gpio.port,
				"wake_gpio is not NULL, but was not assigned");
	}

	if (uart_en_gpio.port != NULL) {
		zassert_not_null(config->enable_uart_gpio.port,
				 "uart_en_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->enable_uart_gpio.port,
				"uart_en_gpio is not NULL, but was not assigned");
	}

	if (cfg_en_gpio.port != NULL) {
		zassert_not_null(config->enable_cfg_gpio.port,
				 "cfg_en_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->enable_cfg_gpio.port,
				"cfg_en_gpio is not NULL, but was not assigned");
	}

	if (heartbeat_gpio.port != NULL) {
		zassert_not_null(config->heartbeat_gpio.port,
				 "heartbeat_gpio is NULL, but was assigned");
	} else {
		zassert_is_null(config->heartbeat_gpio.port,
				"heartbeat_gpio is not NULL, but was not assigned");
	}
}

ZTEST_F(mtch9010_config, test_sleep_time)
{
	const struct mtch9010_config *config = fixture->dev->config;

	zassert((config->sleep_time == DT_PROP_OR(DUT_NODE, mtch9010_sleep_period, 0)),
		"sleepTime was not correctly assigned.");
}

ZTEST_F(mtch9010_config, test_output_format)
{
	const struct mtch9010_config *config = fixture->dev->config;

	if (config->extended_mode_enable) {
		zassert_true(DT_PROP_OR(DUT_NODE, extended_output_enable, false),
			     "Extended output was disabled, but was set");
	} else {
		zassert_false(DT_PROP_OR(DUT_NODE, extended_output_enable, false),
			      "Extended output was enabled, but not set");

		zassert_true((config->format == MTCH9010_OUTPUT_FORMAT_CURRENT),
			     "Current output format was not correctly implied");
	}
}

ZTEST_F(mtch9010_config, test_custom_value)
{
	const struct mtch9010_config *config = fixture->dev->config;
	const struct mtch9010_data *data = fixture->dev->data;
	int custom_value = DT_PROP_OR(DUT_NODE, reference_value, -1);

	switch (config->ref_mode) {
	case MTCH9010_REFERENCE_CURRENT_VALUE: {
		zassert_equal(custom_value, -1, "Incorrect reference initialization mode set");
		break;
	}
	case MTCH9010_REFERENCE_CUSTOM_VALUE: {
		zassert_not_equal(custom_value, -1, "Incorrect reference initialization mode set");
		zassert_equal(custom_value, data->reference,
			      "Reference value was not set to custom value");
		break;
	}
	case MTCH9010_REFERENCE_RERUN_VALUE: {
		zassert_unreachable("Illegal reference value mode set");
		break;
	}
	default: {
		zassert_unreachable("Unknown Reference Value set");
		break;
	}
	}
}

ZTEST_F(mtch9010_config, test_threshold_value)
{
	const struct mtch9010_data *data = fixture->dev->data;
	int custom_value = DT_PROP(DUT_NODE, detect_value);

	zassert_equal(data->threshold, custom_value, "Threshold value was not set to custom value");
}
