/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include "step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(step_dir_stepper);

struct step_counter_update_data {
	uint64_t microstep_interval_ns;
};

static void step_counter_top_interrupt(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	struct step_counter_data *data = user_data;

	data->handler(data->dev);
}

int step_counter_timing_source_update(const struct device *dev, void *update_data)
{
	struct step_counter_data *data = dev->data;
	struct step_counter_update_data *u_data = update_data;
	int ret;

	if (u_data->microstep_interval_ns == 0) {
		return -EINVAL;
	}

	data->counter_top_cfg.ticks = DIV_ROUND_UP(
		counter_get_frequency(data->counter) * u_data->microstep_interval_ns, NSEC_PER_SEC);

	/* Lock interrupts while modifying counter settings */
	int key = irq_lock();

	ret = counter_set_top_value(data->counter, &data->counter_top_cfg);

	irq_unlock(key);

	if (ret != 0) {
		LOG_ERR("%s: Failed to set counter top value (error: %d)", dev->name, ret);
		return ret;
	}

	return 0;
}

int step_counter_timing_source_start(const struct device *dev)
{
	struct step_counter_data *data = dev->data;
	int ret;

	ret = counter_start(data->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start counter: %d", ret);
		return ret;
	}

	data->counter_running = true;

	return 0;
}

int step_counter_timing_source_stop(const struct device *dev)
{
	struct step_counter_data *data = dev->data;
	int ret;

	ret = counter_stop(data->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to stop counter: %d", ret);
		return ret;
	}

	data->counter_running = false;

	return 0;
}

bool step_counter_timing_source_is_running(const struct device *dev)
{
	struct step_counter_data *data = dev->data;

	return data->counter_running;
}

uint64_t step_counter_timing_source_get_current_interval(const struct device *dev)
{

	struct step_counter_data *data = dev->data;

	return step_counter_timing_source_is_running(dev)
		       ? (uint64_t)data->counter_top_cfg.ticks * NSEC_PER_SEC /
				 counter_get_frequency(data->counter)
		       : 0;
}

int step_counter_timing_register_handler(const struct device *dev, step_dir_step_handler handler)
{
	struct step_counter_data *data = dev->data;

	data->handler = handler;

	return 0;
}

int step_counter_timing_source_init(const struct device *dev)
{
	struct step_counter_data *data = dev->data;

	if (!device_is_ready(data->counter)) {
		LOG_ERR("Counter device is not ready");
		return -ENODEV;
	}

	data->counter_top_cfg.callback = step_counter_top_interrupt;
	data->counter_top_cfg.user_data = data;
	data->counter_top_cfg.flags = 0;
	data->counter_top_cfg.ticks = counter_us_to_ticks(data->counter, 1000000);

	return 0;
}

const struct stepper_timing_source_api step_counter_timing_source_api = {
	.init = step_counter_timing_source_init,
	.update = step_counter_timing_source_update,
	.start = step_counter_timing_source_start,
	.stop = step_counter_timing_source_stop,
	.is_running = step_counter_timing_source_is_running,
	.get_current_interval = step_counter_timing_source_get_current_interval,
	.register_step_handler = step_counter_timing_register_handler,
};
