/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(step_dir_stepper, CONFIG_STEPPER_LOG_LEVEL);

static inline int step_dir_stepper_perform_step(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	ret = gpio_pin_toggle_dt(&config->step_pin);
	if (ret < 0) {
		LOG_ERR("Failed to toggle step pin: %d", ret);
		return ret;
	}

	if (!config->dual_edge) {
		ret = gpio_pin_toggle_dt(&config->step_pin);
		if (ret < 0) {
			LOG_ERR("Failed to toggle step pin: %d", ret);
			return ret;
		}
	}

	return 0;
}

static inline int update_dir_pin(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;
	int ret;

	switch (data->direction) {
	case STEPPER_DIRECTION_POSITIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 1 ^ config->invert_direction);
		break;
	case STEPPER_DIRECTION_NEGATIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 0 ^ config->invert_direction);
		break;
	default:
		LOG_ERR("Unsupported direction: %d", data->direction);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Failed to set direction: %d", ret);
		return ret;
	}

	return ret;
}

void stepper_trigger_callback(const struct device *dev, enum stepper_event event)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
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
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
static void stepper_work_event_handler(struct k_work *work)
{
	struct step_dir_stepper_common_data *data =
		CONTAINER_OF(work, struct step_dir_stepper_common_data, event_callback_work);
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
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

static void update_remaining_steps(struct step_dir_stepper_common_data *data)
{
	if (atomic_get(&data->step_count) > 0) {
		atomic_dec(&data->step_count);
	} else if (atomic_get(&data->step_count) < 0) {
		atomic_inc(&data->step_count);
	}
}

static void update_direction_from_step_count(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (atomic_get(&data->step_count) > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (atomic_get(&data->step_count) < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	} else {
		LOG_ERR("Step count is zero");
	}
}

static void position_mode_task(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;
	const struct step_dir_stepper_common_config *config = dev->config;

	update_remaining_steps(dev->data);

	if (config->timing_source->needs_reschedule(dev) && atomic_get(&data->step_count) != 0) {
		(void)config->timing_source->start(dev);
	} else if (atomic_get(&data->step_count) == 0) {
		stepper_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
		config->timing_source->stop(data->dev);
	}
}

static void velocity_mode_task(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;

	if (config->timing_source->needs_reschedule(dev)) {
		(void)config->timing_source->start(dev);
	}
}

void stepper_handle_timing_signal(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	(void)step_dir_stepper_perform_step(dev);
	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		atomic_inc(&data->actual_position);
	} else {
		atomic_dec(&data->actual_position);
	}

	switch (data->run_mode) {
	case STEPPER_RUN_MODE_POSITION:
		position_mode_task(dev);
		break;
	case STEPPER_RUN_MODE_VELOCITY:
		velocity_mode_task(dev);
		break;
	default:
		LOG_WRN("Unsupported run mode: %d", data->run_mode);
		break;
	}
}

int step_dir_stepper_common_init(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->step_pin) || !gpio_is_ready_dt(&config->dir_pin)) {
		LOG_ERR("GPIO pins are not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->step_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure step pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	if (config->timing_source->init) {
		ret = config->timing_source->init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize timing source: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
	struct step_dir_stepper_common_data *data = dev->data;

	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_event),
		    CONFIG_STEPPER_STEP_DIR_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, stepper_work_event_handler);
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

	return 0;
}

int step_dir_stepper_common_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct step_dir_stepper_common_data *data = dev->data;
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	if (micro_steps == 0) {
		stepper_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
		config->timing_source->stop(dev);
		return 0;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		atomic_set(&data->step_count, micro_steps);
		update_direction_from_step_count(dev);
		ret = update_dir_pin(dev);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}
		config->timing_source->update(dev, data->microstep_interval_ns);
		config->timing_source->start(dev);
	}

	return ret;
}

int step_dir_stepper_common_set_microstep_interval(const struct device *dev,
						   const uint64_t microstep_interval_ns)
{
	struct step_dir_stepper_common_data *data = dev->data;
	const struct step_dir_stepper_common_config *config = dev->config;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval cannot be zero");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->microstep_interval_ns = microstep_interval_ns;
		config->timing_source->update(dev, microstep_interval_ns);
	}

	return 0;
}

int step_dir_stepper_common_set_reference_position(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_common_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = value;
	}

	return 0;
}

int step_dir_stepper_common_get_actual_position(const struct device *dev, int32_t *value)
{
	struct step_dir_stepper_common_data *data = dev->data;

	*value = atomic_get(&data->actual_position);

	return 0;
}

int step_dir_stepper_common_move_to(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_common_data *data = dev->data;
	int32_t steps_to_move;

	/* Calculate the relative movement required */
	steps_to_move = value - atomic_get(&data->actual_position);

	return step_dir_stepper_common_move_by(dev, steps_to_move);
}

int step_dir_stepper_common_is_moving(const struct device *dev, bool *is_moving)
{
	const struct step_dir_stepper_common_config *config = dev->config;

	*is_moving = config->timing_source->is_running(dev);
	return 0;
}

int step_dir_stepper_common_run(const struct device *dev, const enum stepper_direction direction)
{
	struct step_dir_stepper_common_data *data = dev->data;
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		ret = update_dir_pin(dev);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}
		config->timing_source->update(dev, data->microstep_interval_ns);
		config->timing_source->start(dev);
	}

	return ret;
}

int step_dir_stepper_common_stop(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	ret = config->timing_source->stop(dev);
	if (ret != 0) {
		LOG_ERR("Failed to stop timing source: %d", ret);
		return ret;
	}

	stepper_trigger_callback(dev, STEPPER_EVENT_STOPPED);
	return 0;
}

int step_dir_stepper_common_set_event_callback(const struct device *dev,
					       stepper_event_callback_t callback, void *user_data)
{
	struct step_dir_stepper_common_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}
