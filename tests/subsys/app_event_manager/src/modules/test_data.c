/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <string.h>

#include <test_events.h>
#include <data_event.h>
#include <order_event.h>

#include "test_config.h"

#define MODULE test_data

static enum test_id cur_test_id;

static bool event_handler(const struct event_header *eh)
{
	if (is_test_start_event(eh)) {
		struct test_start_event *event = cast_test_start_event(eh);

		cur_test_id = event->test_id;

		return false;
	}

	if (is_data_event(eh)) {
		if (cur_test_id == TEST_DATA) {
			struct data_event *event = cast_data_event(eh);
			static char descr[] = TEST_STRING;

			zassert_equal(event->val1, TEST_VAL1, "Wrong value");
			zassert_equal(event->val2, TEST_VAL2, "Wrong value");
			zassert_equal(event->val3, TEST_VAL3, "Wrong value");
			zassert_equal(event->val1u, TEST_VAL1U, "Wrong value");
			zassert_equal(event->val2u, TEST_VAL2U, "Wrong value");
			zassert_equal(event->val3u, TEST_VAL3U, "Wrong value");
			zassert_false(strcmp(event->descr, descr),
				      "Wrong string");

			struct test_end_event *te = new_test_end_event();

			zassert_not_null(te, "Failed to allocate event");
			te->test_id = TEST_DATA;
			EVENT_SUBMIT(te);
		}

		return false;
	}

	if (is_order_event(eh)) {
		if (cur_test_id == TEST_EVENT_ORDER) {
			static int i;
			struct order_event *event = cast_order_event(eh);

			zassert_equal(event->val, i, "Incorrent event order");
			i++;

			if (i == TEST_EVENT_ORDER_CNT) {
				struct test_end_event *te = new_test_end_event();

				zassert_not_null(te, "Failed to allocate event");
				te->test_id = TEST_EVENT_ORDER;
				EVENT_SUBMIT(te);
			}
		}

		return false;
	}

	zassert_true(false, "Event unhandled");

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, data_event);
EVENT_SUBSCRIBE(MODULE, order_event);
EVENT_SUBSCRIBE(MODULE, test_start_event);
