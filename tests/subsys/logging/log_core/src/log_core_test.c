/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log core
 *
 */


#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include "test_module.h"

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

typedef void (*custom_put_callback_t)(struct log_backend const *const backend,
				      struct log_msg *msg, size_t counter);

static bool in_panic;

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
	custom_put_callback_t callback;
	uint32_t total_drops;
};

static void put(struct log_backend const *const backend,
		struct log_msg *msg)
{
	log_msg_get(msg);
	uint32_t nargs = log_msg_nargs_get(msg);
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	if (cb->check_id) {
		uint32_t exp_id = cb->exp_id[cb->counter];

		zassert_equal(log_msg_source_id_get(msg),
			      exp_id,
			      "Unexpected source_id");
	}

	if (cb->check_timestamp) {
		uint32_t exp_timestamp = cb->exp_timestamps[cb->counter];

		zassert_equal(log_msg_timestamp_get(msg),
			      exp_timestamp,
			      "Unexpected message index");
	}

	/* Arguments in the test are fixed, 1,2,3,4,5,... */
	if (cb->check_args && log_msg_is_std(msg) && nargs > 0) {
		for (int i = 0; i < nargs; i++) {
			uint32_t arg = log_msg_arg_get(msg, i);

			zassert_equal(i+1, arg,
				      "Unexpected argument in the message");
		}
	}

	if (cb->check_strdup) {
		zassert_false(cb->exp_strdup[cb->counter]
			      ^ log_is_strdup((void *)log_msg_arg_get(msg, 0)),
			      NULL);
	}

	if (cb->callback) {
		cb->callback(backend, msg, cb->counter);
	}

	cb->counter++;

	if (!cb->keep_msgs) {
		log_msg_put(msg);
	}
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
	.put = put,
	.panic = panic,
	.dropped = dropped,
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);
struct backend_cb backend1_cb;

LOG_BACKEND_DEFINE(backend2, log_backend_test_api, false);
struct backend_cb backend2_cb;

static uint32_t stamp;

static uint32_t test_source_id;

static uint32_t timestamp_get(void)
{
	return stamp++;
}

/**
 * @brief Function for finding source ID based on source name.
 *
 * @param name Source name
 *
 * @return Source ID.
 */
static int16_t log_source_id_get(const char *name)
{

	for (int16_t i = 0; i < log_src_cnt_get(CONFIG_LOG_DOMAIN_ID); i++) {
		if (strcmp(log_source_name_get(CONFIG_LOG_DOMAIN_ID, i), name)
		    == 0) {
			return i;
		}
	}
	return -1;
}

static void log_setup(bool backend2_enable)
{
	stamp = 0U;
	zassert_false(in_panic, "Logger in panic state.");

	log_init();

	zassert_equal(0, log_set_timestamp_func(timestamp_get, 0),
		      "Expects successful timestamp function setting.");

	memset(&backend1_cb, 0, sizeof(backend1_cb));

	log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

	if (backend2_enable) {
		memset(&backend2_cb, 0, sizeof(backend2_cb));

		log_backend_enable(&backend2, &backend2_cb, LOG_LEVEL_DBG);
	} else {
		log_backend_disable(&backend2);
	}

	test_source_id = log_source_id_get(STRINGIFY(LOG_MODULE_NAME));
}

