/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emulated_pm_device.h"

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

struct emulated_pm_stress_data {
	const struct device *self;
	struct k_timer timer;
	struct k_sem op_done;
	atomic_t async_err;
};

static void timer_handler(struct k_timer *timer)
{
	struct emulated_pm_stress_data *data =
		CONTAINER_OF(timer, struct emulated_pm_stress_data, timer);
	int ret;

	ret = pm_device_runtime_put_async(data->self, K_NO_WAIT);
	if (ret != 0) {
		atomic_set(&data->async_err, ret);
	}
	k_sem_give(&data->op_done);
}

static int emulated_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	/* Emulate PM operation duration. */
	k_busy_wait(50);

	return 0;
}

PM_DEVICE_DEFINE(emulated_pm_stress, emulated_pm_action, 0);

static struct emulated_pm_stress_data emulated_data;

static int emulated_pm_stress_init(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	data->self = dev;
	k_sem_init(&data->op_done, 0, 1);
	atomic_clear(&data->async_err);
	k_timer_init(&data->timer, timer_handler, NULL);

	return 0;
}

DEVICE_DEFINE(emulated_pm_stress, "emulated_pm_stress", emulated_pm_stress_init,
	      PM_DEVICE_GET(emulated_pm_stress), &emulated_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

const struct device *emulated_pm_stress_dev(void)
{
	return DEVICE_GET(emulated_pm_stress);
}

int emulated_pm_stress_submit(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	atomic_clear(&data->async_err);
	k_timer_start(&data->timer, K_TICKS(1), K_NO_WAIT);

	return 0;
}

int emulated_pm_stress_wait(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	k_sem_take(&data->op_done, K_FOREVER);

	return atomic_get(&data->async_err);
}
