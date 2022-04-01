/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <data_event.h>
#include <order_event.h>

#include "test_config.h"

#define MODULE test_basic

static bool event_handler(const struct event_header *eh)
{
	if (is_test_start_event(eh)) {
		struct test_start_event *st = cast_test_start_event(eh);

		switch (st->test_id) {
		case TEST_BASIC:
		{
			struct test_end_event *et = new_test_end_event();

			zassert_not_null(et, "Failed to allocate event");
			et->test_id = st->test_id;
			EVENT_SUBMIT(et);
			break;
		}

		case TEST_DATA:
		{
			struct data_event *event = new_data_event();
			static char descr[] = TEST_STRING;

			zassert_not_null(event, "Failed to allocate event");
			event->val1 = TEST_VAL1;
			event->val2 = TEST_VAL2;
			event->val3 = TEST_VAL3;
			event->val1u = TEST_VAL1U;
			event->val2u = TEST_VAL2U;
			event->val3u = TEST_VAL3U;

			event->descr = descr;

			EVENT_SUBMIT(event);
			break;
		}

		case TEST_EVENT_ORDER:
		{
			for (size_t i = 0; i < TEST_EVENT_ORDER_CNT; i++) {
				struct order_event *event = new_order_event();

				zassert_not_null(event, "Failed to allocate event");
				event->val = i;
				EVENT_SUBMIT(event);
			}
			break;
		}

		case TEST_SUBSCRIBER_ORDER:
		{
			struct order_event *event = new_order_event();

			zassert_not_null(event, "Failed to allocate event");
			EVENT_SUBMIT(event);
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
