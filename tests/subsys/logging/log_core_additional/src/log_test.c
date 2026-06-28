/*
 * Copyright (c) 2020-2025 Intel Corporation
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
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/libc-hooks.h>

#define TEST_MESSAGE "test msg"

#define LOG_MODULE_NAME log_test
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);
static K_SEM_DEFINE(log_sem, 0, 1);

#define TIMESTAMP_FREC (2000000)
ZTEST_BMEM uint32_t source_id;
/* used when log_msg create in user space */
ZTEST_BMEM uint8_t domain, level;
ZTEST_DMEM uint32_t msg_data = 0x1234;

/* Memory domain for tests with threads in user space (ZTEST_USER) */
#if CONFIG_USERSPACE
static struct k_mem_domain mem_domain;
static struct k_mem_partition *mem_parts[] = {
#if Z_LIBC_PARTITION_EXISTS
	&z_libc_partition,
#endif
	&ztest_mem_partition,
	&k_log_partition
};
#endif

static uint8_t buf;
static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(data);
	ARG_UNUSED(ctx);
	return length;
}
LOG_OUTPUT_DEFINE(log_output, char_out, &buf, 1);

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
	/* How many messages have been logged.
	 * used in async mode, to make sure all logs have been handled by compare
	 * counter with total_logs
	 */
	size_t total_logs;
};

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	uint32_t flags;
	struct backend_cb *cb = (struct backend_cb *)backend->cb->ctx;

	/* If printk message skip it. */
	if (log_msg_get_level(&(msg->log)) == LOG_LEVEL_INTERNAL_RAW_STRING) {
		return;
	}

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		cb->sync++;
	}

	if (cb->check_domain_id) {
		zassert_equal(log_msg_get_domain(&(msg->log)), Z_LOG_LOCAL_DOMAIN_ID,
				"Unexpected domain id");
	}

	if (cb->check_timestamp) {
		uint32_t exp_timestamp = cb->exp_timestamps[cb->counter];

		zassert_equal(log_msg_get_timestamp(&(msg->log)), exp_timestamp,
			      "Unexpected message index");
	}

	if (cb->check_severity) {
		zassert_equal(log_msg_get_level(&(msg->log)),
			      cb->exp_severity[cb->counter],
			      "Unexpected log severity");
	}

	if (k_is_user_context()) {
		zassert_equal(log_msg_get_domain(&(msg->log)), domain,
				"Unexpected domain id");

		zassert_equal(log_msg_get_level(&(msg->log)), level,
			      "Unexpected log severity");
	}

	flags = log_backend_std_get_flags();
	log_output_msg_process(&log_output, &msg->log, flags);

	cb->counter++;
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		if (cb->counter == cb->total_logs) {
			k_sem_give(&log_sem);
		}
	}
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

const struct log_backend_api log_backend_test_api = {
	.process = process,
	.panic = panic,
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);
struct backend_cb backend1_cb;

LOG_BACKEND_DEFINE(backend2, log_backend_test_api, false);
struct backend_cb backend2_cb;

/* The logging system support user customize timestamping in log messages
 * by register a timestamp function, in timestamp_get() below, just return
 * a counter as timestamp for different messages.
 * when install this timestamp function, timestamping frequency is set to
 * 2000000, means 2 timestamp/us
 */
#ifndef CONFIG_USERSPACE
static uint32_t stamp;
static uint32_t timestamp_get(void)
{
	stamp++;
	return log_output_timestamp_to_us(stamp * 2);
}

static void log_setup(bool backend2_enable)
{
	stamp = 0U;

	log_init();
#ifndef CONFIG_LOG_PROCESS_THREAD
	log_thread_set(k_current_get());
#endif

	/* Drain any log messages queued before this test (e.g. a boot-time
	 * LOG_WRN from the kernel) so they are dispatched to whichever
	 * backends were already active.  Without this, the freshly enabled
	 * test backend below would also receive those pre-test messages on
	 * its first dispatch and the counter would start above zero.
	 */
	log_flush();

	memset(&backend1_cb, 0, sizeof(backend1_cb));

	log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

	if (backend2_enable) {
		memset(&backend2_cb, 0, sizeof(backend2_cb));

		log_backend_enable(&backend2, &backend2_cb, LOG_LEVEL_DBG);
	} else {
		log_backend_disable(&backend2);
	}
}

#endif

static bool log_test_process(void)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		log_flush();
		/* waiting for all logs have been handled */
		k_sem_take(&log_sem, K_FOREVER);
		return false;
	} else {
		return log_process();
	}
}

