/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <logging/log_output.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static u8_t mock_buffer[512];
static u8_t log_output_buf[8];
static u32_t mock_len;

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

static int mock_output_func(u8_t *buf, size_t size, void *ctx)
{
	memcpy(&mock_buffer[mock_len], buf, size);
	mock_len += size;

	return size;
}

LOG_OUTPUT_DEFINE(log_output, mock_output_func,
		  log_output_buf, sizeof(log_output_buf));

static void validate_output_string(const char *exp)
{
	int len = strlen(exp);

	zassert_equal(len, mock_len, "Unexpected string length");
	zassert_equal(0, memcmp(exp, mock_buffer, mock_len),
			"Unxpected string");
}

static void log_output_string_varg(const struct log_output *log_output,
		       struct log_msg_ids src_level, u32_t timestamp,
		       u32_t flags, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	log_output_string(log_output, src_level, timestamp, fmt, ap, flags);

	va_end(ap);
}

void test_log_output_raw_string(void)
{
	const char *exp_str = "abc 1 3";
	const char *exp_str2 = "abc efg 3\n\r";
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INTERNAL_RAW_STRING,
		.source_id = 0, /* not used as level indicates raw string. */
		.domain_id = 0, /* not used as level indicates raw string. */
	};

	log_output_string_varg(&log_output, src_level, 0, 0,
			       "abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str);

	reset_mock_buffer();

	/* Test adding \r after new line feed */
	log_output_string_varg(&log_output, src_level, 0, 0,
			       "abc %s %d\n", "efg", 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str2);
}

void test_log_output_string(void)
{
	const char *exp_str = STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_lvl =
			"<dbg> " STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_timestamp =
			"[00123456] " STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_no_crlf = STRINGIFY(LOG_MODULE_NAME) ".abc 1 3";
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_DBG,
		.source_id = log_const_source_id(
				&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME)),
		.domain_id = CONFIG_LOG_DOMAIN_ID,
	};

	log_output_string_varg(&log_output, src_level, 0,
				0 /* no flags */,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_LEVEL adds log level prefix. */
	log_output_string_varg(&log_output, src_level, 0,
				LOG_OUTPUT_FLAG_LEVEL,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_lvl);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_TIMESTAMP adds timestamp. */
	log_output_string_varg(&log_output, src_level, 123456,
				LOG_OUTPUT_FLAG_TIMESTAMP,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_timestamp);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_CRLF_NONE adds no crlf */
	log_output_string_varg(&log_output, src_level, 0,
				LOG_OUTPUT_FLAG_CRLF_NONE,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_no_crlf);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_message,
		ztest_unit_test_setup_teardown(test_log_output_raw_string,
					       setup, teardown),
		ztest_unit_test_setup_teardown(test_log_output_string,
					       setup, teardown)
		);
	ztest_run_test_suite(test_log_message);
}
