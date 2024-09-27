/*
 * Copyright (c) 2024 Armin Kessler
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_a4988

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(stepper_a4988, CONFIG_STEPPER_LOG_LEVEL);

#define NUM_MICRO_STEP_PINS 3

struct stepper_a4988_config {
	const struct gpio_dt_spec *dir_pin;
	const struct gpio_dt_spec *step_pin;
	const struct gpio_dt_spec *en_pin;
	const struct gpio_dt_spec *msx_pins[];
};

struct stepper_a4988_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	enum micro_step_resolution micro_step_res;
	bool has_micro_step_pins;
	bool drive_enabled;
	uint8_t step_gap;
	struct k_work_delayable stepper_dwork;
	struct k_poll_signal *async_signal;
	int32_t actual_position;
	uint32_t delay_in_us;
	int32_t step_count;
};

static int stepper_motor_make_step(const struct device *dev)
{
	const struct stepper_a4988_config *config = dev->config;
	struct stepper_a4988_data *data = dev->data;

	gpio_pin_set_dt(config->dir_pin, data->direction);
	gpio_pin_toggle_dt(config->step_pin);
	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		data->actual_position++;
	} else {
		data->actual_position--;
	}

	return 0;
}

static void update_remaining_steps(struct stepper_a4988_data *data)
{
	if (data->step_count > 0) {
		data->step_count--;
		(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
	} else if (data->step_count < 0) {
		data->step_count++;
		(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
	} else {
		if (data->async_signal) {
			(void)k_poll_signal_raise(data->async_signal,
						  STEPPER_SIGNAL_STEPS_COMPLETED);
		}
	}
}

static void update_direction_from_step_count(struct stepper_a4988_data *data)
{
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
	struct stepper_a4988_data *data = dev->data;

	if (data->step_count) {
		(void)stepper_motor_make_step(dev);
	}
	update_remaining_steps(dev->data);
}

static void velocity_mode_task(const struct device *dev)
{
	struct stepper_a4988_data *data = dev->data;

	(void)stepper_motor_make_step(dev);
	(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct stepper_a4988_data *data =
		CONTAINER_OF(dwork, struct stepper_a4988_data, stepper_dwork);

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_POSITION_MODE:
			position_mode_task(data->dev);
			break;
		case STEPPER_VELOCITY_MODE:
			velocity_mode_task(data->dev);
			break;
		default:
			LOG_WRN("Unsupported run mode %d", data->run_mode);
			break;
		}
	}
}

static int stepper_a4988_move(const struct device *dev, int32_t micro_steps,
			      struct k_poll_signal *async)
{
	struct stepper_a4988_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}

	if (!data->drive_enabled) {
		LOG_ERR("Motor is not enabled");
		return -EIO;
	}

	K_SPINLOCK(&data->lock) {
		if (data->async_signal) {
			k_poll_signal_reset(data->async_signal);
		}
		data->async_signal = async;
		data->run_mode = STEPPER_POSITION_MODE;
		data->step_count = micro_steps;
		update_direction_from_step_count(data);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int stepper_a4988_set_actual_position(const struct device *dev, int32_t position)
{
	struct stepper_a4988_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
	}
	return 0;
}

static int stepper_a4988_get_actual_position(const struct device *dev, int32_t *position)
{
	struct stepper_a4988_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int stepper_a4988_set_target_position(const struct device *dev, int32_t position,
					     struct k_poll_signal *async)
{
	struct stepper_a4988_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		if (data->async_signal) {
			k_poll_signal_reset(data->async_signal);
		}
		data->async_signal = async;
		data->run_mode = STEPPER_POSITION_MODE;
		data->step_count = position - data->actual_position;
		update_direction_from_step_count(data);
		if (data->drive_enabled) {
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		}
	}
	return 0;
}

static int stepper_a4988_is_moving(const struct device *dev, bool *is_moving)
{
	struct stepper_a4988_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_dwork);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

static int stepper_a4988_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	struct stepper_a4988_data *data = dev->data;

	if (velocity == 0) {
		LOG_ERR("Velocity cannot be zero");
		return -EINVAL;
	}

	if (velocity > USEC_PER_SEC / 2) {
		LOG_ERR("Velocity cannot be greater than %d micro_steps_per_second",
			USEC_PER_SEC / 2);
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->delay_in_us = USEC_PER_SEC / (2 * velocity);
	}
	LOG_DBG("Setting Motor Speed to %d", velocity);
	return 0;
}

static int stepper_a4988_enable_constant_velocity_mode(const struct device *dev,
						       const enum stepper_direction direction,
						       const uint32_t value)
{
	struct stepper_a4988_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_VELOCITY_MODE;
		data->direction = direction;
		if (value != 0) {
			data->delay_in_us = USEC_PER_SEC / (2 * value);
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		} else {
			(void)k_work_cancel_delayable(&data->stepper_dwork);
		}
	}
	return 0;
}

static int stepper_a4988_set_micro_step_res(const struct device *dev,
					    enum micro_step_resolution micro_step_res)
{
	struct stepper_a4988_data *data = dev->data;
	const struct stepper_a4988_config *config = dev->config;

	uint8_t msx = 0;

	if (!data->has_micro_step_pins) {
		LOG_ERR("Micro step pins not defined");
		return -ENODEV;
	}

	K_SPINLOCK(&data->lock) {
		switch (micro_step_res) {
		case STEPPER_FULL_STEP:
			msx = 0;
			break;
		case STEPPER_MICRO_STEP_2:
			msx = 1;
			break;
		case STEPPER_MICRO_STEP_4:
			msx = 2;
			break;
		case STEPPER_MICRO_STEP_8:
			msx = 3;
			break;
		case STEPPER_MICRO_STEP_16:
			msx = 7;
			break;
		default:
			LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
			return -ENOTSUP;
		}

		gpio_pin_set_dt(config->msx_pins[0], msx & 0x01);
		gpio_pin_set_dt(config->msx_pins[1], (msx & 0x02) >> 1);
		gpio_pin_set_dt(config->msx_pins[2], (msx & 0x04) >> 2);

		data->micro_step_res = micro_step_res;
	}
	return 0;
}

static int stepper_a4988_get_micro_step_res(const struct device *dev,
					    enum micro_step_resolution *micro_step_res)
{
	struct stepper_a4988_data *data = dev->data;

	*micro_step_res = data->micro_step_res;
	return 0;
}

static int stepper_a4988_enable(const struct device *dev, bool enable)
{
	const struct stepper_a4988_config *config = dev->config;
	struct stepper_a4988_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		if (config->en_pin) {
			gpio_pin_set_dt(config->en_pin, (int)!enable);
		}
		if (enable) {
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		} else {
			(void)k_work_cancel_delayable(&data->stepper_dwork);
		}
		data->drive_enabled = enable;
	}
	return 0;
}

static int stepper_a4988_motor_controller_init(const struct device *dev)
{
	struct stepper_a4988_data *data = dev->data;
	const struct stepper_a4988_config *config = dev->config;

	data->dev = dev;
	LOG_DBG("Initializing %s gpios", dev->name);
	if (!gpio_is_ready_dt(config->dir_pin) || !gpio_is_ready_dt(config->step_pin)) {
		LOG_ERR("Control pins not ready");
		return -ENODEV;
	}
	if (gpio_pin_configure_dt(config->dir_pin, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Failed to configure dir pin %d", 2);
		return -ENODEV;
	}
	if (gpio_pin_configure_dt(config->step_pin, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Failed to configure step pin");
		return -ENODEV;
	}

	if (config->en_pin) {
		if (!gpio_is_ready_dt(config->en_pin)) {
			LOG_ERR("Enable pin not ready");
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(config->en_pin, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Failed to configure en pin");
			return -ENODEV;
		}
	}

	if (data->has_micro_step_pins) {
		for (size_t i = 0; i < NUM_MICRO_STEP_PINS; i++) {
			if (!gpio_is_ready_dt(config->msx_pins[i])) {
				LOG_ERR("Micro step pins not ready");
				return -ENODEV;
			}
			if (gpio_pin_configure_dt(config->msx_pins[i], GPIO_OUTPUT_INACTIVE)) {
				LOG_ERR("Failed to configure msx pin %d", i);
				return -ENODEV;
			}
		}
		stepper_a4988_set_micro_step_res(dev, data->micro_step_res);
	}

	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);
	return 0;
}

#define STEPPER_A4988_DEVICE_DATA_DEFINE(inst)                                                     \
	static struct stepper_a4988_data stepper_a4988_data_##inst = {                             \
		.micro_step_res = DT_INST_PROP_OR(inst, micro_step_res, STEPPER_FULL_STEP),        \
		.run_mode = STEPPER_POSITION_MODE,                                                 \
		.has_micro_step_pins = DT_INST_NODE_HAS_PROP(inst, msx_gpios),                     \
		.drive_enabled = false,                                                            \
	};

#define STEPPER_A4988_DEVICE_CONFIG_DEFINE(inst)                                                   \
	static const struct gpio_dt_spec stepper_a4988_dir_pin_##inst =                            \
		GPIO_DT_SPEC_INST_GET(inst, dir_gpios);                                            \
	static const struct gpio_dt_spec stepper_a4988_step_pin_##inst =                           \
		GPIO_DT_SPEC_INST_GET(inst, step_gpios);                                           \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),                                         \
		   (static const struct gpio_dt_spec stepper_a4988_msx_pins_##inst[] = {           \
			    GPIO_DT_SPEC_INST_GET_BY_IDX(inst, msx_gpios, 0),                      \
			    GPIO_DT_SPEC_INST_GET_BY_IDX(inst, msx_gpios, 1),                      \
			    GPIO_DT_SPEC_INST_GET_BY_IDX(inst, msx_gpios, 2),                      \
		    };))                                                                           \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, en_gpios),                                          \
		   (static const struct gpio_dt_spec stepper_a4988_en_pin_##inst =                 \
			    GPIO_DT_SPEC_INST_GET(inst, en_gpios);))                               \
	static const struct stepper_a4988_config stepper_a4988_config_##inst = {                   \
		.dir_pin = &stepper_a4988_dir_pin_##inst,                                          \
		.step_pin = &stepper_a4988_step_pin_##inst,                                        \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),                                 \
			   (.msx_pins = {&stepper_a4988_msx_pins_##inst[0],                        \
					 &stepper_a4988_msx_pins_##inst[1],                        \
					 &stepper_a4988_msx_pins_##inst[2]}, ))                    \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, en_gpios),                          \
				   (.en_pin = &stepper_a4988_en_pin_##inst))};

static const struct stepper_driver_api stepper_a4988_api = {
	.enable = stepper_a4988_enable,
	.move = stepper_a4988_move,
	.is_moving = stepper_a4988_is_moving,
	.set_actual_position = stepper_a4988_set_actual_position,
	.get_actual_position = stepper_a4988_get_actual_position,
	.set_target_position = stepper_a4988_set_target_position,
	.set_max_velocity = stepper_a4988_set_max_velocity,
	.enable_constant_velocity_mode = stepper_a4988_enable_constant_velocity_mode,
	.set_micro_step_res = stepper_a4988_set_micro_step_res,
	.get_micro_step_res = stepper_a4988_get_micro_step_res};

#define STEPPER_A4988_DEVICE_DEFINE(inst)                                                          \
	DEVICE_DT_INST_DEFINE(inst, stepper_a4988_motor_controller_init, NULL,                     \
			      &stepper_a4988_data_##inst, &stepper_a4988_config_##inst,            \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY, &stepper_a4988_api);

#define STEPPER_A4988_CONTROLLER_DEFINE(inst)                                                      \
	STEPPER_A4988_DEVICE_CONFIG_DEFINE(inst)                                                   \
	STEPPER_A4988_DEVICE_DATA_DEFINE(inst)                                                     \
	STEPPER_A4988_DEVICE_DEFINE(inst)

DT_INST_FOREACH_STATUS_OKAY(STEPPER_A4988_CONTROLLER_DEFINE)
