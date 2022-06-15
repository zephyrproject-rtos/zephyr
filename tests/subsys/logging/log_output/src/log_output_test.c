/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr/zephyr.h>
#include <ztest.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static uint8_t mock_buffer[512];
static uint8_t log_output_buf[8];
static uint32_t mock_len;

static void reset_mock_buffer(void)
{
	mock_len = 0U;
	memset(mock_buffer, 0, sizeof(mock_buffer));
}

static void setup(void)
{
	reset_mock_buffer();
}

static void teardown(void)
{

}

static int mock_output_func(uint8_t *buf, size_t size, void *ctx)
{
	memcpy(&mock_buffer[mock_len], buf, size);
	mock_len += size;

	return size;
}

LOG_OUTPUT_DEFINE(log_output, mock_output_func,
		  log_output_buf, sizeof(log_output_buf));

static void test_log_output_empty(void)
{
	(void)&log_output;
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_output,
		ztest_unit_test_setup_teardown(test_log_output_empty,
					       setup, teardown)
		);
	ztest_run_test_suite(test_log_output);
}