/* __DOXYGEN__ is predefined in the traceability build (which defines
 * CONFIG_USERSPACE) so the requirement-annotated supervisor-mode tests below
 * stay visible to Doxygen.
 */
#if !defined(CONFIG_USERSPACE) || defined(__DOXYGEN__)

/**
 * @brief Verify dynamic activation and deactivation of a logging backend.
 *
 * @details
 * A logging backend can be brought online and taken offline at runtime so that
 * log messages are routed to (or withheld from) a given system resource. The
 * test exercises the backend lifecycle API and confirms the active state is
 * reported correctly after each transition.
 *
 * Test steps:
 * - Initialize the logging core.
 * - Activate the test backend and confirm it reports as active.
 * - Deactivate it and confirm it reports as inactive.
 *
 * Expected result:
 * - log_backend_is_active() tracks the activate/deactivate transitions.
 *
 * @see log_backend_activate()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 * @verifies ZEP-SRS-11-21
 */
ZTEST(test_log_core_additional, test_log_backend)
{
	log_init();

	zassert_false(log_backend_is_active(&backend1));
	log_backend_activate(&backend1, NULL);
	zassert_true(log_backend_is_active(&backend1));
	log_backend_deactivate(&backend1);
	zassert_false(log_backend_is_active(&backend1));
}

/**
 * @brief Verify a dispatched message carries the local domain identifier.
 *
 * @details
 * The logging core tags each message with the domain (processor) that produced
 * it so that multi-domain deployments can attribute messages to their source.
 * The test emits a message and confirms the backend receives it stamped with
 * the statically configured local domain id.
 *
 * Test steps:
 * - Set up the logging core with a single backend that checks the domain id.
 * - Emit one informational message.
 * - Process the log and confirm the backend received it.
 *
 * Expected result:
 * - The backend receives the message with the expected local domain id.
 *
 * @see LOG_INF()
 * @ingroup logging_tests
 */
