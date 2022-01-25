/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <multicontext_event.h>

#include "test_multicontext_config.h"

#define MODULE test_multictx
#define THREAD_STACK_SIZE 2448


static void send_event(int a, bool sleep)
{
	struct multicontext_event *ev = new_multicontext_event();

	zassert_not_null(ev, "Failed to allocate event");
	/* For every event both values should be the same -
	 * used to check if Event Manager sends proper values.
	 */
	ev->val1 = a;
	if (sleep) {
		k_sleep(K_MSEC(5));
	}
	ev->val2 = a;

	EVENT_SUBMIT(ev);
}

static void timer_handler(struct k_timer *timer_id)
{
	send_event(SOURCE_ISR, false);
}

static K_TIMER_DEFINE(test_timer, timer_handler, NULL);
static K_THREAD_STACK_DEFINE(thread_stack1, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(thread_stack2, THREAD_STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;
static enum test_id cur_test_id;

static void thread1_fn(void)
{
	send_event(SOURCE_T1, true);
}

static void thread2_fn(void)
{
	k_timer_start(&test_timer, K_MSEC(2), K_NO_WAIT);
	send_event(SOURCE_T2, true);
}

static void start_test(void)
{
	k_thread_create(&thread1, thread_stack1,
			K_THREAD_STACK_SIZEOF(thread_stack1),
			(k_thread_entry_t)thread1_fn,
			NULL, NULL, NULL,
			THREAD1_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&thread2, thread_stack2,
			K_THREAD_STACK_SIZEOF(thread_stack2),
			(k_thread_entry_t)thread2_fn,
			NULL, NULL, NULL,
			THREAD2_PRIORITY, 0, K_NO_WAIT);
}


static bool event_handler(const struct event_header *eh)
{
	if (is_test_start_event(eh)) {
		struct test_start_event *st = cast_test_start_event(eh);

		switch (st->test_id) {
		case TEST_MULTICONTEXT:
		{
			cur_test_id = st->test_id;
			start_test();

			break;
		}

		default:
			/* Ignore other test cases, check if proper test_id. */
			zassert_true(st->test_id < TEST_CNT,
				     "test_id out of range");
			break;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, test_start_event);
