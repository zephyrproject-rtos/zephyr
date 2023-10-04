/*
 * Copyright 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/atomic.h>

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
static atomic_t callback_called_counter;
static atomic_t callback_test_user_data_address;
static uint32_t test_user_data = 0x1234;

static void test_rtc_update_callback_handler(const struct device *dev, void *user_data)
{
	atomic_inc(&callback_called_counter);

	atomic_set(&callback_test_user_data_address, (uint32_t)user_data);
}

ZTEST(rtc_api, test_update_callback)
{
	int ret;
	uint32_t counter;
	uint32_t address;

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	zassert_true(ret == 0, "Failed to clear and disable update callback");

	atomic_set(&callback_called_counter, 0);

	k_msleep(5000);

	counter = atomic_get(&callback_called_counter);

	zassert_true(counter == 0, "Update callback should not have been called");

	ret = rtc_update_set_callback(rtc, test_rtc_update_callback_handler, &test_user_data);

	zassert_true(ret == 0, "Failed to set and enable update callback");

	k_msleep(10000);

	counter = atomic_get(&callback_called_counter);

	address = atomic_get(&callback_test_user_data_address);

	zassert_true(counter < 12 && counter > 8, "Invalid update callback called counter");

	zassert_true(address == ((uint32_t)(&test_user_data)), "Incorrect user data");
}
