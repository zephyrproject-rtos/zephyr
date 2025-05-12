/**
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_stepper_control

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_control, CONFIG_STEPPER_CONTROL_LOG_LEVEL);

struct stepper_control_config {
	const struct device *stepper;
};

struct stepper_control_data {
	const struct device *dev;
	struct k_spinlock lock;
	int32_t step_count;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	int32_t actual_position;
	int32_t reference_position;
	enum stepper_control_mode mode;
	uint64_t step_interval;
	struct k_work_delayable stepper_control_dwork;
	stepper_control_event_callback_t callback;
	void *event_cb_user_data;
};

static void update_direction_from_step_count(const struct device *dev)
{
	const struct stepper_control_config *config = dev->config;
	struct stepper_control_data *data = dev->data;

	if (data->step_count > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
		stepper_set_direction(config->stepper, STEPPER_DIRECTION_POSITIVE);
	} else if (data->step_count < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
		stepper_set_direction(config->stepper, STEPPER_DIRECTION_NEGATIVE);
	} else {
		LOG_ERR("Step count is zero");
	}
}

static int z_stepper_control_move_by(const struct device *dev, int32_t micro_steps)
{
	struct stepper_control_data *data = dev->data;

	/** Todo: Introduce a stepper driver call to check if the motor is enabled, if stepper is
	 * not enabled return -ECANCELED
	 */

	if (data->step_interval == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		update_direction_from_step_count(dev);
		(void)k_work_reschedule(&data->stepper_control_dwork, K_NO_WAIT);
	}
	return 0;
}

static int z_stepper_control_move_to(const struct device *dev, int32_t micro_steps)
{
	struct stepper_control_data *data = dev->data;
	int32_t steps_to_move;

	/** Todo: Introduce a stepper driver call to check if the motor is enabled, if stepper is
	 * not enabled return -ECANCELED
	 */

	K_SPINLOCK(&data->lock) {
		steps_to_move = micro_steps - data->actual_position;
	}
	return z_stepper_control_move_by(dev, steps_to_move);
}

static int z_stepper_control_run(const struct device *dev, const enum stepper_direction direction)
{
	const struct stepper_control_config *config = dev->config;
	struct stepper_control_data *data = dev->data;

	/** Todo: Introduce a stepper driver call to check if the motor is enabled, if stepper is
	 * not enabled return -ECANCELED
	 */

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		stepper_set_direction(config->stepper, direction);
		(void)k_work_reschedule(&data->stepper_control_dwork, K_NO_WAIT);
	}
	return 0;
}

static int z_stepper_control_stop(const struct device *dev)
{
	struct stepper_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		(void)k_work_cancel_delayable(&data->stepper_control_dwork);

		if (data->callback) {
			data->callback(data->dev, STEPPER_CONTROL_EVENT_STOPPED,
				       data->event_cb_user_data);
		}
	}
	return 0;
}

static int z_stepper_control_set_reference_position(const struct device *dev, int32_t position)
{
	struct stepper_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
	}
	return 0;
}

static int z_stepper_control_get_actual_position(const struct device *dev, int32_t *position)
{
	struct stepper_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int z_stepper_control_set_step_interval(const struct device *dev,
					       uint64_t microstep_interval_ns)
{
	struct stepper_control_data *data = dev->data;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval is invalid.");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->step_interval = microstep_interval_ns;
	}
	LOG_DBG("Setting Motor step interval to %llu", microstep_interval_ns);
	return 0;
}

static int z_stepper_control_is_moving(const struct device *dev, bool *is_moving)
{
	struct stepper_control_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_control_dwork);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

static void update_remaining_steps(const struct device *dev)
{
	struct stepper_control_data *data = dev->data;

	if (data->step_count > 0) {
		data->step_count--;
	} else if (data->step_count < 0) {
		data->step_count++;
	}
}

static void update_actual_position(const struct device *dev)
{
	struct stepper_control_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		data->actual_position++;
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		data->actual_position--;
	}
}

static void position_mode_task(const struct device *dev)
{
	const struct stepper_control_config *config = dev->config;
	struct stepper_control_data *data = dev->data;

	update_remaining_steps(dev);
	stepper_step(config->stepper);
	update_actual_position(dev);
	if (data->step_count) {
		(void)k_work_reschedule(&data->stepper_control_dwork, K_NSEC(data->step_interval));
	} else {
		if (data->callback) {
			data->callback(dev, STEPPER_CONTROL_EVENT_STEPS_COMPLETED,
				       data->event_cb_user_data);
		}
		(void)k_work_cancel_delayable(&data->stepper_control_dwork);
	}
}

static void velocity_mode_task(const struct device *dev)
{
	const struct stepper_control_config *config = dev->config;
	struct stepper_control_data *data = dev->data;

	stepper_step(config->stepper);
	update_actual_position(dev);
	(void)k_work_reschedule(&data->stepper_control_dwork, K_NSEC(data->step_interval));
}

static int z_stepper_control_set_mode(const struct device *dev, enum stepper_control_mode mode)
{
	if (mode == STEPPER_CONTROL_MODE_RAMP) {
		return -ENOTSUP;
	}
	return 0;
}

static void stepper_control_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct stepper_control_data *data =
		CONTAINER_OF(dwork, struct stepper_control_data, stepper_control_dwork);

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			position_mode_task(data->dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			velocity_mode_task(data->dev);
			break;
		default:
			LOG_WRN("Unsupported run mode %d", data->run_mode);
			break;
		}
	}
}

static int z_stepper_control_set_event_callback(const struct device *dev,
						stepper_control_event_callback_t cb,
						void *user_data)
{
	struct stepper_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->callback = cb;
	}
	data->event_cb_user_data = user_data;
	return 0;
}

static int stepper_control_init(const struct device *dev)
{
	const struct stepper_control_config *config = dev->config;
	struct stepper_control_data *data = dev->data;

	data->dev = dev;
	if (!device_is_ready(config->stepper)) {
		LOG_ERR("Stepper device %s is not ready", config->stepper->name);
		return -ENODEV;
	}
	LOG_DBG("Stepper Control initialized for stepper driver %s", config->stepper->name);
	k_work_init_delayable(&data->stepper_control_dwork, stepper_control_work_step_handler);
	return 0;
}

static DEVICE_API(stepper_control, stepper_control_api) = {
	.move_to = z_stepper_control_move_to,
	.move_by = z_stepper_control_move_by,
	.run = z_stepper_control_run,
	.set_step_interval = z_stepper_control_set_step_interval,
	.get_actual_position = z_stepper_control_get_actual_position,
	.set_reference_position = z_stepper_control_set_reference_position,
	.set_mode = z_stepper_control_set_mode,
	.is_moving = z_stepper_control_is_moving,
	.stop = z_stepper_control_stop,
	.set_event_callback = z_stepper_control_set_event_callback,
};

#define STEPPER_CONTROL_DEFINE(inst)                                                               \
	static const struct stepper_control_config stepper_control_config_##inst = {               \
		.stepper = DEVICE_DT_GET(DT_INST_PHANDLE(inst, stepper)),                          \
	};                                                                                         \
	struct stepper_control_data stepper_control_data_##inst = {                                \
		.step_interval = DT_INST_PROP(inst, step_tick_ns),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, stepper_control_init, NULL, &stepper_control_data_##inst,      \
			      &stepper_control_config_##inst, POST_KERNEL,                         \
			      CONFIG_STEPPER_CONTROL_INIT_PRIORITY, &stepper_control_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_CONTROL_DEFINE)
