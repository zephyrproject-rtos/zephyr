/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include "step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(step_dir_stepper);

static void step_counter_top_interrupt(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	struct step_dir_stepper_common_data *data = user_data;

	stepper_handle_timing_signal(data->dev);
}

int step_counter_timing_source_update(const struct device *dev, const uint32_t velocity)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;
	int ret;

	if (velocity == 0) {
		return -EINVAL;
	}

	data->counter_top_cfg.ticks =
		DIV_ROUND_UP(counter_us_to_ticks(config->counter, USEC_PER_SEC), velocity);

	/* Lock interrupts while modifying counter settings */
	int key = irq_lock();

	ret = counter_set_top_value(config->counter, &data->counter_top_cfg);

	irq_unlock(key);

	if (ret != 0) {
		LOG_ERR("%s: Failed to set counter top value (error: %d)", dev->name, ret);
		return ret;
	}

	return 0;
}

int step_counter_timing_source_start(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;
	int ret;

	ret = counter_start(config->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start counter: %d", ret);
		return ret;
	}

	data->counter_running = true;

	return 0;
}

int step_counter_timing_source_stop(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;
	int ret;

	ret = counter_stop(config->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to stop counter: %d", ret);
		return ret;
	}

	data->counter_running = false;

	return 0;
}

bool step_counter_timing_source_needs_reschedule(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}

bool step_counter_timing_source_is_running(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	return data->counter_running;
}

int step_counter_timing_source_init(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;

	if (!device_is_ready(config->counter)) {
		LOG_ERR("Counter device is not ready");
		return -ENODEV;
	}

	data->counter_top_cfg.callback = step_counter_top_interrupt;
	data->counter_top_cfg.user_data = data;
	data->counter_top_cfg.flags = 0;
	data->counter_top_cfg.ticks = counter_us_to_ticks(config->counter, 1000000);

	return 0;
}

const struct stepper_timing_source_api step_counter_timing_source_api = {
	.init = step_counter_timing_source_init,
	.update = step_counter_timing_source_update,
	.start = step_counter_timing_source_start,
	.needs_reschedule = step_counter_timing_source_needs_reschedule,
	.stop = step_counter_timing_source_stop,
	.is_running = step_counter_timing_source_is_running,
};
