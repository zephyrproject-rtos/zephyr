/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/ztress.h>
#include <zephyr/random/rand32.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>

#define MODULE_NAME test

LOG_MODULE_REGISTER(MODULE_NAME);

static uint32_t test_source_id;

#define CNT_BITS 28

struct mock_log_backend {
	uint32_t last_id[16];
	uint32_t cnt[16];
	uint32_t dropped;
	uint32_t missing;
};

static struct mock_log_backend mock_backend;
static uint32_t log_process_delay = 10;

static void handle_msg(uint32_t arg0)
{
	uint32_t ctx_id = arg0 >> CNT_BITS;
	uint32_t id = arg0 & BIT_MASK(CNT_BITS);
	int off = id - mock_backend.last_id[ctx_id] - 1;

	mock_backend.cnt[ctx_id]++;

	k_busy_wait(log_process_delay);

	if (off > 0) {
		mock_backend.missing += off;
	}

	mock_backend.last_id[ctx_id] = id;
}

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	size_t len;
	uint8_t *package = log_msg_get_package(&msg->log, &len);

	package += 2 * sizeof(void *);

	handle_msg(*(uint32_t *)package);
}

static void mock_init(struct log_backend const *const backend)
{

}

static void panic(struct log_backend const *const backend)
{
	zassert_true(false);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{

	mock_backend.dropped += cnt;
}

static const struct log_backend_api log_backend_api = {
	.process = process,
	.panic = panic,
	.init = mock_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? dropped : NULL,
};

LOG_BACKEND_DEFINE(test, log_backend_api, true, NULL);

static void validate(int ctx_cnt)
{
	uint64_t in_cnt = 0;
	uint64_t out_cnt = 0;

	for (int i = 0; i < ctx_cnt; i++) {
		/* First exectution skips (-1) but there is one final round of logging
		 * when ztress execution is completed (+1).
		 */
		in_cnt += ztress_exec_count(i) - 1 + 1;
		out_cnt += mock_backend.cnt[i];
	}

	out_cnt += mock_backend.dropped;

	zassert_equal(mock_backend.dropped, mock_backend.missing,
			"dropped:%u missing:%u",
			mock_backend.dropped, mock_backend.missing);
	zassert_equal(in_cnt, out_cnt);
}

static bool context_handler(void *user_data, uint32_t cnt, bool last, int prio)
{
	/* Skip first execution to start from id = 1. That simplifies handling in
	 * the backend and validation of message.
	 */
	if (cnt == 0) {
		return true;
	}

	uint32_t i = cnt | (prio << CNT_BITS);

	switch (sys_rand32_get() % 4) {
	case 0:
		LOG_INF("%u", i);
		break;
	case 1:
		LOG_INF("%u %u %u %u", i, 1, 2, 3);
		break;
	case 2:
	{
		char test_str[] = "test string";
		LOG_INF("%u %s %s", i, "test", test_str);
		break;
	}
	default:
		LOG_INF("%u %d", i, 100);
		break;
	}

	return true;
}

static int get_test_timeout(void)
{
	if (IS_ENABLED(CONFIG_BOARD_QEMU_CORTEX_A9)) {
		/* Emulation for that target is extremely slow. */
		return 500;
	}

	if (IS_ENABLED(CONFIG_BOARD_QEMU_X86)) {
		/* Emulation for that target is very fast. */
		return 10000;
	}

	if (IS_ENABLED(CONFIG_BOARD_QEMU_X86_64)) {
		/* Emulation for that target is very fast. */
		return 10000;
	}

	return 5000;
}

static void test_stress(uint32_t delay)
{
	uint32_t preempt = 2000;
	uint32_t ctx_cnt = 3;

	memset(&mock_backend, 0, sizeof(mock_backend));
	log_process_delay = delay;
	ztress_set_timeout(K_MSEC(get_test_timeout()));
	ZTRESS_EXECUTE(
		       ZTRESS_TIMER(context_handler, NULL, 0, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(context_handler, NULL, 0, preempt, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(context_handler, NULL, 0, preempt, Z_TIMEOUT_TICKS(30)));

	while (log_data_pending()) {
		k_msleep(200);
	}

	/* Log last messages, they should not be dropped as all messages are processed earlier. */
	for (int i = 0; i < ctx_cnt; i++) {
		uint32_t data = (i << CNT_BITS) | ztress_exec_count(i);

		LOG_INF("%u", data);
	}

	while (log_data_pending()) {
		k_msleep(100);
	}

	k_msleep(10);

	validate(ctx_cnt);
}

ZTEST(log_stress, test_stress_fast_processing)
{
	test_stress(10);
}

ZTEST(log_stress, test_stress_slow_processing)
{
	test_stress(100);
}

static void *setup(void)
{
	test_source_id = log_source_id_get(STRINGIFY(MODULE_NAME));

	return NULL;
}

static void before(void *data)
{
	ARG_UNUSED(data);

	ztest_simple_1cpu_before(data);
}

static void after(void *data)
{
	ARG_UNUSED(data);

	ztest_simple_1cpu_after(data);
}

static void teardown(void *data)
{
	ARG_UNUSED(data);

	log_backend_disable(&test);
}

ZTEST_SUITE(log_stress, NULL, setup, before, after, teardown);
