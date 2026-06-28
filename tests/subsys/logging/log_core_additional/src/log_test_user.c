/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Additional test case for log core
 *
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(user);

/* Interfaces tested in these cases have been tested in kernel space.
 * Test cases in this file run in user space to improve test coverage
 */
#ifdef CONFIG_USERSPACE
/**
 * @brief Verify messages can be logged and deferred-processed from user space.
 *
 * @details
 * A user-mode thread emits log messages that are buffered by the core and
 * processed later. The test records the buffered message count, drains the
 * queue and confirms the buffered count does not increase, demonstrating that
 * user-space messages are queued and subsequently processed.
 *
 * Test steps:
 * - From user space, emit a plain and a formatted info message.
 * - Capture the buffered count, run log_process() to drain the queue.
 * - Confirm the post-processing buffered count is not greater than before.
 *
 * Expected result:
 * - User-space messages are buffered and then processed off the queue.
 *
 * @see log_process()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-6
 */
ZTEST_USER(test_log_core_additional, test_log_from_user)
{
	uint32_t cnt0, cnt1;
	int d = 0;

	LOG_INF("log from user");
	LOG_INF("log from user %d", d);
	cnt0 = log_buffered_cnt();
	while (log_process()) {
	}
	cnt1 = log_buffered_cnt();
	zassert_true(cnt1 <= cnt0, "no message is handled");
}

/**
 * @brief Verify binary data can be logged as a hex dump from user space.
 *
 * @details
 * The logging system can render a buffer of binary data as a hexadecimal dump.
 * The test issues a LOG_HEXDUMP_INF() of a small data buffer from a user-mode
 * thread and processes the queue, confirming the hexdump path is exercised
 * across the user/kernel boundary.
 *
 * Test steps:
 * - From user space, hexdump a 4-byte value via LOG_HEXDUMP_INF().
 * - Process the log queue.
 *
 * Expected result:
 * - The binary data is logged as a hex dump without fault.
 *
 * @see LOG_HEXDUMP_INF()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-12
 */
ZTEST_USER(test_log_core_additional, test_log_hexdump_from_user)
{
	int32_t data = 128;

	LOG_HEXDUMP_INF(&data, sizeof(data), "test_hexdump");
	while (log_process()) {
	}
}

static void call_log_generic(uint32_t source_id, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_generic(LOG_LEVEL_INF, fmt, ap);
	va_end(ap);
}

/**
 * @brief Verify log_generic() renders formatted messages from user space.
 *
 * @details
 * log_generic() accepts a format string and a va_list and performs printf-style
 * rendering of the message. The test calls it from a user-mode thread and
 * processes the queue, confirming formatted logging works across the user/kernel
 * boundary.
 *
 * Test steps:
 * - From user space, call log_generic() with a format string.
 * - Process the log queue.
 *
 * Expected result:
 * - The formatted message is accepted and processed without fault.
 *
 * @see log_generic()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST_USER(test_log_core_additional, test_log_generic_user)
{
	uint32_t source_id = 0;

	call_log_generic(source_id, "log generic\n");
	while (log_process()) {
	}
}

/**
 * @brief Verify runtime severity filtering can be configured from user space.
 *
 * @details
 * log_filter_set() adjusts the runtime severity threshold for a source/backend
 * combination at runtime. The test invokes it from a user-mode thread to set the
 * warning level for the local domain, confirming runtime filtering is reachable
 * and accepted from user space.
 *
 * Test steps:
 * - From user space, call log_filter_set() with LOG_LEVEL_WRN for the local
 *   domain.
 *
 * Expected result:
 * - The runtime filter is applied without fault.
 *
 * @see log_filter_set()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-10
 */
ZTEST_USER(test_log_core_additional, test_log_filter_set)
{
	log_filter_set(NULL, Z_LOG_LOCAL_DOMAIN_ID, 0, LOG_LEVEL_WRN);
}

/**
 * @brief Verify log_panic() flushes pending messages from user space.
 *
 * @details
 * log_panic() forces the logging subsystem into synchronous mode and flushes all
 * pending messages, as used before halting on a fatal error. The test emits
 * messages from a user-mode thread and then calls log_panic() to confirm the
 * flush path is reachable and completes from user space.
 *
 * Test steps:
 * - From user space, emit two info messages.
 * - Call log_panic() to flush pending messages.
 *
 * Expected result:
 * - Pending messages are flushed and the panic path completes without fault.
 *
 * @see log_panic()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-13
 */
ZTEST_USER(test_log_core_additional, test_log_panic)
{
	int d = 100;

	LOG_INF("log from user\n");
	LOG_INF("log from user, level %d\n", d);

	log_panic();
}

/**
 * @brief Verify LOG_PRINTK() routes printk-style output through logging from
 * user space.
 *
 * @details
 * LOG_PRINTK() funnels printk-style output into the logging subsystem so it can
 * be processed alongside regular log messages. The test issues a LOG_PRINTK()
 * from a user-mode thread and drains the queue, confirming the path is reachable
 * from user space.
 *
 * Test steps:
 * - From user space, emit output via LOG_PRINTK().
 * - Process the log queue.
 *
 * Expected result:
 * - The printk-style output is processed without fault.
 *
 * @see LOG_PRINTK()
 * @ingroup logging_tests
 */
ZTEST_USER(test_log_core_additional, test_log_printk_from_user)
{
	LOG_PRINTK("test_printk");
	while (log_process()) {
	}
}

#endif /** CONFIG_USERSPACE **/
