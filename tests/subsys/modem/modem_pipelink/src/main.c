/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_NODELABEL(test_node)
#define TEST_PIPELINK_NAME test_pipelink_0

#define TEST_EVENTS_CONNECTED_BIT    0
#define TEST_EVENTS_DISCONNECTED_BIT 1

MODEM_PIPELINK_DT_DECLARE(TEST_NODE, TEST_PIPELINK_NAME);

static struct modem_pipelink *test_pipelink =
	MODEM_PIPELINK_DT_GET(TEST_NODE, TEST_PIPELINK_NAME);

static struct modem_pipe test_pipe;
static atomic_t events;
static uint32_t test_user_data;

static void test_pipelink_callback(struct modem_pipelink *link,
				   enum modem_pipelink_event event,
				   void *user_data)
{
	zassert_equal(test_pipelink, link);
	zassert_equal((void *)(&test_user_data), user_data);

	switch (event) {
	case MODEM_PIPELINK_EVENT_CONNECTED:
		atomic_set_bit(&events, TEST_EVENTS_CONNECTED_BIT);
		break;

	case MODEM_PIPELINK_EVENT_DISCONNECTED:
		atomic_set_bit(&events, TEST_EVENTS_DISCONNECTED_BIT);
		break;

	default:
		zassert(0, "invalid event");
		break;
	}
}

static void test_reset_events(void)
{
	atomic_set(&events, 0);
}

static void *test_setup(void)
{
	modem_pipelink_init(test_pipelink, &test_pipe);
	test_reset_events();
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);

	modem_pipelink_notify_disconnected(test_pipelink);
	modem_pipelink_release(test_pipelink);
	test_reset_events();
}

ZTEST(modem_pipelink, test_connect_not_attached)
{
	zassert_false(modem_pipelink_is_connected(test_pipelink));
	modem_pipelink_notify_connected(test_pipelink);
	zassert_true(modem_pipelink_is_connected(test_pipelink));
	modem_pipelink_notify_disconnected(test_pipelink);
	zassert_false(modem_pipelink_is_connected(test_pipelink));
}

ZTEST(modem_pipelink, test_connect_attached)
{
	modem_pipelink_attach(test_pipelink, test_pipelink_callback, &test_user_data);
	modem_pipelink_notify_connected(test_pipelink);
	zassert_true(atomic_test_bit(&events, TEST_EVENTS_CONNECTED_BIT));
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_DISCONNECTED_BIT));
	test_reset_events();
	modem_pipelink_notify_connected(test_pipelink);
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_CONNECTED_BIT));
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_DISCONNECTED_BIT));
	modem_pipelink_notify_disconnected(test_pipelink);
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_CONNECTED_BIT));
	zassert_true(atomic_test_bit(&events, TEST_EVENTS_DISCONNECTED_BIT));
	test_reset_events();
	modem_pipelink_notify_disconnected(test_pipelink);
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_CONNECTED_BIT));
	zassert_false(atomic_test_bit(&events, TEST_EVENTS_DISCONNECTED_BIT));
}

ZTEST_SUITE(modem_pipelink, NULL, test_setup, test_before, NULL, NULL);
