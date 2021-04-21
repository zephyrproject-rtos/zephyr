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

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

/* Interfaces tested in these cases have been tested in kernel space.
 * Test cases in this file run in user space to improve test coverage
 */
void test_log_from_user(void)
{
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INF,
		.domain_id = CONFIG_LOG_DOMAIN_ID,
		.source_id = 0,
	};
	uint32_t cnt0, cnt1;

	log_from_user(src_level, "log from user\n");
	log_from_user(src_level, "log from user, level %d\n", src_level.level);
	cnt0 = log_buffered_cnt();
	while (log_process(false)) {
	}
	cnt1 = log_buffered_cnt();
	zassert_true(cnt1 <= cnt0, "no message is handled");
}

void test_log_hexdump_from_user(void)
{
	int32_t data = 128;
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INF,
		.domain_id = CONFIG_LOG_DOMAIN_ID,
		.source_id = 0,
	};

	log_hexdump_from_user(src_level, "test_hexdump", &data, sizeof(data));
	while (log_process(false)) {
	}
}

static void call_log_generic(uint32_t source_id, const char *fmt, ...)
{
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INF,
		.domain_id = CONFIG_LOG_DOMAIN_ID,
		.source_id = source_id,
	};

	va_list ap;

	va_start(ap, fmt);
	log_generic(src_level, fmt, ap, LOG_STRDUP_SKIP);
	va_end(ap);
}

void test_log_generic(void)
{
	uint32_t source_id = 0;

	call_log_generic(source_id, "log generic\n");
	while (log_process(false)) {
	}
}

void test_log_filter_set(void)
{
	log_filter_set(NULL, CONFIG_LOG_DOMAIN_ID, 0, LOG_LEVEL_WRN);
}

void test_log_panic(void)
{
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_ERR,
		.domain_id = CONFIG_LOG_DOMAIN_ID,
		.source_id = 0,
	};

	log_from_user(src_level, "log from user\n");
	log_from_user(src_level, "log from user, level %d\n", src_level.level);

	log_panic();
}

void test_main(void)
{
	ztest_test_suite(test_log_list,
			 ztest_user_unit_test(test_log_from_user),
			 ztest_user_unit_test(test_log_hexdump_from_user),
			 ztest_user_unit_test(test_log_generic),
			 ztest_user_unit_test(test_log_filter_set),
			 ztest_user_unit_test(test_log_panic));
	ztest_run_test_suite(test_log_list);
}
