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
static uint32_t callback_called_counter;
static void *callback_test_user_data_address;
static uint32_t test_user_data = 0x1234;
static struct k_spinlock lock;

static void test_rtc_update_callback_handler(const struct device *dev, void *user_data)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	callback_called_counter++;
	callback_test_user_data_address = user_data;

	k_spin_unlock(&lock, key);
}

ZTEST(rtc_api, test_update_callback)
{
	int ret;
	k_spinlock_key_t key;
	uint32_t counter;
	void *address;

	ret = rtc_update_set_callback(rtc, NULL, NULL);

	zassert_ok(ret, "Failed to clear and disable update callback");

	key = k_spin_lock(&lock);

	callback_called_counter = 0;
	address = callback_test_user_data_address;

	k_spin_unlock(&lock, key);

	k_msleep(5000);

	key = k_spin_lock(&lock);

	counter = callback_called_counter;

	k_spin_unlock(&lock, key);

	zassert_equal(counter, 0, "Update callback should not have been called");

	ret = rtc_update_set_callback(rtc, test_rtc_update_callback_handler, &test_user_data);

	zassert_ok(ret, "Failed to set and enable update callback");

	k_msleep(10000);

	key = k_spin_lock(&lock);

	counter = callback_called_counter;
	address = callback_test_user_data_address;

	k_spin_unlock(&lock, key);

	zassert_true(counter < 12 && counter > 8, "Invalid update callback called counter");

	zassert_equal(address, ((void *)&test_user_data), "Incorrect user data");
}