ZTEST(test_log_core_additional, test_log_domain_id)
{
	log_setup(false);

	backend1_cb.check_domain_id = true;
	backend1_cb.total_logs = 1;

	LOG_INF("info message for domain id test");

	while (log_test_process()) {
	}

	zassert_equal(backend1_cb.total_logs, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend");
}

/**
 * @brief Verify messages are processed immediately in immediate logging mode.
 *
 * @details
 * In immediate mode each log message is rendered and delivered in the context
 * that generated it rather than being queued for deferred processing. The test
 * emits messages with CONFIG_LOG_MODE_IMMEDIATE enabled and confirms the
 * backend handled them synchronously without an explicit processing step.
 *
 * Test steps:
 * - Skip the test unless CONFIG_LOG_MODE_IMMEDIATE is enabled.
 * - Set up the logging core and emit two informational messages.
 * - Confirm both were handled synchronously by the backend.
 *
 * Expected result:
 * - Both messages are delivered immediately, with no deferred processing.
 *
 * @see LOG_INF()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-8
 */
ZTEST(test_log_core_additional, test_log_sync)
{
	TC_PRINT("Logging synchronously\n");

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
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
 * @brief Verify messages logged before a backend is active are buffered and
 * later delivered.
 *
 * @details
 * In deferred mode log messages are queued by the core and processed later, so
 * messages generated before any backend is enabled must not be lost. The test
 * emits messages while no backend is active, then enables a backend and
 * confirms all the buffered messages are eventually delivered.
 *
 * Test steps:
 * - Skip in immediate mode; otherwise initialize the core and deactivate all
 *   backends.
 * - Emit info, warning and error messages with no backend active.
 * - Enable the test backend, process the queue and confirm all messages arrive.
 *
 * Expected result:
 * - Every message logged before backend activation is delivered after it.
 *
 * @see LOG_INF()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-6
 */
ZTEST(test_log_core_additional, test_log_early_logging)
{
	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		ztest_test_skip();
	} else {
		log_init();

		/* deactivate other backends */
		STRUCT_SECTION_FOREACH(log_backend, backend) {
			if (strcmp(backend->name, "test")) {
				log_backend_deactivate(backend);
			}
		}

		TC_PRINT("Create log message before backend active\n");

		LOG_INF("log info before backend active");
		LOG_WRN("log warn before backend active");
		LOG_ERR("log error before backend active");

		TC_PRINT("Activate backend with context\n");
		memset(&backend1_cb, 0, sizeof(backend1_cb));
		backend1_cb.total_logs = 3;
		log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

		while (log_test_process()) {
		}

		zassert_equal(backend1_cb.total_logs, backend1_cb.counter,
			      "Unexpected amount of messages received. %zu",
			      backend1_cb.counter);
	}
}

/**
 * @brief Verify messages are delivered with their severity level and below-level
 * messages are removed at compile time.
 *
 * @details
 * The module is registered at LOG_LEVEL_INF, so debug messages are stripped from
 * the build while info, warning and error messages are retained. The test emits
 * one message at each retained level and confirms the backend receives them with
 * the correct severity, demonstrating both multi-level severity support and
 * compile-time level filtering.
 *
 * Test steps:
 * - Set up the logging core with a backend that checks severity.
 * - Emit info, warning and error messages.
 * - Process the log and confirm each message carries its expected severity.
 *
 * Expected result:
 * - Three messages are delivered, each tagged with its severity level.
 *
 * @see LOG_INF()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-7
 * @verifies ZEP-SRS-11-9
 */
ZTEST(test_log_core_additional, test_log_severity)
{
	log_setup(false);

	backend1_cb.check_severity = true;
	backend1_cb.exp_severity[0] = LOG_LEVEL_INF;
	backend1_cb.exp_severity[1] = LOG_LEVEL_WRN;
	backend1_cb.exp_severity[2] = LOG_LEVEL_ERR;

	LOG_INF("info message");
	LOG_WRN("warning message");
	LOG_ERR("error message");
	backend1_cb.total_logs = 3;

	while (log_test_process()) {
	}

	zassert_equal(backend1_cb.total_logs, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
}

/**
 * @brief Verify a user-supplied timestamp function stamps log messages.
 *
 * @details
 * The logging core allows the application to register a custom timestamp source
 * and frequency so that each message records the time it was generated. The test
 * registers a counter-based timestamp function, emits messages and confirms each
 * one is stamped with the expected monotonically increasing value, and that a
 * NULL function is rejected.
 *
 * Test steps:
 * - Register a custom timestamp function (and verify NULL is rejected).
 * - Enable the backend with timestamp checking and expected values.
 * - Emit three messages and confirm each carries its expected timestamp.
 *
 * Expected result:
 * - Each delivered message carries the timestamp produced by the custom source.
 *
 * @see log_set_timestamp_func()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-14
 * @verifies ZEP-SRS-11-15
 */
ZTEST(test_log_core_additional, test_log_timestamping)
{
	stamp = 0U;

	log_init();
	/* deactivate all other backend */
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if ((backend == &backend1) || (backend == &backend2)) {
			log_backend_deactivate(backend);
		}
	}

	TC_PRINT("Register timestamp function\n");
	zassert_equal(-EINVAL, log_set_timestamp_func(NULL, 0),
		      "Expects successful timestamp function setting.");
	zassert_equal(0, log_set_timestamp_func(timestamp_get, TIMESTAMP_FREC),
		      "Expects successful timestamp function setting.");

	memset(&backend1_cb, 0, sizeof(backend1_cb));
	log_backend_enable(&backend1, &backend1_cb, LOG_LEVEL_DBG);

	backend1_cb.check_timestamp = true;

	backend1_cb.exp_timestamps[0] = 1U;
	backend1_cb.exp_timestamps[1] = 2U;
	backend1_cb.exp_timestamps[2] = 3U;

	LOG_INF("test timestamp");
	LOG_INF("test timestamp");
	LOG_WRN("test timestamp");
	backend1_cb.total_logs = 3;

	while (log_test_process()) {
	}

	zassert_equal(backend1_cb.total_logs,
		      backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
}

/**
 * @brief Verify the logging system supports multiple concurrent backends.
 *
 * @details
 * Log messages can be routed to several backends (system resources) at once.
 * The test enables two test backends and confirms at least two backends are
 * registered, and additionally that the UART backend is present when configured,
 * demonstrating that logging can target multiple destinations simultaneously.
 *
 * Test steps:
 * - Enable both test backends via the setup helper.
 * - Count the registered backends and assert there are at least two.
 * - When CONFIG_LOG_BACKEND_UART is set, confirm the UART backend is present.
 *
 * Expected result:
 * - Multiple backends are registered and discoverable concurrently.
 *
 * @see log_backend_enable()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 */

#define UART_BACKEND "log_backend_uart"
ZTEST(test_log_core_additional, test_multiple_backends)
{
	int cnt;

	TC_PRINT("Test multiple backends\n");
	/* enable both backend1 and backend2 */
	log_setup(true);
	STRUCT_SECTION_COUNT(log_backend, &cnt);
	zassert_true((cnt >= 2),
		     "There is no multi backends");

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART)) {
		bool have_uart = false;

		STRUCT_SECTION_FOREACH(log_backend, backend) {
			if (strcmp(backend->name, UART_BACKEND) == 0) {
				have_uart = true;
			}
		}
		zassert_true(have_uart, "There is no UART log backend found");
	}
}