static void test_log_strdup_gc(void)
{
	char test_str[] = "test string";
	char *dstr;
	uint32_t size_u0, size_u1, size_l0, size_l1;

	log_setup(false);

	BUILD_ASSERT(CONFIG_LOG_STRDUP_BUF_COUNT == 1,
		     "Test assumes certain configuration");
	backend1_cb.check_strdup = true;
	backend1_cb.exp_strdup[0] = true;
	backend1_cb.exp_strdup[1] = false;

	size_l0 = log_get_strdup_longest_string();
	size_u0 = log_get_strdup_pool_utilization();

	dstr = log_strdup(test_str);
	/* test if message freeing is not fooled by using value within strdup
	 * buffer pool but with different format specifier.
	 */
	LOG_INF("%s %p", dstr, dstr + 1);
	LOG_INF("%s", log_strdup(test_str));

	while (log_process(false)) {
	}

	zassert_equal(2, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");

	/* Processing should free strdup buffer. */
	backend1_cb.exp_strdup[2] = true;
	LOG_INF("%s", log_strdup(test_str));

	while (log_process(false)) {
	}

	zassert_equal(3, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
	size_l1 = log_get_strdup_longest_string();
	size_u1 = log_get_strdup_pool_utilization();
	zassert_true(size_l1 > size_l0, "longest string size never changed");
	zassert_true(size_u1 > size_u0, "strdup pool utilization never changed");
}

#define DETECT_STRDUP_MISSED(str, do_strdup, ...) \
	{\
		char tmp[] = "tmp";\
		uint32_t exp_cnt = backend1_cb.counter + 1 + (do_strdup ? 0 : 1); \
		LOG_ERR(str, ##__VA_ARGS__, do_strdup ? log_strdup(tmp) : tmp); \
		\
		while (log_process(false)) { \
		} \
		\
		zassert_equal(exp_cnt, backend1_cb.counter,\
		"Unexpected amount of messages received by the backend (%d).", \
			backend1_cb.counter); \
	}

static void test_log_strdup_detect_miss(void)
{
	if (IS_ENABLED(CONFIG_LOG_DETECT_MISSED_STRDUP)) {
		return;
	}

	log_setup(false);

	DETECT_STRDUP_MISSED("%s", true);
	DETECT_STRDUP_MISSED("%s", false);

	DETECT_STRDUP_MISSED("%-20s", true);
	DETECT_STRDUP_MISSED("%-20s", false);

	DETECT_STRDUP_MISSED("%20s", true);
	DETECT_STRDUP_MISSED("%20s", false);

	DETECT_STRDUP_MISSED("%20.4s", true);
	DETECT_STRDUP_MISSED("%20.4s", false);

	DETECT_STRDUP_MISSED("%% %s %%", true);
	DETECT_STRDUP_MISSED("%% %s %%", false);

	DETECT_STRDUP_MISSED("%% %08X %s", true, 4);
	DETECT_STRDUP_MISSED("%% %08X %s", false, 4);
}

static void strdup_trim_callback(struct log_backend const *const backend,
			  struct log_msg *msg, size_t counter)
{
	char *str = (char *)log_msg_arg_get(msg, 0);
	size_t len = strlen(str);

	zassert_equal(len, CONFIG_LOG_STRDUP_MAX_STRING,
			"Expected trimmed string");
}

static void test_strdup_trimming(void)
{
	char test_str[] = "123456789";

	BUILD_ASSERT(CONFIG_LOG_STRDUP_MAX_STRING == 8,
		     "Test assumes certain configuration");

	log_setup(false);

	backend1_cb.callback = strdup_trim_callback;

	LOG_INF("%s", log_strdup(test_str));

	while (log_process(false)) {
	}

	zassert_equal(1, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
}

static void test_single_z_log_get_s_mask(const char *str, uint32_t nargs,
					 uint32_t exp_mask)
{
	uint32_t mask = z_log_get_s_mask(str, nargs);

	zassert_equal(mask, exp_mask, "Unexpected mask %x (expected %x)",
								mask, exp_mask);
}

static void test_z_log_get_s_mask(void)
{
	test_single_z_log_get_s_mask("%d%%%-10s%p%x", 4, 0x2);
	test_single_z_log_get_s_mask("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d"
				     "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%s",
				     32, 0x80000000);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_core,
			 ztest_unit_test(test_log_strdup_gc),
			 ztest_unit_test(test_log_strdup_detect_miss),
			 ztest_unit_test(test_strdup_trimming),
			 ztest_unit_test(test_z_log_get_s_mask)
			 );
	ztest_run_test_suite(test_log_core);
}
