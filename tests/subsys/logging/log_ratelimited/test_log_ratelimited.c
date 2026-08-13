/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_log_ratelimited, CONFIG_LOG_DEFAULT_LEVEL);

/* Test data for hexdump */
static uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/**
 * @brief Verify all rate-limited logging macros compile and run.
 *
 * @details
 * Exercise the full set of rate-limited logging macros (LOG_*_RATELIMIT,
 * LOG_*_RATELIMIT_RATE and their hexdump variants) at every severity level. The
 * test primarily ensures the macros expand and build correctly and can be
 * invoked at runtime; it does not assert log counts because severity filtering
 * may suppress some messages depending on Kconfig.
 *
 * Test steps:
 * - Invoke each rate-limited message macro once.
 * - Invoke each rate-limited message macro with an explicit rate.
 * - Invoke each rate-limited hexdump macro, with and without an explicit rate.
 *
 * Expected result:
 * - All macros compile and execute without error.
 *
 * @see LOG_INF_RATELIMIT()
 * @ingroup logging_tests
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

/* Define the test suite and specify the setup function to be called before each
 * test case.
 */
ZTEST_SUITE(log_ratelimited, NULL, NULL, NULL, NULL, NULL);
