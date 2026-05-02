/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* maximum time to wait before aborting thread in case of 0 log messages / second */
#define MAX_STALL_TIMEOUT_S 3
/* maximum time (seconds) to wait for logging thread to finish */
#define MAX_JOIN_TIMEOUT_S  1

#define WILL_STALL (CONFIG_TEST_INPUT_LOG_RATE == 0 || CONFIG_TEST_OUTPUT_LOG_RATE == 0)

#define MODULE_NAME     test

LOG_MODULE_REGISTER(MODULE_NAME);

struct mock_log_backend {
	uint32_t dropped;
	uint32_t handled;
};

static uint32_t end_ms;
static uint32_t start_ms;
static uint32_t test_source_id;
static struct mock_log_backend mock_backend;

static inline uint32_t then(void)
{
	return start_ms;
}

static inline uint32_t now(void)
{
	/* some platforms currently _not_ starting uptime at 0!! */
	return k_uptime_get_32();
}

static inline uint32_t end(void)
{
	return end_ms;
}

static inline void create_start_end(void)
{
	start_ms = k_uptime_get_32();
	end_ms = start_ms;
	/* some "fuzz" in ms to account for odd variances */
	end_ms += MAX_STALL_TIMEOUT_S * MSEC_PER_SEC;

#if WILL_STALL
	end_ms += MAX_STALL_TIMEOUT_S * MSEC_PER_SEC;
#elif CONFIG_TEST_INPUT_LOG_RATE > 0 && CONFIG_TEST_INPUT_LOG_RATE <= CONFIG_TEST_OUTPUT_LOG_RATE
	end_ms += MSEC_PER_SEC * DIV_ROUND_UP(CONFIG_TEST_NUM_LOGS, CONFIG_TEST_INPUT_LOG_RATE);
#elif CONFIG_TEST_OUTPUT_LOG_RATE > 0 && CONFIG_TEST_INPUT_LOG_RATE > CONFIG_TEST_OUTPUT_LOG_RATE
	end_ms += MSEC_PER_SEC * DIV_ROUND_UP(CONFIG_TEST_NUM_LOGS, CONFIG_TEST_OUTPUT_LOG_RATE);
#else
#error "Impossible scenario"
#endif

	TC_PRINT("Start time: %u ms\n", start_ms);
	TC_PRINT("End   time: %u ms\n", end_ms);
}

static void handle_output(uint32_t i)
{
	while (true) {
		if (i + 1 <= (CONFIG_TEST_OUTPUT_LOG_RATE * (now() - then())) / MSEC_PER_SEC) {
			break;
		}
		k_msleep(1);
	}

	++mock_backend.handled;
}

static void handle_input(void)
{
	for (int i = 0; i < CONFIG_TEST_NUM_LOGS; i++) {
		while (true) {
			if (i + 1 <= CONFIG_TEST_INPUT_LOG_RATE * (now() - then()) / MSEC_PER_SEC) {
				break;
			}
			zassert_true(now() <= end());
			k_msleep(1);
		}

		LOG_INF("%u", i);
	}
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	size_t len;
	uint8_t *package = log_msg_get_package(&msg->log, &len);

	package += 2 * sizeof(void *);

	handle_output(*(uint32_t *)package);
}

static void mock_init(struct log_backend const *const backend)
{
}

static void panic(struct log_backend const *const backend)
{
#if WILL_STALL
	/* Don't panic! */
	return;
#endif

	zassert_true(false);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	mock_backend.dropped += cnt;
}

static const struct log_backend_api log_blocking_api = {
	.process = process,
	.panic = panic,
	.init = mock_init,
	.dropped = dropped,
};

LOG_BACKEND_DEFINE(blocking_log_backend, log_blocking_api, true, NULL);

BUILD_ASSERT(CONFIG_TEST_INPUT_LOG_RATE >= 0);
BUILD_ASSERT(CONFIG_TEST_OUTPUT_LOG_RATE >= 0);

static void print_input(void)
{
	TC_PRINT("CONFIG_TEST_NUM_LOGS: %d\n", CONFIG_TEST_NUM_LOGS);
	TC_PRINT("CONFIG_TEST_INPUT_LOG_RATE: %d\n", CONFIG_TEST_INPUT_LOG_RATE);
	TC_PRINT("CONFIG_TEST_OUTPUT_LOG_RATE: %d\n", CONFIG_TEST_OUTPUT_LOG_RATE);
}

static void print_output(void)
{
	TC_PRINT("Log backend dropped %u messages\n", mock_backend.dropped);
	TC_PRINT("Log backend handled %u messages\n", mock_backend.handled);
}

static void test_blocking_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	handle_input();
}
K_THREAD_DEFINE(test_blocking_thread, 4096, test_blocking_thread_entry, NULL, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, UINT32_MAX);

#if WILL_STALL
ZTEST_EXPECT_FAIL(log_blocking, test_blocking);
#endif
ZTEST(log_blocking, test_blocking)
{
#if WILL_STALL
	/*
	 * This is a workaround for a possible bug in the testing subsys:
	 * - comment-out ztest_test_fail() below
	 * - run with:
	 *   west build -p auto -b qemu_riscv64 -t run \
	 *     -T tests/subsys/logging/log_blocking/logging.blocking.rate.stalled
	 * - observe "Assertion failed at..."
	 * - technically, testsuite should pass. Since ZTEST_EXPECT_FAIL() is set. Never gets there.
	 * - run with:
	 *   twister -i -p qemu_riscv64 -T tests/subsys/logging/log_blocking/
	 * - observe "..FAILED : Timeout"
	 * - possible conclusions:
	 *   - test thread has not properly longjumped?
	 *   - twister not detecting assertion failures?
	 *   - twister expecting some other string and never sees it?
	 */
	ztest_test_fail();
#endif

	create_start_end();
	k_thread_start(test_blocking_thread);
	k_msleep(end() - now());

#if WILL_STALL
	k_thread_abort(test_blocking_thread);
#endif
	zassert_ok(k_thread_join(test_blocking_thread, K_SECONDS(MAX_JOIN_TIMEOUT_S)));

	print_output();

	zassert_equal(mock_backend.dropped, 0, "dropped %u / %u logs", mock_backend.dropped,
		      CONFIG_TEST_NUM_LOGS);

	zassert_equal(mock_backend.handled, CONFIG_TEST_NUM_LOGS, "handled %u / %u logs",
		      mock_backend.handled, CONFIG_TEST_NUM_LOGS);
}

static void *setup(void)
{
	/*
	 * This testsuite was added mainly to address a regression caused
	 * by this subtle, but very different interpretation.
	 */
	__ASSERT(K_TIMEOUT_EQ(K_NO_WAIT, K_MSEC(-1)), "K_NO_WAIT should be equal to K_MSEC(-1)");
	__ASSERT(!K_TIMEOUT_EQ(K_FOREVER, K_MSEC(-1)),
		 "K_FOREVER should not be equal to K_MSEC(-1)");

	test_source_id = log_source_id_get(STRINGIFY(MODULE_NAME));

	print_input();

	return NULL;
}

static void before(void *arg)
{
	memset(&mock_backend, 0, sizeof(mock_backend));
}

static void teardown(void *data)
{
	ARG_UNUSED(data);
	log_backend_disable(&blocking_log_backend);
}

ZTEST_SUITE(log_blocking, NULL, setup, before, NULL, teardown);
