/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/audio/audio_caps.h>
#include <zephyr/drivers/i2s.h>

#include "i2s_api_test.h"

#define INVALID_TRIGGER_SETTING 7

ZTEST_USER(i2s_errors, test_i2s_improper_configuration)
{
	int err;
	struct i2s_config invalid_config = { .word_size = 16U,
					     .channels = 2U,
					     .format = I2S_FMT_DATA_FORMAT_I2S,
					     .frame_clk_freq = FRAME_CLK_FREQ,
					     .block_size = BLOCK_SIZE,
					     .timeout = TIMEOUT,
					     .options = I2S_OPT_FRAME_CLK_MASTER |
							I2S_OPT_BIT_CLK_MASTER,
					     .mem_slab = &tx_mem_slab };


	invalid_config.format =
		I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED | I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED;

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &invalid_config);
	zassert_not_equal(
		err, 0,
		"I2S configuration did not detect improper data format (I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED | I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED)");

	invalid_config.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_DATA_ORDER_LSB;

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &invalid_config);
	zassert_not_equal(
		err, 0,
		"I2S configuration did not detect improper stream format (I2S_FMT_DATA_ORDER_LSB)");

	invalid_config.format = I2S_FMT_DATA_FORMAT_I2S;
	invalid_config.channels = 3U;
	err = i2s_configure(dev_i2s, I2S_DIR_TX, &invalid_config);
	zassert_not_equal(err, 0,
			  "I2S configuration did not detect improper channels configuration (3)");
}

ZTEST_USER(i2s_errors, test_i2s_config_attempt_in_wrong_state)
{
	int err;
	int config_err;
	char tx_data[BLOCK_SIZE] = {0};
	struct i2s_config inactive_config = { .word_size = 16U,
					      .channels = 2U,
					      .format = I2S_FMT_DATA_FORMAT_I2S,
					      .frame_clk_freq = FRAME_CLK_FREQ,
					      .block_size = BLOCK_SIZE,
					      .timeout = TIMEOUT,
					      .options = I2S_OPT_FRAME_CLK_MASTER |
							 I2S_OPT_BIT_CLK_MASTER,
					      .mem_slab = &tx_mem_slab };

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &inactive_config);
	zassert_equal(err, 0, "I2S interface configuration failed, err=%d", err);

	err = i2s_buf_write(dev_i2s, tx_data, BLOCK_SIZE);
	zassert_equal(err, 0, "I2S buffer write unexpected error: %d", err);

	err = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(err, 0, "I2S_TRIGGER_START unexpected error: %d", err);

	config_err = i2s_configure(dev_i2s, I2S_DIR_TX, &inactive_config);

	err = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(err, 0, "I2S_TRIGGER_STOP unexpected error: %d", err);

	err = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
	zassert_equal(err, 0, "I2S_TRIGGER_DROP unexpected error: %d", err);

	zassert_not_equal(
		config_err, 0,
		"I2S configuration should not be possible in states other than I2S_STATE_READY");
}

ZTEST_USER(i2s_errors, test_i2s_incorrect_trigger)
{
	int err;
	char tx_data[BLOCK_SIZE] = {0};
	struct i2s_config test_config = { .word_size = 16U,
					  .channels = 2U,
					  .format = I2S_FMT_DATA_FORMAT_I2S,
					  .frame_clk_freq = FRAME_CLK_FREQ,
					  .block_size = BLOCK_SIZE,
					  .timeout = TIMEOUT,
					  .options =
						  I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
					  .mem_slab = &tx_mem_slab };

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &test_config);
	zassert_equal(err, 0, "CFG err=%d", err);

	err = i2s_buf_write(dev_i2s, tx_data, BLOCK_SIZE);
	zassert_equal(err, 0, "I2S buffer write unexpected error: %d", err);

	err = i2s_trigger(dev_i2s, I2S_DIR_TX, INVALID_TRIGGER_SETTING);
	zassert_equal(err, -EINVAL, "I2S invalid trigger setting not detected: err=%d", err);
}

