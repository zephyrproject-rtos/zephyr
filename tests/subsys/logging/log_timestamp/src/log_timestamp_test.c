/*
 * Copyright (c) 2023 Nobleo Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log timestamp
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_custom.h>

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define SNAME "src"
#define DNAME "domain"
#define TEST_STR "test"

static uint8_t mock_buffer[512];
static uint8_t log_output_buf[4];
static uint32_t mock_len;

static void reset_mock_buffer(void)
{
	mock_len = 0U;
	memset(mock_buffer, 0, sizeof(mock_buffer));
}

static int mock_output_func(uint8_t *buf, size_t size, void *ctx)
{
	memcpy(&mock_buffer[mock_len], buf, size);
	mock_len += size;

	return size;
}

LOG_OUTPUT_DEFINE(log_output, mock_output_func,
		  log_output_buf, sizeof(log_output_buf));

int custom_timestamp(const struct log_output *output,
				const log_timestamp_t timestamp,
				const log_timestamp_printer_t printer)
{
	uint8_t buffer[] = "custom-timestamp: ";

	return printer(output, "%s", buffer);
}

ZTEST(test_timestamp, test_custom_timestamp)
{
	if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP)) {
		log_custom_timestamp_set(custom_timestamp);
	}
	static const char *exp_str = IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP) ?
			"custom-timestamp: " DNAME "/" SNAME ": " TEST_STR "\r\n" :
			"[00000001] " DNAME "/" SNAME ": " TEST_STR "\r\n";

	char package[256];
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 1, DNAME, SNAME, NULL, LOG_LEVEL_INF,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	zassert_equal(strcmp(exp_str, mock_buffer), 0);
}

static void before(void *notused)
{
	reset_mock_buffer();
}

ZTEST_SUITE(test_timestamp, NULL, NULL, before, NULL, NULL);
