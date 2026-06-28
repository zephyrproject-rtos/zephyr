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

/**
 * @brief Verify a log message renders with source name and no extra prefixes.
 *
 * @details
 * Drives the log_output formatter with zero output flags to confirm that the
 * baseline rendering of a message is "<source>: <text>" terminated by CRLF.
 * This exercises the printf-style formatting of a log message into text with
 * the default decorations only.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it through log_output with flags = 0.
 * - Compare the rendered buffer against the expected string.
 *
 * Expected result:
 * - Output equals "src: test\r\n".
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_no_flags)
{
	char package[256];
	static const char *exp_str = SNAME ": " TEST_STR "\r\n";
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 0, NULL, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, 0);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify a raw-string log message is rendered without any decoration.
 *
 * @details
 * Processes a message tagged as an internal raw string and confirms the
 * formatter emits only the payload text, with no source prefix, level marker,
 * or trailing CRLF. This validates the formatter's handling of unformatted
 * passthrough content.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it through log_output with LOG_LEVEL_INTERNAL_RAW_STRING.
 * - Compare the rendered buffer against the raw payload.
 *
 * Expected result:
 * - Output equals "test" with no added prefix or newline.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_raw)
{
	char package[256];
	static const char *exp_str = TEST_STR;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 0, NULL, SNAME, NULL, 0, LOG_LEVEL_INTERNAL_RAW_STRING,
			   package, NULL, 0, 0);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify a log message renders with domain and source name prefix.
 *
 * @details
 * Supplies both a domain name and a source name to the formatter and confirms
 * the rendered message is prefixed with "domain/src:". This validates that the
 * formatter composes the message prefix from domain and source identifiers.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it through log_output supplying both DNAME and SNAME.
 * - Compare the rendered buffer against the expected string.
 *
 * Expected result:
 * - Output equals "domain/src: test\r\n".
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_no_flags_dname)
{
	char package[256];
	static const char *exp_str = DNAME "/" SNAME ": " TEST_STR "\r\n";
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 0, DNAME, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, 0);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify the severity level marker is rendered when requested.
 *
 * @details
 * Enables LOG_OUTPUT_FLAG_LEVEL and confirms the formatter prepends the
 * textual severity marker (e.g. "<inf>") ahead of the domain/source prefix.
 * This validates rendering of the message severity into the formatted output.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it at LOG_LEVEL_INF with the level flag set.
 * - Compare the rendered buffer against the expected string.
 *
 * Expected result:
 * - Output equals "<inf> domain/src: test\r\n".
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-7
 */
ZTEST(test_log_output, test_level_flag)
{
	char package[256];
	static const char *exp_str = "<inf> " DNAME "/" SNAME ": " TEST_STR "\r\n";
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 0, DNAME, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify a raw timestamp is rendered when the timestamp flag is set.
 *
 * @details
 * Enables LOG_OUTPUT_FLAG_TIMESTAMP and confirms the formatter prepends the
 * numeric timestamp in brackets, sized according to the 32/64-bit timestamp
 * configuration. A timestamped, machine-readable prefix supports later
 * post-processing of the log stream.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it with the timestamp flag set.
 * - Compare the rendered buffer against the expected bracketed timestamp.
 *
 * Expected result:
 * - Output is prefixed with "[00000000] " (or 64-bit form) followed by the message.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-14
 */
ZTEST(test_log_output, test_ts_flag)
{
	char package[256];
	static const char *exp_str = IS_ENABLED(CONFIG_LOG_TIMESTAMP_64BIT) ?
		"[0000000000000000] " DNAME "/" SNAME ": " TEST_STR "\r\n" :
		"[00000000] " DNAME "/" SNAME ": " TEST_STR "\r\n";
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 0, DNAME, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify a timestamp is rendered as a formatted date/time string.
 *
 * @details
 * Sets the timestamp frequency and enables both the timestamp and
 * format-timestamp flags so the raw tick value is converted into a human and
 * machine readable date/time string (plain, date, or ISO8601 per Kconfig).
 * This validates timestamp formatting that supports post-processing of logs.
 *
 * Test steps:
 * - Set the output timestamp frequency to 1 MHz.
 * - Package a simple string and process it with timestamp + format flags.
 * - Compare the rendered buffer against the expected formatted timestamp.
 *
 * Expected result:
 * - Output is prefixed with the configured formatted timestamp string.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-14
 */
ZTEST(test_log_output, test_format_ts)
{
#ifdef CONFIG_LOG_OUTPUT_FORMAT_DATE_TIMESTAMP
#define TIMESTAMP_STR "[1970-01-01 00:00:01.000,000] "
#elif defined(CONFIG_LOG_OUTPUT_FORMAT_ISO8601_TIMESTAMP)
#define TIMESTAMP_STR "[1970-01-01T00:00:01,000000Z] "
#else
#define TIMESTAMP_STR "[00:00:01.000,000] "
#endif
	char package[256];
	static const char *exp_str = TIMESTAMP_STR DNAME "/" SNAME ": " TEST_STR "\r\n";
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	int err;

	log_output_timestamp_freq_set(1000000);

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	log_output_process(&log_output, 1000000, DNAME, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	printk("%s", mock_buffer);
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify timestamp ticks are converted to microseconds correctly.
 *
 * @details
 * Exercises the timestamp-to-microseconds conversion helper at two different
 * timestamp frequencies and confirms the scaled results. Correct conversion
 * underpins the timestamps emitted into formatted log output.
 *
 * Test steps:
 * - Set frequency to 1 MHz and convert 1000 ticks.
 * - Set frequency to 32768 Hz and convert 10 ticks.
 * - Assert the converted microsecond values.
 *
 * Expected result:
 * - Conversions yield 1000 us and 305 us respectively.
 *
 * @see log_output_timestamp_to_us()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_ts_to_us)
{
	log_output_timestamp_freq_set(1000000);

	zassert_equal(log_output_timestamp_to_us(1000), 1000);

	log_output_timestamp_freq_set(32768);

	zassert_equal(log_output_timestamp_to_us(10), 305);
}

static bool use_func_prefix(uint8_t level)
{
	switch (level) {
	case LOG_LEVEL_ERR:
		return IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_ERR);
	case LOG_LEVEL_WRN:
		return IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_WRN);
	case LOG_LEVEL_INF:
		return IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_INF);
	case LOG_LEVEL_DBG:
		return IS_ENABLED(CONFIG_LOG_FUNC_NAME_PREFIX_DBG);
	default:
		return false;
	}
}

