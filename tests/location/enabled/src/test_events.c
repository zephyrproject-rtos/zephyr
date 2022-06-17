/*
 * Copyright (c) 2022, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/location.h>
#include <ztest.h>
#include <string.h>

/***********************************************************************
 *                          Driver instances
 **********************************************************************/
extern const struct device test_driver1;
extern const struct device test_driver2;

/***********************************************************************
 *                        Handlers data
 **********************************************************************/
static union location_event event_filter1 = {
	.position   = 0,
	.bearing    = 0,
	.speed      = 1,
	.altitude   = 1,
};

static union location_event event_filter2 = {
	.position   = 1,
	.bearing    = 1,
	.speed      = 0,
	.altitude   = 0,
};

static uint8_t event_handler1_invoked_cnt;
static uint8_t event_handler2_invoked_cnt;

/***********************************************************************
 *                          Handlers
 **********************************************************************/
static void test_location_api_event_handler1(const struct location_provider *provider,
							union location_event event)
{
	/* Verify any raised event was enabled */
	zassert_true(event_filter1.value & event.value, "Handler 1 invoked mistakenly");

	/* Verify correct provider supplied */
	zassert_true(provider->dev == &test_driver1, "Incorrect location provider supplied");

	/* Increment count */
	event_handler1_invoked_cnt++;
}

static void test_location_api_event_handler2(const struct location_provider *provider,
							union location_event event)
{
	/* Verify any raised event was enabled */
	zassert_true(event_filter2.value & event.value, "Handler 2 invoked mistakenly");

	/* Verify correct provider supplied */
	zassert_true(provider->dev == &test_driver2, "Incorrect location provider supplied");

	/* Increment count */
	event_handler2_invoked_cnt++;
}

/***********************************************************************
 *                        Supervisor mode tests
 **********************************************************************/
void test_location_api_event_handler_register(void)
{
	/* Try to register event handlers 1 and 2 */
	zassert_true(location_event_handler_register(test_location_api_event_handler1,
		event_filter1) == 0, "Failed to register location event handler 1");
	zassert_true(location_event_handler_register(test_location_api_event_handler2,
		event_filter2) == 0, "Failed to register location event handler 1");

	/* Try to register event handler 2 despite only having allocated two event handlers max */
	zassert_true(location_event_handler_register(test_location_api_event_handler2,
		event_filter2) == -ENOMEM, "Memory overflow while registering event handler");
}

void test_location_api_raise_event(void)
{
	union location_event event11 = {
		.position   = 0,
		.bearing    = 0,
		.speed      = 0,
		.altitude   = 1,
	};

	union location_event event12 = {
		.position   = 0,
		.bearing    = 0,
		.speed      = 1,
		.altitude   = 0,
	};

	union location_event event13 = {
		.position   = 0,
		.bearing    = 0,
		.speed      = 1,
		.altitude   = 1,
	};

	union location_event event21 = {
		.position   = 1,
		.bearing    = 0,
		.speed      = 0,
		.altitude   = 0,
	};

	union location_event event22 = {
		.position   = 0,
		.bearing    = 1,
		.speed      = 0,
		.altitude   = 0,
	};

	union location_event event23 = {
		.position   = 1,
		.bearing    = 1,
		.speed      = 0,
		.altitude   = 0,
	};

	union location_event event24 = {
		.position   = 1,
		.bearing    = 1,
		.speed      = 0,
		.altitude   = 0,
	};

	/* Send event from driver1 */
	zassert_true(location_provider_raise_event(&test_driver1, event11) == 0,
		"Failed to raise event 11");
	zassert_true(location_provider_raise_event(&test_driver1, event12) == 0,
		"Failed to raise event 12");
	zassert_true(location_provider_raise_event(&test_driver1, event13) == 0,
		"Failed to raise event 13");

	/* Send event from driver2 */
	zassert_true(location_provider_raise_event(&test_driver2, event21) == 0,
		"Failed to raise event 21");
	zassert_true(location_provider_raise_event(&test_driver2, event22) == 0,
		"Failed to raise event 22");
	zassert_true(location_provider_raise_event(&test_driver2, event23) == 0,
		"Failed to raise event 23");
	zassert_true(location_provider_raise_event(&test_driver2, event24) == 0,
		"Failed to raise event 24");

	/* Ensure correct number of event received */
	zassert_true(event_handler1_invoked_cnt == 3,
		"Incorrect number of invocations if handler1");
	zassert_true(event_handler2_invoked_cnt == 4,
		"Incorrect number of invocations if handler2");
}
