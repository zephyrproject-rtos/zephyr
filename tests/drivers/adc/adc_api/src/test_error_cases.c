/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/adc.h>

#include "adc_test.h"

#define BUFFER_LEN 8
static uint16_t m_sample_buffer[BUFFER_LEN];

/* A channel the board describes, and a channel id it does not. */
#define VALID_CHANNEL (&adc_test_channels[0])
static uint8_t unused_channel_id;

static struct adc_sequence valid_seq = {
	.buffer = m_sample_buffer,
	.buffer_size = BUFFER_LEN * sizeof(m_sample_buffer),
	.options = NULL,
	.oversampling = 0,
};

/**
 * @brief test adc_read() with invalid oversampling value
 *
 * function should return -EINVAL
 */

ZTEST(adc_error_cases, test_adc_read_invalid_oversampling)
{
	int ret;

	adc_channel_setup_dt(VALID_CHANNEL);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set oversampling to invalid value */
	invalid_seq.oversampling = 99;

	ret = adc_read_dt(VALID_CHANNEL, &invalid_seq);

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

	adc_channel_setup_dt(VALID_CHANNEL);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set resolution to invalid value */
	invalid_seq.resolution = 99;

	ret = adc_read_dt(VALID_CHANNEL, &invalid_seq);

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

	adc_channel_setup_dt(VALID_CHANNEL);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set channels configuration to invalid value */
	invalid_seq.channels = 0;

	ret = adc_read_dt(VALID_CHANNEL, &invalid_seq);

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

	adc_channel_setup_dt(VALID_CHANNEL);

	struct adc_sequence invalid_seq = valid_seq;
	/* Set channels configuration to use not configured channel */
	invalid_seq.channels = BIT(unused_channel_id);

	ret = adc_read_dt(VALID_CHANNEL, &invalid_seq);

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

	adc_channel_setup_dt(VALID_CHANNEL);

	struct adc_sequence invalid_seq = valid_seq;
	/* set buffer size to 0 bytes */
	invalid_seq.buffer_size = 0;

	ret = adc_read_dt(VALID_CHANNEL, &invalid_seq);

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

	struct adc_channel_cfg invalid_channel_cfg = VALID_CHANNEL->channel_cfg;
	/* set invalid reference */
	invalid_channel_cfg.reference = 99;

	ret = adc_channel_setup(VALID_CHANNEL->dev, &invalid_channel_cfg);

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

	struct adc_channel_cfg invalid_channel_cfg = VALID_CHANNEL->channel_cfg;
	/* set invalid gain value */
	invalid_channel_cfg.gain = 99;
	ret = adc_channel_setup(VALID_CHANNEL->dev, &invalid_channel_cfg);
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

	zassert_true(adc_is_ready_dt(VALID_CHANNEL), "ADC device is not ready");

	/* The sequence the board's first channel is valid for. */
	(void)adc_sequence_init_dt(VALID_CHANNEL, &valid_seq);

	/* A channel id below the width of the channel mask that no channel uses. */
	for (unused_channel_id = 0; unused_channel_id < 32; unused_channel_id++) {
		bool used = false;

		for (size_t i = 0; i < ADC_TEST_CHANNEL_COUNT; i++) {
			if (adc_test_channels[i].dev == VALID_CHANNEL->dev &&
			    adc_test_channels[i].channel_id == unused_channel_id) {
				used = true;
			}
		}
		if (!used) {
			break;
		}
	}
	zassert_true(unused_channel_id < 32, "Every channel id is in use");

	return NULL;
}

ZTEST_SUITE(adc_error_cases, NULL, suite_setup, NULL, NULL, NULL);
