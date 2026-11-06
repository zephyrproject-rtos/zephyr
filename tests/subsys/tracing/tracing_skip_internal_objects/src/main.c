/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <ctf_top.h>
#include <tracing_backend.h>
#include <tracing_core.h>

/* A CTF event packet is the optional timestamp, the event id, then its fields */
#define EVENT_ID_OFFSET    (IS_ENABLED(CONFIG_TRACING_CTF_TIMESTAMP) ? sizeof(uint64_t) : 0)
#define FIRST_FIELD_OFFSET (EVENT_ID_OFFSET + sizeof(uint16_t))

/* Backends are called from the tracing thread, which is how the test finds it */
static k_tid_t tracing_tid;
static atomic_t timer_starts;
static atomic_t tracing_thread_events;

void __real_tracing_format_raw_data(uint8_t *data, uint32_t length);

void __wrap_tracing_format_raw_data(uint8_t *data, uint32_t length)
{
	uint16_t id;
	uint32_t object;

	if (length >= FIRST_FIELD_OFFSET + sizeof(object)) {
		memcpy(&id, &data[EVENT_ID_OFFSET], sizeof(id));
		memcpy(&object, &data[FIRST_FIELD_OFFSET], sizeof(object));

		/* This application starts no timers, so each one is the flush timer */
		if (id == CTF_EVENT_TIMER_START) {
			atomic_inc(&timer_starts);
		}

		if ((id == CTF_EVENT_THREAD_SWITCHED_IN || id == CTF_EVENT_THREAD_SWITCHED_OUT ||
		     id == CTF_EVENT_THREAD_SCHED_READY) &&
		    tracing_tid != NULL && object == (uint32_t)(uintptr_t)tracing_tid) {
			atomic_inc(&tracing_thread_events);
		}
	}

	__real_tracing_format_raw_data(data, length);
}

static void test_backend_output(const struct tracing_backend *backend, uint8_t *data,
				uint32_t length)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(data);
	ARG_UNUSED(length);

	tracing_tid = k_current_get();
}

static const struct tracing_backend_api test_backend_api = {
	.output = test_backend_output,
};

TRACING_BACKEND_DEFINE(test_skip_internal_backend, test_backend_api);

K_SEM_DEFINE(traffic_sem, 0, 1);

/* Each burst refills the empty buffer, re-arming the flush timer and waking the tracing thread */
static void generate_bursts(int count)
{
	for (int i = 0; i < count; i++) {
		k_sem_give(&traffic_sem);
		(void)k_sem_take(&traffic_sem, K_NO_WAIT);
		k_msleep(5);
	}
}

static void *setup(void)
{
	generate_bursts(20);
	zassert_not_null(tracing_tid, "tracing thread never emitted to the backend");

	atomic_clear(&timer_starts);
	atomic_clear(&tracing_thread_events);
	generate_bursts(20);

	return NULL;
}

ZTEST(tracing_skip_internal_objects, test_flush_timer_events)
{
	if (IS_ENABLED(CONFIG_TRACING_SKIP_INTERNAL_OBJECTS)) {
		zassert_equal(atomic_get(&timer_starts), 0, "flush timer events were traced");
	} else {
		zassert_true(atomic_get(&timer_starts) > 0, "flush timer events were not traced");
	}
}

ZTEST(tracing_skip_internal_objects, test_tracing_thread_events)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_TRACING_SKIP_INTERNAL_OBJECTS);

	zassert_equal(atomic_get(&tracing_thread_events), 0, "tracing thread events were traced");
}

ZTEST(tracing_skip_internal_objects, test_other_objects)
{
	zassert_false(is_tracing_internal(k_current_get()), "test thread treated as internal");
	zassert_false(is_tracing_internal(&traffic_sem), "app semaphore treated as internal");
	zassert_false(is_tracing_internal(NULL), "NULL treated as internal");
}

ZTEST_SUITE(tracing_skip_internal_objects, NULL, setup, NULL, NULL, NULL);
