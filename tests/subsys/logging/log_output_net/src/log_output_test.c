/*
 * Copyright (c) 2018 Nordic Semiconductor
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define SNAME    "src"
#define DNAME    "domain"
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

LOG_OUTPUT_DEFINE(log_output, mock_output_func, log_output_buf, sizeof(log_output_buf));

BUILD_ASSERT(IS_ENABLED(CONFIG_LOG_BACKEND_NET), "syslog backend not enabled");

ZTEST(test_log_output_net, test_format)
{
	char package[256];

	static const char *exp_str =
		"<134>1 1970-01-01T00:00:01.000000Z zephyr - - - - " DNAME "/" SNAME ": " TEST_STR;
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP |
			 LOG_OUTPUT_FLAG_FORMAT_SYSLOG;
	int err;

	log_output_timestamp_freq_set(1000000);

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 1000000, DNAME, SNAME, NULL, LOG_LEVEL_INF, package, NULL,
			   0, flags);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer, "expected: %s, is: %s", exp_str, mock_buffer);
}

static void before(void *notused)
{
	reset_mock_buffer();
}

ZTEST_SUITE(test_log_output_net, NULL, NULL, before, NULL, NULL);