ZTEST_USER(i2s_errors, test_i2s_unconfigured_access)
{
	int err;
	char tx_data[BLOCK_SIZE] = {0};
	struct i2s_config inactive_config = { .word_size = 16U,
					      .channels = 2U,
					      .format = I2S_FMT_DATA_FORMAT_I2S,
					      .frame_clk_freq = 0,
					      .block_size = BLOCK_SIZE,
					      .timeout = TIMEOUT,
					      .options = I2S_OPT_FRAME_CLK_MASTER |
							 I2S_OPT_BIT_CLK_MASTER,
					      .mem_slab = &tx_mem_slab };

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &inactive_config);
	zassert_equal(err, 0, "I2S interface NOT_READY state transition failed. err=%d", err);

	err = i2s_buf_write(dev_i2s, tx_data, BLOCK_SIZE);
	zassert_equal(
		err, -EIO,
		"I2S attempting unconfigured interface access did not raise I/O error, err=%d",
		err);
}

ZTEST_USER(i2s_errors, test_i2s_improper_block_size_write)
{
	int err;
	char tx_data[BLOCK_SIZE] = {0};
	struct i2s_config test_config = { .word_size = 16U,
					  .channels = 2U,
					  .format = I2S_FMT_DATA_FORMAT_I2S,
					  .frame_clk_freq = FRAME_CLK_FREQ,
					  .block_size = BLOCK_SIZE,
					  .timeout = TIMEOUT,
					  .options =
						  I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
					  .mem_slab = &tx_mem_slab };

	err = i2s_configure(dev_i2s, I2S_DIR_TX, &test_config);
	zassert_equal(err, 0, "Unexpected error when configuring I2S interface: %d", err);

	err = i2s_buf_write(dev_i2s, tx_data, sizeof(uint16_t) + BLOCK_SIZE);
	zassert_not_equal(
		err, 0,
		"I2S attempting write with incorrect block size did not raise error, err=%d", err);
}

/**
 * @brief Test the i2s_get_caps API function
 *
 * This test validates the I2S get_caps API functionality by testing both
 * successful operation and error handling scenarios.
 */
ZTEST_USER(i2s_errors, test_i2s_get_caps)
{
	int ret;
	struct audio_caps caps;

	/**
	 * Test Case 1: Normal operation - Valid parameters
	 * Expected: Function should return 0 (success) or -ENOSYS (not implemented)
	 */
	ret = i2s_get_caps(dev_i2s, &caps);

	/* Handle case where driver doesn't implement get_caps */
	if (ret == -ENOSYS) {
		TC_PRINT("I2S get_caps not implemented by driver\n");
		ztest_test_skip();
		return;
	}

	/* Verify successful execution */
	zassert_equal(ret, 0, "i2s_get_caps should return 0, got %d", ret);

	/**
	 * Test Case 2: Capability value validation
	 * Verify that returned capability values are within reasonable ranges
	 */
	zassert_true(caps.min_total_channels >= 1, "min_total_channels should be >= 1, got %u",
		     caps.min_total_channels);

	zassert_true(caps.max_total_channels >= caps.min_total_channels,
		     "max_total_channels (%u) should be >= min_total_channels (%u)",
		     caps.max_total_channels, caps.min_total_channels);

	zassert_not_equal(caps.supported_sample_rates, 0, "supported_sample_rates should not be 0");

	zassert_not_equal(caps.supported_bit_widths, 0, "supported_bit_widths should not be 0");

	zassert_true(caps.min_num_buffers >= 1, "min_num_buffers should be >= 1, got %u",
		     caps.min_num_buffers);

	zassert_true(caps.max_frame_interval >= caps.min_frame_interval,
		     "max_frame_interval (%u) should be >= min_frame_interval (%u)",
		     caps.max_frame_interval, caps.min_frame_interval);

	/**
	 * Test Case 3: Error handling - NULL caps pointer
	 * Expected: Function should return -EINVAL for invalid parameter
	 */
	ret = i2s_get_caps(dev_i2s, NULL);
	zassert_equal(ret, -EINVAL,
		      "i2s_get_caps should return -EINVAL for NULL caps pointer, got %d", ret);
}
