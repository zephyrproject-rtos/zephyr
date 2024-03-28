/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_driver.h"

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

struct test_driver_data {
	const struct device *self;
	struct k_timer timer;
	bool ongoing;
};

static int test_driver_action(const struct device *dev,
			      enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static void timer_expire_cb(struct k_timer *timer)
{
	struct test_driver_data *data = k_timer_user_data_get(timer);

	data->ongoing = false;
	k_timer_stop(timer);
	pm_policy_device_power_lock_put(data->self);
}

void test_driver_async_operation(const struct device *dev)
{
	struct test_driver_data *data = dev->data;

	data->ongoing = true;
	pm_policy_device_power_lock_get(dev);

	/** Lets set a timer big enough to ensure that any deep
	 *  sleep state would be suitable but constraints will
	 *  make only state0 (suspend-to-idle) will be used.
	 */
	k_timer_start(&data->timer, K_MSEC(500), K_NO_WAIT);
}

int test_driver_init(const struct device *dev)
{
	struct test_driver_data *data = dev->data;

	data->self = dev;

	k_timer_init(&data->timer, timer_expire_cb, NULL);
	k_timer_user_data_set(&data->timer, data);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), test_driver_action);

static struct test_driver_data data;

DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), test_driver_init,
	      PM_DEVICE_DT_GET(DT_NODELABEL(test_dev)), &data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
