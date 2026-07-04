/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_message.h>

extern struct k_heap _system_heap;

struct mp_bus_api_fixture {
	struct mp_bus bus;
	struct mp_element elem;
	struct sys_memory_stats mem_before;
	struct mp_bus_sync_listener listener;
};

static void *bus_suite_setup(void)
{
	static struct mp_bus_api_fixture fixture;

	return &fixture;
}

static void bus_before(void *f)
{
	struct mp_bus_api_fixture *fix = f;
	struct mp_message out;

	mp_bus_init(&fix->bus);
	memset(&fix->elem, 0, sizeof(fix->elem));
	mp_element_init(&fix->elem, 42);

	zassert_equal(mp_bus_peek(&fix->bus, &out), -ENOMSG, "bus not empty after init");

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

static void bus_after(void *f)
{
	struct mp_bus_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_bus_api, NULL, bus_suite_setup, bus_before, bus_after, NULL);

static int listener_call_count;
static enum mp_message_type listener_last_type;
static struct mp_element *listener_last_src;

static bool test_listener_cb(struct mp_message *message, void *user_data)
{
	listener_call_count++;
	listener_last_type = message->type;
	listener_last_src = message->origin;

	return true;
}

ZTEST_F(mp_bus_api, test_post_peek_pop)
{
	struct mp_element *src = &fixture->elem;
	struct mp_message msg;
	struct mp_message peeked;
	struct mp_message popped;

	MP_MESSAGE_INIT(&msg, src, MP_MESSAGE_EOS);
	zassert_ok(mp_bus_post(&fixture->bus, &msg), "mp_bus_post failed");

	zassert_ok(mp_bus_peek(&fixture->bus, &peeked), "mp_bus_peek failed");
	zassert_equal(peeked.type, MP_MESSAGE_EOS, "peeked type != EOS");
	zassert_equal(peeked.origin, src, "peeked src mismatch");

	zassert_ok(mp_bus_pop(&fixture->bus, &popped), "mp_bus_pop failed");
	zassert_equal(popped.type, peeked.type, "peek and pop type mismatch");
	zassert_equal(popped.origin, peeked.origin, "peek and pop src mismatch");

	zassert_equal(mp_bus_peek(&fixture->bus, &peeked), -ENOMSG, "bus not empty after pop");
}

ZTEST_F(mp_bus_api, test_post_multiple_fifo_order)
{
	struct mp_element *src = &fixture->elem;
	struct mp_message msg1;
	struct mp_message msg2;
	struct mp_message first;
	struct mp_message second;

	MP_MESSAGE_INIT(&msg1, src, MP_MESSAGE_EOS);
	MP_MESSAGE_INIT(&msg2, NULL, MP_MESSAGE_ERROR);

	mp_bus_post(&fixture->bus, &msg1);
	mp_bus_post(&fixture->bus, &msg2);

	zassert_ok(mp_bus_pop(&fixture->bus, &first));
	zassert_ok(mp_bus_pop(&fixture->bus, &second));

	zassert_equal(first.type, MP_MESSAGE_EOS, "first type != EOS");
	zassert_equal(first.origin, src, "first src mismatch");
	zassert_equal(second.type, MP_MESSAGE_ERROR, "second type != ERROR");
	zassert_is_null(second.origin, "second src != NULL");
}

ZTEST_F(mp_bus_api, test_sanity)
{
	struct mp_message msg;
	struct mp_message out;

	MP_MESSAGE_INIT(&msg, NULL, MP_MESSAGE_EOS);

	zassert_true(mp_bus_post(NULL, &msg) < 0, "post to NULL bus did not fail");
	zassert_true(mp_bus_post(&fixture->bus, NULL) < 0, "post NULL msg did not fail");
	zassert_true(mp_bus_peek(NULL, &out) < 0, "peek on NULL bus did not fail");
	zassert_true(mp_bus_peek(&fixture->bus, NULL) < 0, "peek with NULL out did not fail");
	zassert_true(mp_bus_pop(NULL, &out) < 0, "pop on NULL bus did not fail");
	zassert_true(mp_bus_pop(&fixture->bus, NULL) < 0, "pop with NULL out did not fail");
}

ZTEST_F(mp_bus_api, test_pop_msg_filters_by_type)
{
	struct mp_element *src = &fixture->elem;
	struct mp_message eos;
	struct mp_message err;
	struct mp_message found;

	MP_MESSAGE_INIT(&eos, src, MP_MESSAGE_EOS);
	MP_MESSAGE_INIT(&err, src, MP_MESSAGE_ERROR);

	mp_bus_post(&fixture->bus, &eos);
	mp_bus_post(&fixture->bus, &err);

	zassert_ok(mp_bus_pop_msg(&fixture->bus, MP_MESSAGE_ERROR, &found));
	zassert_equal(found.type, MP_MESSAGE_ERROR, "found type != ERROR");
	zassert_equal(found.origin, src, "found src mismatch");
}

ZTEST_F(mp_bus_api, test_flush_clears_all)
{
	struct mp_message msg1;
	struct mp_message msg2;
	struct mp_message out;

	MP_MESSAGE_INIT(&msg1, NULL, MP_MESSAGE_EOS);
	MP_MESSAGE_INIT(&msg2, NULL, MP_MESSAGE_ERROR);

	mp_bus_post(&fixture->bus, &msg1);
	mp_bus_post(&fixture->bus, &msg2);

	mp_bus_flush(&fixture->bus);

	zassert_equal(mp_bus_peek(&fixture->bus, &out), -ENOMSG, "bus not empty after flush");
}

ZTEST_F(mp_bus_api, test_sync_listener)
{
	struct mp_element *src = &fixture->elem;
	struct mp_message msg;

	listener_call_count = 0;
	listener_last_type = MP_MESSAGE_UNKNOWN;
	listener_last_src = NULL;

	fixture->listener.cb = test_listener_cb;
	fixture->listener.filter_mask = MP_MESSAGE_EOS;
	fixture->listener.user_data = NULL;

	zassert_ok(mp_bus_add_sync_listener(&fixture->bus, &fixture->listener),
		   "adding sync listener failed");

	MP_MESSAGE_INIT(&msg, src, MP_MESSAGE_EOS);
	mp_bus_post(&fixture->bus, &msg);

	zassert_equal(listener_call_count, 1, "listener call count != 1");
	zassert_equal(listener_last_type, MP_MESSAGE_EOS, "listener type != EOS");
	zassert_equal(listener_last_src, src, "listener src mismatch");
}

ZTEST_F(mp_bus_api, test_sync_listener_filters_type)
{
	struct mp_message msg;

	listener_call_count = 0;
	fixture->listener.cb = test_listener_cb;
	fixture->listener.filter_mask = MP_MESSAGE_ERROR;
	fixture->listener.user_data = NULL;

	zassert_ok(mp_bus_add_sync_listener(&fixture->bus, &fixture->listener));

	MP_MESSAGE_INIT(&msg, NULL, MP_MESSAGE_EOS);
	mp_bus_post(&fixture->bus, &msg);

	zassert_equal(listener_call_count, 0, "listener called for non-matching type");
}
