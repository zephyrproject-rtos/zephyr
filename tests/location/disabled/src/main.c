/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/location.h>

/***********************************************************************
 *                        Static test data
 **********************************************************************/
static const struct device test_driver1;
static const struct location_provider_api test_api;

/***********************************************************************
 *                    Static test event handler
 **********************************************************************/
static void test_event_handler(const struct location_provider *provider,
	union location_event event)
{
	ARG_UNUSED(provider);
	ARG_UNUSED(event);
}

/***********************************************************************
 *                        Supervisor mode tests
 **********************************************************************/
void test_location_disabled_api_event_handler_register(void)
{
	union location_event event = {
		.position = 1,
	};

	zassert_true(location_event_handler_register(test_event_handler, event) == -ENOSYS,
		"Incorrect response to event handler register while disabled");
}

void test_location_disabled_api_event_handler_unregister(void)
{
	zassert_true(location_event_handler_unregister(test_event_handler) == -ENOSYS,
		"Incorrect response to event handler unregister while disabled");
}

void test_location_disabled_api_providers_get(void)
{
	const struct location_provider *providers;

	zassert_true(location_providers_get(&providers) == -ENOSYS,
		"Incorrect response to providers get while disabled");
}

void test_location_disabled_api_register(void)
{
	zassert_ok(location_provider_register(&test_driver1, &test_api),
		"Got error while registering provider while disabled");
}

void test_location_disabled_api_raise_event(void)
{
	union location_event event = {
		.position = 1,
	};

	zassert_ok(location_provider_raise_event(&test_driver1, event),
		"Incorrect response to providers get while disabled");
}

/***********************************************************************
 *                           Usermode tests
 **********************************************************************/
void test_location_disabled_api_event_handler_register_user(void)
{
	union location_event event = {
		.position = 1,
	};

	zassert_true(location_event_handler_register(test_event_handler, event) == -ENOSYS,
		"Incorrect response to event handler register while disabled and user");
}

void test_location_disabled_api_event_handler_unregister_user(void)
{
	zassert_true(location_event_handler_unregister(test_event_handler) == -ENOSYS,
		"Incorrect response to event handler unregister while disabled and user");
}

void test_location_disabled_api_providers_get_user(void)
{
	const struct location_provider *providers;

	zassert_true(location_providers_get(&providers) == -ENOSYS,
		"Incorrect response to providers get while disabled and user");
}

/***********************************************************************
 *                         Run test suites
 **********************************************************************/
void test_main(void)
{
	ztest_test_suite(location_disabled_test_supervisor,
		ztest_unit_test(test_location_disabled_api_event_handler_register),
		ztest_unit_test(test_location_disabled_api_event_handler_unregister),
		ztest_unit_test(test_location_disabled_api_providers_get),
		ztest_unit_test(test_location_disabled_api_raise_event));

	ztest_test_suite(location_disabled_test_user,
		ztest_user_unit_test(test_location_disabled_api_event_handler_register_user),
		ztest_user_unit_test(test_location_disabled_api_event_handler_unregister_user),
		ztest_user_unit_test(test_location_disabled_api_providers_get_user));

	ztest_run_test_suite(location_disabled_test_supervisor);
	ztest_run_test_suite(location_disabled_test_user);
}
