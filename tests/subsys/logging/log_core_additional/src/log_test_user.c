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

/* test log_hexdump_from_user() from user space */
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

/* test log_generic() from user space */
ZTEST_USER(test_log_core_additional, test_log_generic_user)
{
	uint32_t source_id = 0;

	call_log_generic(source_id, "log generic\n");
	while (log_process()) {
	}
}

/* test log_filter_set from user space */
ZTEST_USER(test_log_core_additional, test_log_filter_set)
{
	log_filter_set(NULL, Z_LOG_LOCAL_DOMAIN_ID, 0, LOG_LEVEL_WRN);
}

/* test log_panic() from user space */
ZTEST_USER(test_log_core_additional, test_log_panic)
{
	int d = 100;

	LOG_INF("log from user\n");
	LOG_INF("log from user, level %d\n", d);

	log_panic();
}

#endif /** CONFIG_USERSPACE **/
