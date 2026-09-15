/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log benchmark
 *
 */


#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include "test_helpers.h"

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if LOG_BENCHMARK_DETAILED_PRINT
#define DBG_PRINT(...) PRINT(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

struct backend_cb {
	size_t counter;
	bool panic;
	bool keep_msgs;
	bool check_id;
	uint32_t exp_id[100];
	bool check_timestamp;
	uint32_t exp_timestamps[100];
	bool check_args;
	uint32_t exp_nargs[100];
	bool check_strdup;
	bool exp_strdup[100];
	uint32_t total_drops;
};

static void process(struct log_backend const *const backend,
		    union log_msg_generic *msg)
{
}

static void panic(struct log_backend const *const backend)
{
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	cb->panic = true;
}

static void dropped(struct log_backend const *const backend, uint32_t cnt)
{
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	cb->total_drops += cnt;
}

const struct log_backend_api log_backend_test_api = {
	.process = process,
	.panic = panic,
	.dropped = dropped,
};

LOG_BACKEND_DEFINE(backend, log_backend_test_api, false);
struct backend_cb backend_ctrl_blk;

#define TEST_FORMAT_SPEC(i, _) " %d"
#define TEST_VALUE(i, _), i

/** @brief Count log messages until first drop.
 *
 * Report number of messages that fit in the buffer.
 *
 * @param nargs Number of int arguments in the log message.
 */
#define TEST_LOG_CAPACITY(nargs, inc_cnt, _print) do { \
	int _cnt = 0; \
	test_helpers_log_setup(); \
	while (!test_helpers_log_dropped_pending()) { \
		LOG_ERR("test" LISTIFY(nargs, TEST_FORMAT_SPEC, ()) \
				LISTIFY(nargs, TEST_VALUE, ())); \
		_cnt++; \
	} \
	_cnt--; \
	inc_cnt += _cnt; \
	if (_print) { \
		PRINT("%d log message with %d arguments fit in %d space.\n", \
			_cnt, nargs, CONFIG_LOG_BUFFER_SIZE); \
	} \
	log_flush(); \
} while (0)

/** Test how many messages fits in the logging buffer in deferred mode. Test
 * serves as the comparison between logging versions.
 */
ZTEST(test_log_benchmark, test_log_capacity)
{
	int total_cnt = 0;

	TEST_LOG_CAPACITY(0, total_cnt, 1);
	TEST_LOG_CAPACITY(1, total_cnt, 1);
	TEST_LOG_CAPACITY(2, total_cnt, 1);
	TEST_LOG_CAPACITY(3, total_cnt, 1);
	TEST_LOG_CAPACITY(4, total_cnt, 1);
	TEST_LOG_CAPACITY(5, total_cnt, 1);
	TEST_LOG_CAPACITY(6, total_cnt, 1);
	TEST_LOG_CAPACITY(7, total_cnt, 1);
	TEST_LOG_CAPACITY(8, total_cnt, 1);

	PRINT("In total %d message were stored.\n", total_cnt);
}

#define TEST_LOG_MESSAGE(inc_time, _repeat, ...) do { \
	test_helpers_log_setup(); \
	uint32_t cyc = test_helpers_cycle_get(); \
	for (int i = 0; i < _repeat; i++) { \
		LOG_ERR(__VA_ARGS__); \
	} \
	cyc = test_helpers_cycle_get() - cyc; \
	inc_time += cyc; \
	DBG_PRINT("Message logged in %u cycles (%u us). %d message logged in %u cycles.\n", \
			cyc / _repeat, k_cyc_to_us_ceil32(cyc) / _repeat, _repeat, cyc); \
} while (0)

#define TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(nargs, inc_time, inc_msg) do { \
	int _msg_cnt = 0; \
	TEST_LOG_CAPACITY(nargs, _msg_cnt, 0); \
	TEST_LOG_MESSAGE(inc_time, _msg_cnt, \
		"test" LISTIFY(nargs, TEST_FORMAT_SPEC, ()) \
				LISTIFY(nargs, TEST_VALUE, ())); \
	inc_msg += _msg_cnt; \
} while (0)

static void test_report(uint32_t cyc, uint32_t msg_cnt, int args_cnt, uint32_t init_usage)
{
	uint32_t us = k_cyc_to_us_ceil32(cyc);

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		uint32_t buf_size;
		uint32_t curr_usage;

		zassert_ok(log_mem_get_usage(&buf_size, &curr_usage));
		PRINT("Memory per message:%d\n", (curr_usage - init_usage) / msg_cnt);
	}
	log_flush();
	if (args_cnt < 0) {
		PRINT("%sAverage logging a message with string argument:  %u cycles (%u.%03u us)\n",
			k_is_user_context() ? "USERSPACE: " : "",
			cyc / msg_cnt, us / msg_cnt, ((us % msg_cnt) * 1000) / msg_cnt);
	} else {
		PRINT("%sAverage logging a message with %d arguments:  %u cycles (%u.%03u us)\n",
			k_is_user_context() ? "USERSPACE: " : "",
			args_cnt, cyc / msg_cnt, us / msg_cnt, ((us % msg_cnt) * 1000) / msg_cnt);
	}
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_log0)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 20;
	uint32_t buf_size = 0;
	uint32_t init_usage = 0;

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		zassert_ok(log_mem_get_usage(&buf_size, &init_usage));
		PRINT("init_usage: %d, buf_size: %d\n", init_usage, buf_size);
	}

	TEST_LOG_MESSAGE(total_cyc, total_msg, "test");

	test_report(total_cyc, total_msg, 0, init_usage);
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_log1)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 20;
	uint32_t buf_size = 0;
	uint32_t init_usage = 0;

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		zassert_ok(log_mem_get_usage(&buf_size, &init_usage));
	}

	TEST_LOG_MESSAGE(total_cyc, total_msg, "test %d", 1);

	test_report(total_cyc, total_msg, 1, init_usage);
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_log2)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 20;
	uint32_t buf_size = 0;
	uint32_t init_usage = 0;

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		zassert_ok(log_mem_get_usage(&buf_size, &init_usage));
	}

	TEST_LOG_MESSAGE(total_cyc, total_msg, "test %d %d", 1, 2);

	test_report(total_cyc, total_msg, 2, init_usage);
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_log3)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 20;
	uint32_t buf_size = 0;
	uint32_t init_usage = 0;

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		zassert_ok(log_mem_get_usage(&buf_size, &init_usage));
	}

	TEST_LOG_MESSAGE(total_cyc, total_msg, "test %d %d %d", 1, 2, 3);

	test_report(total_cyc, total_msg, 3, init_usage);
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_log_string)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 20;
	uint32_t buf_size = 0;
	uint32_t init_usage = 0;

	if (IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION)) {
		zassert_ok(log_mem_get_usage(&buf_size, &init_usage));
	}

	char str[] = "test string";

	TEST_LOG_MESSAGE(total_cyc, total_msg, "test with string: %s", str);

	test_report(total_cyc, total_msg, 1, init_usage);
}

