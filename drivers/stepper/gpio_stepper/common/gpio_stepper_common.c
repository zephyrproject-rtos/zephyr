/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpio_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_stepper, CONFIG_STEPPER_LOG_LEVEL);

void gpio_stepper_trigger_callback(const struct device *dev, enum stepper_event event)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
	/* Dispatch to msgq instead of raising directly */
	int ret = k_msgq_put(&data->event_msgq, &event, K_NO_WAIT);

	if (ret != 0) {
		LOG_WRN("Failed to put event in msgq: %d", ret);
	}

	ret = k_work_submit(&data->event_callback_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit work item: %d", ret);
	}
#else
	LOG_WRN_ONCE("Event callback called from ISR context without ISR safe events enabled");
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
static void gpio_stepper_work_event_handler(struct k_work *work)
{
	struct gpio_stepper_common_data *data =
		CONTAINER_OF(work, struct gpio_stepper_common_data, event_callback_work);
	enum stepper_event event;
	int ret;

	ret = k_msgq_get(&data->event_msgq, &event, K_NO_WAIT);
	if (ret != 0) {
		return;
	}

	/* Run the callback */
	if (data->callback != NULL) {
		data->callback(data->dev, event, data->event_cb_user_data);
	}

	/* If there are more pending events, resubmit this work item to handle them */
	if (k_msgq_num_used_get(&data->event_msgq) > 0) {
		k_work_submit(work);
	}
}
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */

int gpio_stepper_common_init(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;
	int ret;

	if (config->timing_source->init) {
		ret = config->timing_source->init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize timing source: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS
	struct gpio_stepper_common_data *data = dev->data;

	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_event),
		    CONFIG_STEPPER_GPIO_STEPPER_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, gpio_stepper_work_event_handler);
#endif /* CONFIG_STEPPER_GPIO_STEPPER_GENERATE_ISR_SAFE_EVENTS */
	return 0;
}

void gpio_stepper_common_update_direction_from_step_count(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (atomic_get(&data->step_count) > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (atomic_get(&data->step_count) < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	} else {
		LOG_ERR("Step count is zero");
	}
}

int gpio_stepper_common_set_reference_position(const struct device *dev, const int32_t value)
{
	struct gpio_stepper_common_data *data = dev->data;

	atomic_set(&data->actual_position, value);

	return 0;
}

int gpio_stepper_common_get_actual_position(const struct device *dev, int32_t *value)
{
	struct gpio_stepper_common_data *data = dev->data;

	*value = atomic_get(&data->actual_position);

	return 0;
}

int gpio_stepper_common_move_to(const struct device *dev, const int32_t value)
{
	struct gpio_stepper_common_data *data = dev->data;
	int32_t steps_to_move;

	/* Calculate the relative movement required */
	steps_to_move = value - atomic_get(&data->actual_position);

	return stepper_move_by(dev, steps_to_move);
}

int gpio_stepper_common_is_moving(const struct device *dev, bool *is_moving)
{
	const struct gpio_stepper_common_config *config = dev->config;

	*is_moving = config->timing_source->is_running(dev);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

int gpio_stepper_common_set_event_callback(const struct device *dev,
					   stepper_event_callback_t callback, void *user_data)
{
	struct gpio_stepper_common_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}

void gpio_stepper_common_update_remaining_steps(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (atomic_get(&data->step_count) > 0) {
		atomic_dec(&data->step_count);
	} else if (atomic_get(&data->step_count) < 0) {
		atomic_inc(&data->step_count);
	}
}

void gpio_stepper_common_position_mode_task(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;
	const struct gpio_stepper_common_config *config = dev->config;

	if (config->timing_source->needs_reschedule(dev) && atomic_get(&data->step_count) != 0) {
		(void)config->timing_source->start(dev);
	} else if (atomic_get(&data->step_count) == 0) {
		config->timing_source->stop(data->dev);
		gpio_stepper_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
	}
}

void gpio_stepper_common_velocity_mode_task(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;

	if (config->timing_source->needs_reschedule(dev)) {
		(void)config->timing_source->start(dev);
	}
}
