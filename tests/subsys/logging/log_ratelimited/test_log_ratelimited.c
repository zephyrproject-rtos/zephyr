/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_log_ratelimited, CONFIG_LOG_DEFAULT_LEVEL);

/* Test data for hexdump. Unused when the fallback drops all messages. */
static uint8_t test_data[] __maybe_unused = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* Rate limit state shared by the explicit-state variant calls */
static struct log_ratelimit_state test_state;

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

	LOG_ERR_RATELIMIT_STATE(&test_state, 100, "Compilation test: Error message with state");
	LOG_WRN_RATELIMIT_STATE(&test_state, 100, "Compilation test: Warning message with state");
	LOG_INF_RATELIMIT_STATE(&test_state, 100, "Compilation test: Info message with state");
	LOG_DBG_RATELIMIT_STATE(&test_state, 100, "Compilation test: Debug message with state");

	LOG_HEXDUMP_ERR_RATELIMIT_STATE(&test_state, 100, test_data, sizeof(test_data),
					"Compilation test: Error hexdump with state");
	LOG_HEXDUMP_WRN_RATELIMIT_STATE(&test_state, 100, test_data, sizeof(test_data),
					"Compilation test: Warning hexdump with state");
	LOG_HEXDUMP_INF_RATELIMIT_STATE(&test_state, 100, test_data, sizeof(test_data),
					"Compilation test: Info hexdump with state");
	LOG_HEXDUMP_DBG_RATELIMIT_STATE(&test_state, 100, test_data, sizeof(test_data),
					"Compilation test: Debug hexdump with state");

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

/* The rate limit state machine only exists when rate limiting is enabled. */
#ifdef CONFIG_LOG_RATELIMIT
ZTEST(log_ratelimited, test_explicit_state_first_ms)
{
	struct log_ratelimit_state state = {0};

	/* A message emitted during the very first millisecond of uptime biases
	 * the stored timestamp away from the reserved 0.
	 */
	zassert_true(z_log_ratelimit_update(&state, 60 * 1000, 0, 0) >= 0,
		     "message at t=0 must be emitted");
	zassert_equal(atomic_get(&state.last_log_time), 1, "t=0 must be biased to 1");

	/* Without the bias the timestamp stayed 0 and the second message was
	 * emitted as well, because 0 means "never emitted".
	 */
	zassert_equal(z_log_ratelimit_update(&state, 60 * 1000, 1, 0), -1,
		      "second message within the first millisecond must be suppressed");
	zassert_equal(atomic_get(&state.skipped_count), 1,
		      "the suppressed message must be counted");

	/* Once the interval has elapsed, the message is emitted and reports the
	 * number of suppressed ones.
	 */
	zassert_equal(z_log_ratelimit_update(&state, 100, 1, 1000), 1,
		      "message after the interval must report one skipped message");
	zassert_equal(atomic_get(&state.skipped_count), 0, "skipped count must be cleared");
}

ZTEST(log_ratelimited, test_explicit_state_suppression)
{
	struct log_ratelimit_state state = {0};

	/* The first message is always emitted. */
	zassert_true(z_log_ratelimit_take(&state, 60 * 1000) >= 0,
		     "first message must be emitted");

	/* A back-to-back second message is suppressed and counted. */
	zassert_equal(z_log_ratelimit_take(&state, 60 * 1000), -1,
		      "message within the interval must be suppressed");
	zassert_equal(atomic_get(&state.skipped_count), 1,
		      "one suppressed message must be counted");

	/* The stored timestamp never takes the reserved "never emitted" value. */
	zassert_not_equal(atomic_get(&state.last_log_time), 0,
			  "timestamp must not use the reserved 0");
}

ZTEST(log_ratelimited, test_explicit_state_lost_race)
{
	struct log_ratelimit_state state = {0};

	/* Another context emitted at t=1000 between the two timestamp reads of
	 * the caller: the stale expectation makes the CAS fail, so the message
	 * is suppressed instead of moving the timestamp backwards.
	 */
	zassert_true(z_log_ratelimit_update(&state, 100, 0, 1000) >= 0,
		     "first message must be emitted");
	zassert_equal(z_log_ratelimit_update(&state, 100, 900, 5000), -1,
		      "lost race must be suppressed");
	zassert_equal(atomic_get(&state.skipped_count), 1, "lost race must be counted");
}
#endif

/* Define the test suite and specify the setup function to be called before each
 * test case.
 */
ZTEST_SUITE(log_ratelimited, NULL, NULL, NULL, NULL, NULL);
