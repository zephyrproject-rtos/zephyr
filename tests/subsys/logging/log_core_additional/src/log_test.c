/*
 * Copyright (c) 2020 Intel Corporation
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

#define LOG_MODULE_NAME log_test
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

struct backend_cb {
	/* count log messages handled by this backend */
	size_t counter;
	/* count log messages handled immediately by this backend */
	size_t sync;
	/* inform put() to check timestamp of message against exp_timestamps */
	bool check_timestamp;
	uint32_t exp_timestamps[16];
	/* inform put() to check severity of message against exp_severity */
	bool check_severity;
	uint16_t exp_severity[4];
	/* inform put() to check domain_id of message */
	bool check_domain_id;
};

static void put(struct log_backend const *const backend, struct log_msg *msg)
{
	log_msg_get(msg);
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	if (cb->check_domain_id) {
		zassert_equal(log_msg_domain_id_get(msg), CONFIG_LOG_DOMAIN_ID,
				"Unexpected domain id");
	}

	if (cb->check_timestamp) {
		uint32_t exp_timestamp = cb->exp_timestamps[cb->counter];

		zassert_equal(log_msg_timestamp_get(msg), exp_timestamp,
			      "Unexpected message index");
	}

	if (cb->check_severity) {
		zassert_equal(log_msg_level_get(msg),
			      cb->exp_severity[cb->counter],
			      "Unexpected log severity");
	}

	cb->counter++;
	log_msg_put(msg);
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	cb->counter++;
	cb->sync++;
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data,
			 uint32_t length)
{
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	cb->counter++;
	cb->sync++;
}

const struct log_backend_api log_backend_test_api = {
	.put = put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);
struct backend_cb backend1_cb;

LOG_BACKEND_DEFINE(backend2, log_backend_test_api, false);
struct backend_cb backend2_cb;

/* The logging system support user customize timestamping in log messages
 * by register a timestamp function, in timestamp_get() below, just return
 * a counter as timestamp for different messages.
 */
static uint32_t stamp;
static uint32_t timestamp_get(void)
{
	return stamp++;
}

static void log_setup(bool backend2_enable)
{
	stamp = 0U;

	log_init();

	memset(&backend1_cb, 0, sizeof(backend1_cb));

	log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

	if (backend2_enable) {
		memset(&backend2_cb, 0, sizeof(backend2_cb));

		log_backend_enable(&backend2, &backend2_cb, LOG_LEVEL_DBG);
	} else {
		log_backend_disable(&backend2);
	}
}

static bool log_test_process(bool bypass)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		/* a dedicate thread handle log messages, wait for 1 second */
		k_sleep(K_MSEC(1000));
		return false;
	} else {
		return log_process(bypass);
	}
}

/**
 * @brief Support multi-processor systems
 *
 * @details Logging system identify domain/processor by domain_id which is now
 *          statically configured by CONFIG_LOG_DOMAIN_ID
 *
 * @addtogroup logging
 */

static void test_log_domain_id(void)
{
	log_setup(false);

	backend1_cb.check_domain_id = true;

	LOG_INF("info message for domain id test");

	while (log_test_process(false)) {
	}

	zassert_equal(1, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend");
}

/**
 * @brief Synchronous processing of logging messages.
 *
 * @details if CONFIG_LOG_IMMEDIATE is enabled, log message is
 *          handled immediately
 *
 * @addtogroup logging
 */

static void test_log_sync(void)
{
	TC_PRINT("Logging synchronousely\n");

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		log_setup(false);
		LOG_INF("Log immediately");
		LOG_INF("Log immediately");

		/* log immediately, no log_process needed */
		zassert_equal(2, backend1_cb.sync,
			      "Unexpected amount of messages received by the backend.");
	} else {
		ztest_test_skip();
	}
}

/**
 * @brief Early logging
 * @details Handle log message attempts as well as creating new log contexts
 *         instance, before the backend are active
 *
 * @addtogroup logging
 */

static void test_log_early_logging(void)
{
	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		ztest_test_skip();
	} else {
		log_init();

		/* deactivate other backends */
		const struct log_backend *backend;

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);
			if (strcmp(backend->name, "test")) {
				log_backend_deactivate(backend);
			}
		}

		TC_PRINT("Create log message before backend active\n");

		LOG_INF("log info before backend active");
		LOG_WRN("log warn before backend active");
		LOG_ERR("log error before backend active");

		TC_PRINT("Activate backend with context");
		memset(&backend1_cb, 0, sizeof(backend1_cb));
		log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

		while (log_test_process(false)) {
		}

		zassert_equal(3, backend1_cb.counter,
			      "Unexpected amount of messages received. %d",
			      backend1_cb.counter);
	}
}