/**
 * @brief Verify queued log messages are processed by the dedicated logging
 * thread.
 *
 * @details
 * When the dedicated logging thread is enabled, messages are buffered and
 * processed asynchronously by that thread, isolating log processing from the
 * contexts that generate messages. The test emits messages, confirms data is
 * pending, then waits for the logging thread to drain the queue.
 *
 * Test steps:
 * - Set up the logging core and confirm no data is pending.
 * - Emit info, warning and error messages and confirm data is now pending.
 * - Wait for the logging thread to process and confirm the queue is drained.
 *
 * Expected result:
 * - The dedicated logging thread delivers all messages and clears the queue.
 *
 * @see log_data_pending()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-1
 * @verifies ZEP-SRS-11-6
 */

#ifdef CONFIG_LOG_PROCESS_THREAD
ZTEST(test_log_core_additional, test_log_thread)
{
	TC_PRINT("Logging buffer is configured to %d bytes\n",
		 CONFIG_LOG_BUFFER_SIZE);

	TC_PRINT("Stack size of logging thread is configured by ");
	TC_PRINT("CONFIG_LOG_PROCESS_THREAD_STACK_SIZE: %d bytes\n",
		 CONFIG_LOG_PROCESS_THREAD_STACK_SIZE);

	log_setup(false);

	zassert_false(log_data_pending());

	LOG_INF("log info to log thread");
	LOG_WRN("log warning to log thread");
	LOG_ERR("log error to log thread");

	zassert_true(log_data_pending());

	/* wait 2 seconds for logging thread to handle this log message*/
	k_sleep(K_MSEC(2000));
	zassert_equal(3, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
	zassert_false(log_data_pending());
}
#else
ZTEST(test_log_core_additional, test_log_thread)
{
	ztest_test_skip();
}
#endif

/**
 * @brief Verify the logging thread can be triggered to process queued messages
 * immediately.
 *
 * @details
 * The dedicated logging thread normally processes buffered messages on its own
 * schedule, but it can be triggered to drain the queue as soon as possible. The
 * test emits messages, confirms they are pending, triggers the thread and
 * confirms the queue is drained shortly afterwards.
 *
 * Test steps:
 * - Set up the logging core and confirm no data is pending.
 * - Emit info, warning and error messages and confirm data is pending.
 * - Call log_thread_trigger(), then confirm the queue is drained.
 *
 * Expected result:
 * - Triggering the logging thread promptly delivers all queued messages.
 *
 * @see log_thread_trigger()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-1
 * @verifies ZEP-SRS-11-6
 */

#ifdef CONFIG_LOG_PROCESS_THREAD
ZTEST(test_log_core_additional, test_log_thread_trigger)
{
	log_setup(false);

	zassert_false(log_data_pending());

	LOG_INF("log info to log thread");
	LOG_WRN("log warning to log thread");
	LOG_ERR("log error to log thread");

	zassert_true(log_data_pending());

	/* Trigger log thread to process messages as soon as possible. */
	log_thread_trigger();

	/* wait 1ms to give logging thread chance to handle these log messages. */
	k_sleep(K_MSEC(1));
	zassert_equal(3, backend1_cb.counter,
		      "Unexpected amount of messages received by the backend.");
	zassert_false(log_data_pending());
}
#else
ZTEST(test_log_core_additional, test_log_thread_trigger)
{
	ztest_test_skip();
}
#endif

static void call_log_generic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_generic(LOG_LEVEL_INF, fmt, ap);
	va_end(ap);
}

/**
 * @brief Verify log_generic() renders messages with printf-style arguments.
 *
 * @details
 * log_generic() accepts a format string and a va_list, letting callers emit log
 * messages with printf-style argument substitution. The test issues several
 * messages with differing format specifiers and argument counts and confirms the
 * core accepts and processes each one.
 *
 * Test steps:
 * - Set up the logging core with a single backend.
 * - Call log_generic() with no-arg, string and integer format variants.
 * - Process the log queue.
 *
 * Expected result:
 * - All formatted messages are accepted and processed without error.
 *
 * @see log_generic()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-3
 */
