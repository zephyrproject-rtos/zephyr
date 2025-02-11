/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_tracing.h"
#include "bs_types.h"
#include "bstests.h"

extern enum bst_result_t bst_result;

/*
 * @brief Mark the test as in progress
 *
 * Call this at the start of the test entry point.
 *
 * @param ... format-string and arguments to print to console
 *
 */
#define TEST_START(msg, ...)                                                                       \
	do {                                                                                       \
		bst_result = In_progress;                                                          \
		bs_trace_info_time(2, "Test start: " msg "\n", ##__VA_ARGS__);                     \
	} while (0)

/*
 * @brief Fail the test and exit
 *
 * Printf-like function that also terminates the device with an error code.
 *
 * @param ... format-string and arguments to print to console
 *
 */
#define TEST_FAIL(msg, ...)                                                                        \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(msg "\n", ##__VA_ARGS__);                                 \
	} while (0)

/*
 * @brief Mark the currently-running test as "Passed"
 *
 * Mark the test as "passed". The execution continues after that point.
 *
 * @note Use this if you use backchannels (testlib/bsim/sync.h).
 *
 * After calling this, the executable will eventually return 0 when it exits.
 * Unless `TEST_FAIL` is called (e.g. in a callback).
 *
 * @param ... format-string and arguments to print to console
 *
 */
#define TEST_PASS(msg, ...)                                                                        \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(2, "Test end: " msg "\n", ##__VA_ARGS__);                       \
	} while (0)

/*
 * @brief Mark test case as passed and end execution
 *
 * Mark the role / test-case as "Passed" and return 0.
 *
 * @note DO NOT use this if you use backchannels (testlib/bsim/sync.h).
 *
 * @note This macro only ends execution for the current executable, not all
 * executables in a simulation.
 *
 * @param ... format-string and arguments to print to console
 *
 */
#define TEST_PASS_AND_EXIT(msg, ...)                                                               \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(2, "Test end: " msg "\n", ##__VA_ARGS__);                       \
		bs_trace_silent_exit(0);                                                           \
	} while (0)

/*
 * @brief Assert `expr` is true
 *
 * Assert that `expr` is true. If assertion is false, print a printf-like
 * message to the console and fail the test. I.e. return non-zero.
 *
 * @note This is different than `sys/__assert.h`.
 *
 * @param message String to print to console
 *
 */
#define TEST_ASSERT(expr, ...)                                                                     \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			TEST_FAIL(__VA_ARGS__);                                                    \
		}                                                                                  \
	} while (0)

/*
 * @brief Print a value. Lower-level than `printk` or `LOG_xx`.
 *
 * Print a message to console.
 *
 * This can be safely called at any time in the execution of the device.
 * Use it to print when the logging subsystem is not available, e.g. early
 * startup or shutdown.
 *
 * @param ... format-string and arguments to print to console
 *
 */
#define TEST_PRINT(msg, ...)                                                                       \
		bs_trace_print(BS_TRACE_INFO, __FILE__, __LINE__, 0, BS_TRACE_AUTOTIME, 0,         \
			       msg "\n", ##__VA_ARGS__)