static void run_log_message_store_time_no_overwrite(void)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 0;

	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(0, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(1, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(2, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(3, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(4, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(5, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(6, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(7, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_NO_OVERFLOW(8, total_cyc, total_msg);

	uint32_t total_us = k_cyc_to_us_ceil32(total_cyc);

	PRINT("%sAverage logging a message:  %u cycles (%u us)\n",
		k_is_user_context() ? "USERSPACE: " : "",
		total_cyc / total_msg, total_us / total_msg);
}

ZTEST(test_log_benchmark, test_log_message_store_time_no_overwrite)
{
	run_log_message_store_time_no_overwrite();
}

#define TEST_LOG_MESSAGE_STORE_OVERFLOW(nargs, _msg_cnt, inc_time, inc_msg) do { \
	int _dummy = 0; \
	/* Saturate buffer. */ \
	TEST_LOG_CAPACITY(nargs, _dummy, 0); \
	uint32_t cyc = test_helpers_cycle_get(); \
	for (int i = 0; i < _msg_cnt; i++) { \
		LOG_ERR("test" LISTIFY(nargs, TEST_FORMAT_SPEC, ()) \
				LISTIFY(nargs, TEST_VALUE, ())); \
	} \
	cyc = test_helpers_cycle_get() - cyc; \
	inc_time += cyc; \
	inc_msg += _msg_cnt; \
	DBG_PRINT("%d arguments message logged in %u cycles (%u us). " \
		  "%d message logged in %u cycles.\n", \
			nargs, cyc / _msg_cnt, k_cyc_to_us_ceil32(cyc) / _msg_cnt, \
			_msg_cnt, cyc); \
} while (0)

ZTEST(test_log_benchmark, test_log_message_store_time_overwrite)
{
	uint32_t total_cyc = 0;
	uint32_t total_msg = 0;

	TEST_LOG_MESSAGE_STORE_OVERFLOW(0, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(1, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(2, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(3, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(4, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(5, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(6, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(7, 50, total_cyc, total_msg);
	TEST_LOG_MESSAGE_STORE_OVERFLOW(8, 50, total_cyc, total_msg);

	uint32_t total_us = k_cyc_to_us_ceil32(total_cyc);

	PRINT("Average overwrite logging a message:  %u cycles (%u us)\n",
		total_cyc / total_msg, total_us / total_msg);
}

ZTEST_USER(test_log_benchmark, test_log_message_store_time_no_overwrite_from_user)
{
	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		printk("no userspace\n");
		return;
	}

	run_log_message_store_time_no_overwrite();
}

ZTEST(test_log_benchmark, test_log_message_with_string)
{
	test_helpers_log_setup();
	char strbuf[] = "test string";
	uint32_t cyc = test_helpers_cycle_get();
	int repeat = 8;

	for (int i = 0; i < repeat; i++) {
		LOG_ERR("test with string to duplicate: %s", strbuf);
	}

	cyc = test_helpers_cycle_get() - cyc;
	uint32_t us = k_cyc_to_us_ceil32(cyc);

	PRINT("%slogging with transient string %u cycles (%u us).\n",
		k_is_user_context() ? "USERSPACE: " : "",
		cyc / repeat, us / repeat);
}

/*test case main entry*/
static void *log_benchmark_setup(void)
{
	PRINT("LOGGING MODE:%s\n", IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? "DEFERRED" : "IMMEDIATE");
	PRINT("\tOVERWRITE: %d\n", IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW));
	PRINT("\tBUFFER_SIZE: %d\n", CONFIG_LOG_BUFFER_SIZE);
	PRINT("\tSPEED: %d\n", IS_ENABLED(CONFIG_LOG_SPEED));
	PRINT("\tSIMPLE_MSG_COMPRESS: %d\n", IS_ENABLED(CONFIG_LOG_SIMPLE_MSG_COMPRESS));
	PRINT("\tMEM_UTILIZATION: %d\n", IS_ENABLED(CONFIG_LOG_MEM_UTILIZATION));

	log_backend_enable(&backend, &backend_ctrl_blk, LOG_LEVEL_DBG);
	return NULL;
}

ZTEST_SUITE(test_log_benchmark, NULL, log_benchmark_setup, NULL, NULL, NULL);
