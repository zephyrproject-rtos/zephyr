/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *					 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>

#include <zephyr/logging/log.h>

#include "../timing_source/stepper_timing_source.h"
LOG_MODULE_DECLARE(stepper_motion_controller, CONFIG_STEPPER_LOG_LEVEL);

static void step_counter_top_interrupt(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	const struct stepper_timing_source *timing_source = user_data;

	timing_source->callback(timing_source->user_data);
}

static int step_counter_timing_source_start(const struct stepper_timing_source *timing_source,
					    const uint64_t interval_ns)
{
	const struct stepper_timing_source_counter_cfg *const cfg = timing_source->config;
	struct stepper_timing_source_counter_data *data = timing_source->data;

	data->counter_top_cfg.ticks =
		DIV_ROUND_CLOSEST(counter_get_frequency(cfg->dev) * interval_ns, NSEC_PER_SEC);

	if (data->counter_top_cfg.ticks == 0) {
		LOG_ERR("Invalid interval: %llu", interval_ns);
		return -EINVAL;
	}

	const int key = irq_lock();

	int ret = counter_set_top_value(cfg->dev, &data->counter_top_cfg);

	irq_unlock(key);

	if (ret != 0) {
		LOG_ERR("Failed to set counter top value (error: %d)", ret);
		return ret;
	}

	ret = counter_start(cfg->dev);

	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start counter: %d", ret);
	}

	return ret;
}

static int step_counter_timing_source_stop(const struct stepper_timing_source *timing_source)
{
	const struct stepper_timing_source_counter_cfg *const cfg = timing_source->config;
	struct stepper_timing_source_counter_data *data = timing_source->data;
	int ret = 0;

	ret = counter_stop(cfg->dev);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to stop counter: %d", ret);
	}

	data->counter_top_cfg.ticks = 0;
	counter_set_top_value(cfg->dev, &data->counter_top_cfg);

	return ret;
}

static int step_counter_timing_source_init(struct stepper_timing_source *timing_source)
{
	const struct stepper_timing_source_counter_cfg *const cfg = timing_source->config;
	struct stepper_timing_source_counter_data *data = timing_source->data;

	if (!device_is_ready(cfg->dev)) {
		LOG_ERR("Counter device is not ready");
		return -ENODEV;
	}

	data->counter_top_cfg.callback = step_counter_top_interrupt;
	data->counter_top_cfg.user_data = (void *)timing_source;
	data->counter_top_cfg.flags = COUNTER_TOP_CFG_RESET_WHEN_LATE | COUNTER_ALARM_CFG_ABSOLUTE;
	data->counter_top_cfg.ticks = 0;

	return 0;
}

static uint64_t
step_counter_timing_source_get_interval(const struct stepper_timing_source *timing_source)
{
	const struct stepper_timing_source_counter_cfg *const cfg = timing_source->config;
	const struct stepper_timing_source_counter_data *data = timing_source->data;

	return data->counter_top_cfg.ticks * NSEC_PER_SEC / counter_get_frequency(cfg->dev);
}

const struct stepper_timing_source_api stepper_timing_source_counter_api = {
	.init = step_counter_timing_source_init,
	.start = step_counter_timing_source_start,
	.stop = step_counter_timing_source_stop,
	.get_interval = step_counter_timing_source_get_interval,
};
