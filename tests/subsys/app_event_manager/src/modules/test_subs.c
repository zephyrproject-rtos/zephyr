/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <order_event.h>

#include "test_config.h"


static enum test_id cur_test_id;

static int first_cnt;
static int early_cnt;
static int normal_cnt;

static bool app_event_handler_first(const struct app_event_header *aeh)
{
	if (is_test_start_event(aeh)) {
		struct test_start_event *event = cast_test_start_event(aeh);

		cur_test_id = event->test_id;

		return false;
	}

	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			first_cnt++;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");
	return false;
}

/* Create one first listener. */
APP_EVENT_LISTENER(first, app_event_handler_first);
APP_EVENT_SUBSCRIBE_FIRST(first, order_event);
APP_EVENT_SUBSCRIBE_EARLY(first, test_start_event);


static bool app_event_handler_early(const struct app_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - early before first");
			early_cnt++;
		}

		return false;
	}

	zassert_true(false, "Event unhandled");
	return false;
}

/* Create 3 early listeners. */
APP_EVENT_LISTENER(early1, app_event_handler_early);
APP_EVENT_SUBSCRIBE_EARLY(early1, order_event);

APP_EVENT_LISTENER(early2, app_event_handler_early);
APP_EVENT_SUBSCRIBE_EARLY(early2, order_event);

APP_EVENT_LISTENER(early3, app_event_handler_early);
APP_EVENT_SUBSCRIBE_EARLY(early3, order_event);


static bool app_event_handler_normal(const struct app_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - normal before first");
			zassert_equal(early_cnt, 3, "Incorrect subscriber order"
				      " - normal before early");
			normal_cnt++;
		}

		return false;
	}

	zassert_true(false, "Wrong event type received");

	return false;
}

/* Create 3 normal listeners. */
APP_EVENT_LISTENER(listener1, app_event_handler_normal);
APP_EVENT_SUBSCRIBE(listener1, order_event);

APP_EVENT_LISTENER(listener2, app_event_handler_normal);
APP_EVENT_SUBSCRIBE(listener2, order_event);

APP_EVENT_LISTENER(listener3, app_event_handler_normal);
APP_EVENT_SUBSCRIBE(listener3, order_event);

static bool app_event_handler_final(const struct app_event_header *aeh)
{
	if (is_order_event(aeh)) {
		if (cur_test_id == TEST_SUBSCRIBER_ORDER) {
			zassert_equal(first_cnt, 1, "Incorrect subscriber order"
				      " - late before first");
			zassert_equal(early_cnt, 3, "Incorrect subscriber order"
				      " - late before early");
			zassert_equal(normal_cnt, 3,
				      "Incorrect subscriber order"
				      " - late before normal");

			struct test_end_event *te = new_test_end_event();

			zassert_not_null(te, "Failed to allocate event");
			te->test_id = cur_test_id;
			APP_EVENT_SUBMIT(te);
		}

		return false;
	}

	zassert_true(false, "Wrong event type received");

	return false;
}

/* Create one final listener. */
APP_EVENT_LISTENER(final, app_event_handler_final);
APP_EVENT_SUBSCRIBE_FINAL(final, order_event);