/**
 * @brief Verify each severity level renders its corresponding text marker.
 *
 * @details
 * Iterates over the error, warning, info and debug levels and confirms the
 * formatter renders the matching marker ("<err>", "<wrn>", "<inf>", "<dbg>"),
 * optionally including the function-name prefix when configured. This
 * validates rendering of all supported severity levels into formatted output.
 *
 * Test steps:
 * - For each level, package a message and process it with the level flag.
 * - Account for the function-name prefix when enabled for that level.
 * - Assert the rendered length and string for each level.
 *
 * Expected result:
 * - Each message carries the correct severity marker and text.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-7
 */
ZTEST(test_log_output, test_levels)
{
	char package[256];
	static const char *const level_strs[] = {
		"<err>",
		"<wrn>",
		"<inf>",
		"<dbg>"
	};
	uint8_t levels[] = {LOG_LEVEL_ERR, LOG_LEVEL_WRN, LOG_LEVEL_INF, LOG_LEVEL_DBG};
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL;
	char exp_str[128];
	int err;

	ARRAY_FOR_EACH(levels, i) {
		int slen;

		reset_mock_buffer();

		if (use_func_prefix(levels[i])) {
			slen = sprintf(exp_str, "%s %s.%s: %s\r\n",
				level_strs[i], SNAME, __func__, TEST_STR);
			err = cbprintf_package(package, sizeof(package), 0,
						"%s: " TEST_STR, __func__);
		} else {
			slen = sprintf(exp_str, "%s %s: %s\r\n", level_strs[i], SNAME, TEST_STR);
			err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
		}
		zassert_true(err > 0);

		log_output_process(&log_output, 0, NULL, SNAME, NULL, 0, levels[i],
				   package, NULL, 0, flags);

		mock_buffer[mock_len] = '\0';
		zassert_equal(mock_len, slen);
		zassert_str_equal(exp_str, mock_buffer, "exp:%s, got:%s", exp_str, mock_buffer);
	}
}

/**
 * @brief Verify color escape codes wrap messages per severity when enabled.
 *
 * @details
 * Enables the level and colors flags and confirms the formatter surrounds each
 * message with the ANSI color escape sequence associated with its severity
 * (or the default sequence when colors are disabled in the build). This
 * validates color decoration of formatted, severity-tagged output.
 *
 * Test steps:
 * - For each level, build the expected colored string and package a message.
 * - Process it with the level + colors flags set.
 * - Assert the rendered length and string for each level.
 *
 * Expected result:
 * - Each message is wrapped with the expected color codes for its level.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 * @verifies ZEP-SRS-11-7
 */
