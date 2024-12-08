/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/adc.h>

static const struct device *dev_adc = DEVICE_DT_GET(DT_ALIAS(adc));
#define BUFFER_LEN 8
static uint16_t m_sample_buffer[BUFFER_LEN];

static const struct adc_channel_cfg valid_channel_cfg = {
	.gain = ADC_GAIN_1,
	.channel_id = 0,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.differential = false,
	#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
	.input_positive = 1,
	#endif
};

static const struct adc_sequence valid_seq = {
	.buffer = m_sample_buffer,
	.buffer_size = BUFFER_LEN * sizeof(m_sample_buffer),
	.options = NULL,
	.resolution = 10,
	.oversampling = 0,
	.channels = 1,
};

/**
 * @brief test adc_read() with invalid oversampling value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_read_invalid_oversampling)
{
	int ret;

	adc_channel_setup(dev_adc, &valid_channel_cfg);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set oversampling to invalid value */
	invalid_seq.oversampling = 99;

	ret = adc_read(dev_adc, &invalid_seq);

	zassert_true(
		ret == -EINVAL,
		"adc_read() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_read() with invalid resolution value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_read_invalid_resolution)
{
	int ret;

	adc_channel_setup(dev_adc, &valid_channel_cfg);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set resolution to invalid value */
	invalid_seq.resolution = 99;

	ret = adc_read(dev_adc, &invalid_seq);

	zassert_true(
		ret == -EINVAL,
		"adc_read() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_read() with invalid channels value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_read_invalid_channels)
{
	int ret;

	adc_channel_setup(dev_adc, &valid_channel_cfg);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set channels configuration to invalid value */
	invalid_seq.channels = 0;

	ret = adc_read(dev_adc, &invalid_seq);

	zassert_true(
		ret == -EINVAL,
		"adc_read() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_read() with not configured channel
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_read_not_configured_channel)
{
	int ret;

	adc_channel_setup(dev_adc, &valid_channel_cfg);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set channels configuration to use not configured channel */
	invalid_seq.channels = BIT(1);

	ret = adc_read(dev_adc, &invalid_seq);

	zassert_true(
		ret == -EINVAL,
		"adc_read() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_read() with invalid buffer length
 *
 * function should return -ENOMEM
 */

ZTEST(adc_error_cases, test_adc_read_invalid_buffer)
{
	int ret;

	adc_channel_setup(dev_adc, &valid_channel_cfg);

	struct adc_sequence invalid_seq = valid_seq;
	/* set buffer size to 0 bytes */
	invalid_seq.buffer_size = 0;

	ret = adc_read(dev_adc, &invalid_seq);

	zassert_true(
		ret == -ENOMEM,
		"adc_read() should return -ENOMEM,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_channel_setup() with invalid reference value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_setup_invalid_reference)
{
	int ret;

	struct adc_channel_cfg invalid_channel_cfg = valid_channel_cfg;
	/* set invalid reference */
	invalid_channel_cfg.reference = 99;

	ret = adc_channel_setup(dev_adc, &invalid_channel_cfg);

	zassert_true(
		ret == -EINVAL,
		"adc_channel_setup() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief test adc_read() with invalid gain value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_setup_invalid_gain)
{
	int ret;

	struct adc_channel_cfg invalid_channel_cfg = valid_channel_cfg;
	/* set invalid gain value */
	invalid_channel_cfg.gain = 99;
	ret = adc_channel_setup(dev_adc, &invalid_channel_cfg);
	zassert_true(
		ret == -EINVAL,
		"adc_channel_setup() should return -EINVAL,"
		" got unexpected value of %d",
		ret
	);
}

static void *suite_setup(void)
{
	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("===================================================================\n");

	return NULL;
}

ZTEST_SUITE(adc_error_cases, NULL, suite_setup, NULL, NULL, NULL);
