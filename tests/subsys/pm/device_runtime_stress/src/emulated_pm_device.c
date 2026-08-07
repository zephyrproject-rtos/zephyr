/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * Copyright (c) 2026 NXP
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
	struct k_timer get_timer;
	struct k_sem get_done;
	atomic_t isr_get_ret;
	atomic_t callback_concurrency;
	atomic_t callback_max_concurrency;
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

static void get_timer_handler(struct k_timer *timer)
{
	struct emulated_pm_stress_data *data = k_timer_user_data_get(timer);

	atomic_set(&data->isr_get_ret, pm_device_runtime_get(data->self));
	k_sem_give(&data->get_done);
}

static int emulated_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct emulated_pm_stress_data *data = dev->data;
	atomic_val_t current;
	atomic_val_t maximum;

	ARG_UNUSED(action);

	current = atomic_inc(&data->callback_concurrency) + 1;
	do {
		maximum = atomic_get(&data->callback_max_concurrency);
	} while ((current > maximum) &&
		 !atomic_cas(&data->callback_max_concurrency, maximum, current));

	/* Emulate PM operation duration. */
	k_busy_wait(50);
	atomic_dec(&data->callback_concurrency);

	return 0;
}

PM_DEVICE_DEFINE(emulated_pm_stress, emulated_pm_action,
		 COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0)));

static struct emulated_pm_stress_data emulated_data;

static int emulated_pm_stress_init(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	data->self = dev;
	k_sem_init(&data->op_done, 0, 1);
	atomic_clear(&data->async_err);
	k_timer_init(&data->timer, timer_handler, NULL);
	k_sem_init(&data->get_done, 0, 1);
	atomic_clear(&data->isr_get_ret);
	k_timer_init(&data->get_timer, get_timer_handler, NULL);
	k_timer_user_data_set(&data->get_timer, data);
	atomic_clear(&data->callback_concurrency);
	atomic_clear(&data->callback_max_concurrency);

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

void emulated_pm_stress_isr_get_submit(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	k_timer_start(&data->get_timer, K_TICKS(1), K_NO_WAIT);
}

int emulated_pm_stress_isr_get_result(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	k_sem_take(&data->get_done, K_FOREVER);

	return atomic_get(&data->isr_get_ret);
}

void emulated_pm_stress_callback_max_reset(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	atomic_clear(&data->callback_concurrency);
	atomic_clear(&data->callback_max_concurrency);
}

int emulated_pm_stress_callback_max_get(const struct device *dev)
{
	struct emulated_pm_stress_data *data = dev->data;

	return atomic_get(&data->callback_max_concurrency);
}
