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
	struct step_dir_stepper_common_data *data = dev->data;
	int ret;

	switch (data->direction) {
	case STEPPER_DIRECTION_POSITIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 1);
		break;
	case STEPPER_DIRECTION_NEGATIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 0);
		break;
	default:
		LOG_ERR("Unsupported direction: %d", data->direction);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Failed to set direction: %d", ret);
		return ret;
	}

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

static void update_remaining_steps(struct step_dir_stepper_common_data *data)
{
	if (data->step_count > 0) {
		data->step_count--;
		(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
	} else if (data->step_count < 0) {
		data->step_count++;
		(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
	} else {
		if (!data->callback) {
			LOG_WRN_ONCE("No callback set");
			return;
		}
		data->callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED, data->event_cb_user_data);
	}
}

static void update_direction_from_step_count(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (data->step_count > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (data->step_count < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	} else {
		LOG_ERR("Step count is zero");
	}
}

static void position_mode_task(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (data->step_count) {
		(void)step_dir_stepper_perform_step(dev);
	}
	update_remaining_steps(dev->data);
}

static void velocity_mode_task(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	(void)step_dir_stepper_perform_step(dev);
	(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct step_dir_stepper_common_data *data =
		CONTAINER_OF(dwork, struct step_dir_stepper_common_data, stepper_dwork);

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			position_mode_task(data->dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			velocity_mode_task(data->dev);
			break;
		default:
			LOG_WRN("Unsupported run mode: %d", data->run_mode);
			break;
		}
	}
}

int step_dir_stepper_common_init(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	struct step_dir_stepper_common_data *data = dev->data;
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

	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);

	return 0;
}

int step_dir_stepper_common_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		update_direction_from_step_count(dev);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}

	return 0;
}

int step_dir_stepper_common_set_max_velocity(const struct device *dev, const uint32_t velocity)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (velocity == 0) {
		LOG_ERR("Velocity cannot be zero");
		return -EINVAL;
	}

	if (velocity > USEC_PER_SEC) {
		LOG_ERR("Velocity cannot be greater than %d micro steps per second", USEC_PER_SEC);
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->delay_in_us = USEC_PER_SEC / velocity;
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

	K_SPINLOCK(&data->lock) {
		*value = data->actual_position;
	}

	return 0;
}

int step_dir_stepper_common_move_to(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_common_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = value - data->actual_position;
		update_direction_from_step_count(dev);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}

	return 0;
}

int step_dir_stepper_common_is_moving(const struct device *dev, bool *is_moving)
{
	struct step_dir_stepper_common_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_dwork);
	return 0;
}

int step_dir_stepper_common_run(const struct device *dev, const enum stepper_direction direction,
				const uint32_t velocity)
{
	struct step_dir_stepper_common_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		if (velocity != 0) {
			data->delay_in_us = USEC_PER_SEC / velocity;
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		} else {
			(void)k_work_cancel_delayable(&data->stepper_dwork);
		}
	}

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