/**
 * @brief Log severity
 *
 * @details This module is registered with LOG_LEVEL_INF, LOG_LEVEL_DBG will be
 *          filtered out at compile time, only 3 message handled
 *
 * @addtogroup logging
 */

static void test_log_severity(void)
{
	log_setup(false);

	backend1_cb.check_severity = true;
	backend1_cb.exp_severity[0] = LOG_LEVEL_INF;
	backend1_cb.exp_severity[1] = LOG_LEVEL_WRN;
	backend1_cb.exp_severity[2] = LOG_LEVEL_ERR;

	LOG_INF("info message");
	LOG_WRN("warning message");
	LOG_ERR("error message");

	while (log_test_process(false)) {
	}

	zassert_equal(3, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
}

/**
 * @brief Customizable timestamping in log messages
 *
 * @details Log core permit user to register customized timestamp function
 *
 * @addtogroup logging
 */

static void test_log_timestamping(void)
{
	stamp = 0U;

	log_init();
	/* deactivate all other backend */
	const struct log_backend *backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);
		log_backend_deactivate(backend);
	}

	TC_PRINT("Register timestamp function\n");
	zassert_equal(0, log_set_timestamp_func(timestamp_get, 0),
		      "Expects successful timestamp function setting.");

	memset(&backend1_cb, 0, sizeof(backend1_cb));
	log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

	backend1_cb.check_timestamp = true;

	backend1_cb.exp_timestamps[0] = 0U;
	backend1_cb.exp_timestamps[1] = 1U;
	backend1_cb.exp_timestamps[2] = 2U;

	LOG_INF("test timestamp");
	LOG_INF("test timestamp");
	LOG_WRN("test timestamp");

	while (log_test_process(false)) {
	}

	zassert_equal(3,
		      backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
}

/**
 * @brief Multiple logging backends
 *
 * @details Enable two backends in this module and enable UART backend
 *          by CONFIG_LOG_BACKEND_UART, there are three backends at least.
 *
 * @addtogroup logging
 */

#define UART_BACKEND "log_backend_uart"
static void test_multiple_backends(void)
{
	TC_PRINT("Test multiple backends");
	/* enable both backend1 and backend2 */
	log_setup(true);
	zassert_true((log_backend_count_get() >= 2),
		     "There is no multi backends");

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART)) {
		bool have_uart = false;
		struct log_backend const *backend;

		for (int i = 0; i < log_backend_count_get(); i++) {
			backend = log_backend_get(i);
			if (strcmp(backend->name, UART_BACKEND) == 0) {
				have_uart = true;
			}
		}
		zassert_true(have_uart, "There is no UART log backend found");
	}
}

/**
 * @brief Process all logging activities using a dedicated thread
 *
 * @addtogroup logging
 */

#ifdef CONFIG_LOG_PROCESS_THREAD
static void test_log_thread(void)
{
	TC_PRINT("Logging buffer is configured to %d bytes\n",
		 CONFIG_LOG_BUFFER_SIZE);

	TC_PRINT("Stack size of logging thread is configured by");
	TC_PRINT("CONFIG_LOG_PROCESS_THREAD_STACK_SIZE: %d bytes\n",
		 CONFIG_LOG_PROCESS_THREAD_STACK_SIZE);

	log_setup(false);
	LOG_INF("log info to log thread");
	LOG_WRN("log warning to log thread");
	LOG_ERR("log error to log thread");
	/* wait 2 seconds for logging thread to handle this log message*/
	k_sleep(K_MSEC(2000));
	zassert_equal(3, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");

}
#else
static void test_log_thread(void)
{
	ztest_test_skip();
}
#endif

/* The log process thread has the K_LOWEST_APPLICATION_THREAD_PRIO, adjust it
 * to a higher priority to increase the chances of being scheduled to handle
 * log message as soon as possible
 */
void promote_log_thread(const struct k_thread *thread, void *user_data)
{
	if (!(strcmp(thread->name, "logging"))) {
		k_thread_priority_set((k_tid_t)thread, -1);
	}
}

/*test case main entry*/
void test_main(void)
{
#ifdef CONFIG_LOG_PROCESS_THREAD
	k_thread_foreach(promote_log_thread, NULL);
#endif
	ztest_test_suite(test_log_list,
			 ztest_unit_test(test_multiple_backends),
			 ztest_unit_test(test_log_domain_id),
			 ztest_unit_test(test_log_severity),
			 ztest_unit_test(test_log_timestamping),
			 ztest_unit_test(test_log_early_logging),
			 ztest_unit_test(test_log_sync),
			 ztest_unit_test(test_log_thread));
	ztest_run_test_suite(test_log_list);
}
