/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test output capture helper.
 *
 * Captures the textual diagnostic output produced during a test into an
 * in-memory buffer so it can be asserted on, without each test having to wire
 * up its own log backend or printk hook. Both output routes are handled
 * transparently:
 *
 * - printk() output (and logging in @kconfig{CONFIG_LOG_MODE_MINIMAL}, which is
 *   routed through printk) is captured by wrapping the printk character-output
 *   hook.
 * - Full logging output (@kconfig{CONFIG_LOG} with a real backend mode) is
 *   captured through a dedicated log backend.
 *
 * Typical use:
 *
 * @code{.c}
 * tc_capture_start();
 * do_something_that_prints();
 * tc_capture_stop();
 * zassert_true(tc_capture_contains("expected text"));
 * @endcode
 *
 * @note When capturing @b logging output, the message must reach the backend
 * before tc_capture_stop() is called. Use @kconfig{CONFIG_LOG_MODE_IMMEDIATE}
 * (or otherwise flush the logs) so deferred messages are not missed.
 */

#ifndef ZEPHYR_TESTSUITE_TC_CAPTURE_H_
#define ZEPHYR_TESTSUITE_TC_CAPTURE_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start capturing test output.
 *
 * Clears any previously captured data and begins recording printk and logging
 * output into the internal capture buffer. Capturing continues until
 * tc_capture_stop() is called. Output is still forwarded to the normal console
 * so ordinary test logs are unaffected.
 */
void tc_capture_start(void);

/**
 * @brief Stop capturing test output.
 *
 * Restores the normal output path. The captured data remains available for
 * inspection with tc_capture_contains() and tc_capture_get() until the next
 * tc_capture_start() or tc_capture_clear().
 */
void tc_capture_stop(void);

/**
 * @brief Check whether the captured output contains a substring.
 *
 * @param substr NUL-terminated substring to search for.
 *
 * @retval true  if @p substr is present in the captured output.
 * @retval false otherwise.
 */
bool tc_capture_contains(const char *substr);

/**
 * @brief Copy the captured output into a caller-provided buffer.
 *
 * The copy is always NUL-terminated (unless @p size is 0) and is truncated to
 * fit @p size.
 *
 * @param dst  Destination buffer.
 * @param size Size of @p dst in bytes.
 *
 * @return Number of bytes copied, excluding the NUL terminator.
 */
size_t tc_capture_get(char *dst, size_t size);

/**
 * @brief Discard any captured output without stopping capture.
 */
void tc_capture_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_TC_CAPTURE_H_ */
