/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_log_ratelimited, CONFIG_LOG_DEFAULT_LEVEL);

static uint32_t backend_msg_count;

static void test_backend_process(const struct log_backend *const backend,
				 union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(msg);
	backend_msg_count++;
}

static void test_backend_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api test_backend_api = {
	.process = test_backend_process,
	.panic = test_backend_panic,
};

LOG_BACKEND_DEFINE(test_ratelimit_backend, test_backend_api, true);

/* Test data for hexdump */
static uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/**
 * @brief Test compilation of all rate-limited macros.
 * This test primarily checks for compilation errors and does not assert log
 * counts, as log levels might filter some messages depending on Kconfig.
 */
ZTEST(log_ratelimited, test_compilation)
{
	/* This test only checks if the macros compile without errors. */
	/* Call each macro once to ensure compilation. */
	LOG_ERR_RATELIMIT("Compilation test: Error message");
	LOG_WRN_RATELIMIT("Compilation test: Warning message");
	LOG_INF_RATELIMIT("Compilation test: Info message");
	LOG_DBG_RATELIMIT("Compilation test: Debug message");

	LOG_ERR_RATELIMIT_RATE(100, "Compilation test: Error message with rate");
	LOG_WRN_RATELIMIT_RATE(100, "Compilation test: Warning message with rate");
	LOG_INF_RATELIMIT_RATE(100, "Compilation test: Info message with rate");
	LOG_DBG_RATELIMIT_RATE(100, "Compilation test: Debug message with rate");

	LOG_HEXDUMP_ERR_RATELIMIT(test_data, sizeof(test_data), "Compilation test: Error hexdump");
	LOG_HEXDUMP_WRN_RATELIMIT(test_data, sizeof(test_data),
				  "Compilation test: Warning hexdump");
	LOG_HEXDUMP_INF_RATELIMIT(test_data, sizeof(test_data), "Compilation test: Info hexdump");
	LOG_HEXDUMP_DBG_RATELIMIT(test_data, sizeof(test_data), "Compilation test: Debug hexdump");

	LOG_HEXDUMP_ERR_RATELIMIT_RATE(100, test_data, sizeof(test_data),
				       "Compilation test: Error hexdump with rate");
	LOG_HEXDUMP_WRN_RATELIMIT_RATE(100, test_data, sizeof(test_data),
				       "Compilation test: Warning hexdump with rate");
	LOG_HEXDUMP_INF_RATELIMIT_RATE(100, test_data, sizeof(test_data),
				       "Compilation test: Info hexdump with rate");
	LOG_HEXDUMP_DBG_RATELIMIT_RATE(100, test_data, sizeof(test_data),
				       "Compilation test: Debug hexdump with rate");

	zassert_true(true, "All rate-limited macros compile successfully");
}

static void log_test_i2c_err(uint32_t idx)
{
	LOG_ERR_RATELIMIT("Bus error occurred on i2c %s::%02x!", "i2c0", idx);
}

ZTEST(log_ratelimited, test_err_ratelimit_burst)
{
	const uint32_t burst_count = 20U;

	k_sleep(K_MSEC(10));
	while (log_process()) {
	}
	backend_msg_count = 0U;

	for (uint32_t i = 0U; i < burst_count; i++) {
		log_test_i2c_err(i);
	}
	while (log_process()) {
	}

	if (IS_ENABLED(CONFIG_LOG_RATELIMIT)) {
		zassert_equal(backend_msg_count, 1U, "Expected 1 logged error in burst, got %u",
			      backend_msg_count);

		k_sleep(K_MSEC(LOG_RATELIMIT_INTERVAL_MS + 10));
		log_test_i2c_err(burst_count);
		while (log_process()) {
		}

		/* Expect "Skipped 19 messages" + the new error message. */
		zassert_equal(backend_msg_count, 3U,
			      "Expected skipped summary + new error (3 total), got %u",
			      backend_msg_count);
	} else if (IS_ENABLED(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)) {
		zassert_equal(backend_msg_count, 0U, "Expected all messages dropped, got %u",
			      backend_msg_count);
	} else {
		zassert_equal(backend_msg_count, burst_count,
			      "Expected %u unthrottled messages, got %u", burst_count,
			      backend_msg_count);
	}
}

/* Define the test suite and specify the setup function to be called before each
 * test case.
 */
ZTEST_SUITE(log_ratelimited, NULL, NULL, NULL, NULL, NULL);
