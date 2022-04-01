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

#define MODULE test_multictx_handler
#define THREAD_STACK_SIZE 400

static enum test_id cur_test_id;

static void end_test(void)
{
	struct test_end_event *event = new_test_end_event();

	zassert_not_null(event, "Failed to allocate event");
	event->test_id = cur_test_id;

	APP_EVENT_SUBMIT(event);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *st = cast_test_start_event(aeh);

		switch (st->test_id) {
		case TEST_MULTICONTEXT:
		{
			cur_test_id = st->test_id;

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

	if (is_multicontext_event(aeh)) {
		if (cur_test_id == TEST_MULTICONTEXT) {
			static bool isr_received;
			static bool t1_received;
			static bool t2_received;

			struct multicontext_event *ev =
				cast_multicontext_event(aeh);

			zassert_equal(ev->val1, ev->val2,
				      "Invalid event data");

			zassert_true(ev->val1 < SOURCE_CNT,
				     "Invalid source ID");

			if (ev->val1 == SOURCE_T1) {
				zassert_true(isr_received,
					     "Incorrect event order");
				t1_received = true;
			} else if (ev->val1 == SOURCE_T2) {
				zassert_true(isr_received,
					     "Incorrect event order");
				t2_received = true;

			} else if (ev->val1 == SOURCE_ISR) {
				isr_received = true;
			}

			if (isr_received && t1_received && t2_received) {
				end_test();
			}
		}

		return false;
	}

	zassert_true(false, "Event unhandled");

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
APP_EVENT_SUBSCRIBE(MODULE, multicontext_event);