ZTEST(test_log_output, test_colors)
{
#define LOG_COLOR_CODE_DEFAULT "\x1B[0m"
#define LOG_COLOR_CODE_RED     "\x1B[1;31m"
#define LOG_COLOR_CODE_GREEN   "\x1B[1;32m"
#define LOG_COLOR_CODE_YELLOW  "\x1B[1;33m"
#define LOG_COLOR_CODE_BLUE    "\x1B[1;34m"

#ifdef CONFIG_LOG_BACKEND_SHOW_COLOR
#define LOG_COLOR_ERR          LOG_COLOR_CODE_RED
#define LOG_COLOR_WRN          LOG_COLOR_CODE_YELLOW
#define LOG_COLOR_INF          LOG_COLOR_CODE_GREEN
#define LOG_COLOR_DBG          LOG_COLOR_CODE_BLUE
#else
#define LOG_COLOR_ERR          LOG_COLOR_CODE_DEFAULT
#define LOG_COLOR_WRN          LOG_COLOR_CODE_DEFAULT
#define LOG_COLOR_INF          LOG_COLOR_CODE_DEFAULT
#define LOG_COLOR_DBG          LOG_COLOR_CODE_DEFAULT
#endif /* CONFIG_LOG_BACKEND_SHOW_COLOR */

	char package[256];
	static const char *const color_strs[] = {
		LOG_COLOR_ERR,
		LOG_COLOR_WRN,
		LOG_COLOR_INF,
		LOG_COLOR_DBG
	};
	static const char *const level_strs[] = {
		"<err>",
		"<wrn>",
		"<inf>",
		"<dbg>"
	};
	uint8_t levels[] = {LOG_LEVEL_ERR, LOG_LEVEL_WRN, LOG_LEVEL_INF, LOG_LEVEL_DBG};
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_COLORS;
	char exp_str[128];
	int err;

	ARRAY_FOR_EACH(levels, i) {
		int slen;

		if (use_func_prefix(levels[i])) {
			slen = sprintf(exp_str, "%s%s %s.%s: %s%s\r\n",
				color_strs[i], level_strs[i], SNAME, __func__, TEST_STR,
				LOG_COLOR_CODE_DEFAULT);
			err = cbprintf_package(package, sizeof(package), 0,
						"%s: " TEST_STR, __func__);
			zassert_true(err >= 0);
		} else {
			slen = sprintf(exp_str, "%s%s %s: %s%s\r\n",
				color_strs[i], level_strs[i], SNAME, TEST_STR,
				LOG_COLOR_CODE_DEFAULT);
			err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
			zassert_true(err >= 0);
		}
		reset_mock_buffer();

		log_output_process(&log_output, 0, NULL, SNAME, NULL, 0, levels[i],
				   package, NULL, 0, flags);

		mock_buffer[mock_len] = '\0';
		zassert_equal(mock_len, slen);
		zassert_str_equal(exp_str, mock_buffer, "exp:%s got:%s", exp_str, mock_buffer);
	}
}

/**
 * @brief Verify the originating thread identity is rendered when requested.
 *
 * @details
 * Enables the thread flag and confirms the formatter prepends the originating
 * thread's priority and name (or pointer when names are disabled) to the
 * message. This validates rendering of thread context into formatted output.
 *
 * Test steps:
 * - Skip unless CONFIG_LOG_THREAD_ID_PREFIX is enabled.
 * - Build the expected string from the current thread's priority and name.
 * - Package a message and process it with the level + thread flags.
 *
 * Expected result:
 * - Output includes the "[prio name]" thread prefix ahead of the message.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_thread_id)
{
	if (!IS_ENABLED(CONFIG_LOG_THREAD_ID_PREFIX)) {
		return;
	}

	char exp_str[256];
	char package[256];

	k_tid_t tid = k_current_get();
	const char *name = k_thread_name_get(tid);
	int prio = k_thread_priority_get(tid);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		sprintf(exp_str, "<err> [%3d %s] src: Test\r\n", prio, name);
	} else {
		sprintf(exp_str, "<err> [%3d %p] src: Test\r\n", prio, tid);
	}

	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_THREAD;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, "Test");
	zassert_true(err > 0);

	log_output_process(&log_output, 0, NULL, SNAME, k_current_get(), 0, 1,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	printk("%s", mock_buffer);
	zassert_str_equal(exp_str, mock_buffer);
}

/**
 * @brief Verify the source prefix is omitted when the skip-source flag is set.
 *
 * @details
 * Enables LOG_OUTPUT_FLAG_SKIP_SOURCE and confirms the formatter renders only
 * the message payload and CRLF, without the source-name prefix. This validates
 * the formatter's ability to suppress source decoration in the output.
 *
 * Test steps:
 * - Package a simple string with cbprintf_package().
 * - Process it with the skip-source flag set.
 * - Compare the rendered buffer against the expected string.
 *
 * Expected result:
 * - Output equals "test\r\n" with no source prefix.
 *
 * @see log_output_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_output, test_skip_src)
{
	char package[256];
	const char exp_str[] = TEST_STR "\r\n";
	uint32_t flags = LOG_OUTPUT_FLAG_SKIP_SOURCE;
	int err;

	err = cbprintf_package(package, sizeof(package), 0, TEST_STR);
	zassert_true(err > 0);

	reset_mock_buffer();
	log_output_process(&log_output, 0, NULL, SNAME, NULL, 0, LOG_LEVEL_INF,
			   package, NULL, 0, flags);

	mock_buffer[mock_len] = '\0';
	zassert_str_equal(exp_str, mock_buffer);
}

static void before(void *notused)
{
	reset_mock_buffer();
}

ZTEST_SUITE(test_log_output, NULL, NULL, before, NULL, NULL);
