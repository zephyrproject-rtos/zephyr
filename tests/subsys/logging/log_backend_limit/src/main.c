/*
 * Copyright (c) 2026 Jacob Beningo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Register the maximum number of backends that runtime filtering supports
 * (LOG_FILTERS_MAX_BACKENDS) and check that the logger initializes and that
 * every backend's filter slot is independent.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

#define NUM_BACKENDS CONFIG_TEST_NUM_BACKENDS

BUILD_ASSERT(NUM_BACKENDS == LOG_FILTERS_MAX_BACKENDS,
	     "Test must register exactly LOG_FILTERS_MAX_BACKENDS backends");

static uint32_t rx_cnt[NUM_BACKENDS];
static int16_t test_source_id;

#ifdef CONFIG_LOG_FRONTEND
extern uint32_t frontend_rx_cnt;
extern const void *frontend_test_source;
#endif

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uintptr_t idx = (uintptr_t)backend->cb->ctx;

	/* Only count this module; other modules may log during boot. */
	if (log_msg_get_source_id(&msg->log) == test_source_id) {
		rx_cnt[idx]++;
	}
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api backend_api = {
	.process = process,
	.panic = panic,
};

#define TEST_BACKEND_DEFINE(i, _)                                                                  \
	LOG_BACKEND_DEFINE(test_backend_##i, backend_api, true, (void *)(uintptr_t)i)

LISTIFY(NUM_BACKENDS, TEST_BACKEND_DEFINE, (;));

/* Level given to backend @p i in pass @p pass. Over four passes every slot
 * holds every level, including LOG_LEVEL_DBG, which uses the slot's top bit.
 */
static uint32_t backend_level(int i, int pass)
{
	return ((i + pass) % LOG_LEVEL_DBG) + 1;
}

ZTEST(log_backend_limit, test_backend_count)
{
	/* Reaching this point means the logger initialized without asserting. */
	zassert_equal(log_backend_count_get(), LOG_FILTERS_MAX_BACKENDS);

	for (int i = 0; i < log_backend_count_get(); i++) {
		zassert_true(log_backend_is_active(log_backend_get(i)), "Backend %d not active", i);
	}
}

ZTEST(log_backend_limit, test_filter_slots_independent)
{
	test_source_id = LOG_CURRENT_MODULE_ID();
#ifdef CONFIG_LOG_FRONTEND
	frontend_test_source = Z_LOG_CURRENT_DATA();
#endif

	for (int pass = 0; pass < LOG_LEVEL_DBG; pass++) {
		memset(rx_cnt, 0, sizeof(rx_cnt));

		for (int i = 0; i < NUM_BACKENDS; i++) {
			log_filter_set(log_backend_get(i), Z_LOG_LOCAL_DOMAIN_ID, test_source_id,
				       backend_level(i, pass));
		}

#ifdef CONFIG_LOG_FRONTEND
		frontend_rx_cnt = 0;
		log_frontend_filter_set(test_source_id, LOG_LEVEL_WRN);
#endif

		for (int i = 0; i < NUM_BACKENDS; i++) {
			uint32_t level = log_filter_get(log_backend_get(i), Z_LOG_LOCAL_DOMAIN_ID,
							test_source_id, true);

			zassert_equal(level, backend_level(i, pass),
				      "pass %d: backend %d level %u, expected %u", pass, i, level,
				      backend_level(i, pass));
		}

		LOG_ERR("err");
		LOG_WRN("wrn");
		LOG_INF("inf");
		LOG_DBG("dbg");

		while (log_process()) {
		}

		/* One message per level up to and including the backend's level. */
		for (int i = 0; i < NUM_BACKENDS; i++) {
			zassert_equal(rx_cnt[i], backend_level(i, pass),
				      "pass %d: backend %d got %u messages, expected %u", pass, i,
				      rx_cnt[i], backend_level(i, pass));
		}

#ifdef CONFIG_LOG_FRONTEND
		zassert_equal(frontend_rx_cnt, LOG_LEVEL_WRN,
			      "pass %d: frontend got %u messages, expected %u", pass,
			      frontend_rx_cnt, LOG_LEVEL_WRN);
#endif
	}
}

ZTEST_SUITE(log_backend_limit, NULL, NULL, NULL, NULL, NULL);