ZTEST(test_log_core_additional, test_log_generic)
{
	char *log_msg = "log user space";
	int i = 100;

	log_setup(false);
	backend1_cb.total_logs = 4;

	call_log_generic("log generic");
	call_log_generic("log generic: %s", log_msg);
	call_log_generic("log generic %d\n", i);
	call_log_generic("log generic %d, %d\n", i, 1);
	while (log_test_process()) {
	}
}

/**
 * @brief Verify log messages can be created via the low-level message
 * construction primitives.
 *
 * @details
 * The logging core exposes runtime, stack and macro-based primitives for
 * building a log message with explicit domain, source, level and payload. The
 * test, in deferred mode, constructs messages through each primitive and
 * confirms they are accepted and processed by the backend.
 *
 * Test steps:
 * - Set up the logging core; run only in deferred mode.
 * - Create messages via z_log_msg_runtime_create(), Z_LOG_MSG_STACK_CREATE()
 *   and Z_LOG_MSG_CREATE() with explicit domain and level.
 * - Process the log queue.
 *
 * Expected result:
 * - Messages built through each primitive are processed successfully.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 */
ZTEST(test_log_core_additional, test_log_msg_create)
{
	log_setup(false);
	if (IS_ENABLED(CONFIG_LOG_MODE_DEFERRED)) {
		int mode;

		domain = 3;
		level = 2;

		z_log_msg_runtime_create(domain, __log_current_const_data,
					  level, &msg_data, 0,
					  sizeof(msg_data), NULL);
		/* try z_log_msg_static_create() */
		Z_LOG_MSG_STACK_CREATE(0, domain, __log_current_const_data,
					level, &msg_data,
					sizeof(msg_data), NULL);

		Z_LOG_MSG_CREATE(!IS_ENABLED(CONFIG_USERSPACE), mode,
			  Z_LOG_LOCAL_DOMAIN_ID, NULL,
			  LOG_LEVEL_INF, NULL, 0, TEST_MESSAGE);

		backend1_cb.total_logs = 3;

		while (log_test_process()) {
		}
	}
}

#else

/**
 * @brief Verify log messages can be created from user space via the low-level
 * primitives.
 *
 * @details
 * Mirrors the message-construction coverage in supervisor mode, but exercises
 * the runtime, stack and macro message-creation primitives from a user-mode
 * thread to confirm they are usable across the user/kernel boundary. The
 * constructed messages are then processed by the core.
 *
 * Test steps:
 * - From a user-mode thread, build messages via z_log_msg_runtime_create(),
 *   Z_LOG_MSG_STACK_CREATE() and Z_LOG_MSG_CREATE().
 * - Process the log queue.
 *
 * Expected result:
 * - Messages built from user space are processed without fault.
 *
 * @see z_log_msg_runtime_create()
 * @ingroup logging_tests
 */
ZTEST_USER(test_log_core_additional, test_log_msg_create_user)
{
	int mode;

	domain = 3;
	level = 2;

	z_log_msg_runtime_create(domain, NULL,
				  level, &msg_data, 0,
				  sizeof(msg_data), TEST_MESSAGE);
	/* try z_log_msg_static_create() */
	Z_LOG_MSG_STACK_CREATE(0, domain, NULL,
				level, &msg_data,
				sizeof(msg_data), TEST_MESSAGE);

	Z_LOG_MSG_CREATE(!IS_ENABLED(CONFIG_USERSPACE), mode,
			  Z_LOG_LOCAL_DOMAIN_ID, NULL,
		  LOG_LEVEL_INTERNAL_RAW_STRING, NULL, 0, TEST_MESSAGE);

	while (log_test_process()) {
	}
}

#endif /** CONFIG_USERSPACE **/

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

static void *test_log_core_additional_setup(void)
{
#ifdef CONFIG_LOG_PROCESS_THREAD
	k_thread_foreach(promote_log_thread, NULL);
#endif

#if CONFIG_USERSPACE
	/* Set memory domain for threads in user space */
	k_mem_domain_init(&mem_domain, ARRAY_SIZE(mem_parts), mem_parts);
	k_mem_domain_add_thread(&mem_domain, k_current_get());
#endif
	return NULL;
}

ZTEST_SUITE(test_log_core_additional, NULL, test_log_core_additional_setup, NULL, NULL, NULL);
