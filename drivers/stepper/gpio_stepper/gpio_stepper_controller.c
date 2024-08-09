/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(gpio_stepper_motor_controller, CONFIG_STEPPER_LOG_LEVEL);

#define NUMBER_OF_GPIO_PINS 4

struct gpio_stepper_motor_controller_config {
	const struct gpio_dt_spec *control_pins;
};

struct gpio_stepper_motor_controller_data {
	const struct device *dev;
	struct k_spinlock lock;
	struct k_work_delayable stepper_dwork;
	int32_t actual_position;
	int32_t target_position;
	uint32_t delay_in_us;
};

static int32_t stepper_motor_move_with_4_gpios(const struct gpio_dt_spec *gpio_spec,
					       int32_t step_number)
{
	switch (step_number) {
	case 0: {
		LOG_DBG("Step 1010 %d %d %d %d", gpio_spec[0].pin, gpio_spec[1].pin,
			gpio_spec[2].pin, gpio_spec[3].pin);
		gpio_pin_configure_dt(&gpio_spec[0], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[1], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[2], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[3], GPIO_OUTPUT_INACTIVE);
		break;
	}
	case 1: {
		LOG_DBG("Step 0110");
		gpio_pin_configure_dt(&gpio_spec[0], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[1], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[2], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[3], GPIO_OUTPUT_INACTIVE);
		break;
	}
	case 2: {
		LOG_DBG("Step 0101");
		gpio_pin_configure_dt(&gpio_spec[0], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[1], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[2], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[3], GPIO_OUTPUT_ACTIVE);
		break;
	}
	case 3: {
		LOG_DBG("Step 1001");
		gpio_pin_configure_dt(&gpio_spec[0], GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&gpio_spec[1], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[2], GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&gpio_spec[3], GPIO_OUTPUT_ACTIVE);
		break;
	}
	}
	return 0;
}

void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gpio_stepper_motor_controller_data *data =
		CONTAINER_OF(dwork, struct gpio_stepper_motor_controller_data, stepper_dwork);
	const struct gpio_stepper_motor_controller_config *config = data->dev->config;

	static int32_t step_count = 1;

	K_SPINLOCK(&data->lock) {
		LOG_DBG("Moving Motor from %d to %d with delay %d ", data->actual_position,
			data->target_position, data->delay_in_us);
		if (data->actual_position != data->target_position) {
			if (data->actual_position > data->target_position) {
				stepper_motor_move_with_4_gpios(config->control_pins, step_count--);
				step_count =
					step_count < 0 ? (NUMBER_OF_GPIO_PINS - 1) : step_count;
				data->actual_position--;
			} else if (data->actual_position < data->target_position) {
				stepper_motor_move_with_4_gpios(config->control_pins, step_count++);
				step_count =
					step_count > (NUMBER_OF_GPIO_PINS - 1) ? 0 : step_count;
				data->actual_position++;
			}
			k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
		}
	}
}

static int32_t gpio_stepper_move(const struct device *dev, int32_t micro_steps)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->target_position = data->actual_position + micro_steps;
		k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int32_t gpio_stepper_set_actual_position(const struct device *dev, int32_t position)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
		k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int32_t gpio_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int32_t gpio_stepper_set_target_position(const struct device *dev, int32_t position)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->target_position = position;
		k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int32_t gpio_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_dwork);
	LOG_DBG("Is Motor Moving %d", *is_moving);
	return 0;
}

static int32_t gpio_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	if (velocity == 0) {
		LOG_ERR("Velocity cannot be zero");
		return -EINVAL;
	}
	struct gpio_stepper_motor_controller_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->delay_in_us = USEC_PER_SEC / velocity;
	}
	LOG_DBG("Setting Motor Speed to %d", velocity);
	return 0;
}

static int32_t gpio_stepper_get_micro_step_res(const struct device *dev,
					   enum micro_step_resolution *micro_step_res)
{
	ARG_UNUSED(dev);
	*micro_step_res = STEPPER_FULL_STEP;
	return 0;
}

static int32_t gpio_stepper_motor_controller_init(const struct device *dev)
{
	struct gpio_stepper_motor_controller_data *data = dev->data;

	const struct gpio_stepper_motor_controller_config *config = dev->config;

	data->dev = dev;
	LOG_DBG("Initializing %s gpio_stepper_motor_controller", dev->name);
	for (uint8_t n_pin = 0; n_pin < NUMBER_OF_GPIO_PINS; n_pin++) {
		gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
	}
	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);
	return 0;
}

static const struct stepper_api gpio_stepper_api = {
	.move = gpio_stepper_move,
	.is_moving = gpio_stepper_is_moving,
	.set_actual_position = gpio_stepper_set_actual_position,
	.get_actual_position = gpio_stepper_get_actual_position,
	.set_target_position = gpio_stepper_set_target_position,
	.set_max_velocity = gpio_stepper_set_max_velocity,
	.get_micro_step_res = gpio_stepper_get_micro_step_res};

#define GPIO_STEPPER_MOTOR_CONTROLLER_DEFINE(inst)                                                 \
	static const struct gpio_dt_spec gpio_stepper_motor_control_pins_##inst[] = {              \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),          \
	};                                                                                         \
	BUILD_ASSERT(ARRAY_SIZE(gpio_stepper_motor_control_pins_##inst) == NUMBER_OF_GPIO_PINS,    \
		     "This driver only supports 4 control pins");                                  \
	static struct gpio_stepper_motor_controller_data                                           \
		gpio_stepper_motor_controller_data_##inst;                                         \
	static const struct gpio_stepper_motor_controller_config                                   \
		gpio_stepper_motor_controller_config_##inst = {                                    \
			.control_pins = gpio_stepper_motor_control_pins_##inst};                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_stepper_motor_controller_init, NULL,                      \
			      &gpio_stepper_motor_controller_data_##inst,                          \
			      &gpio_stepper_motor_controller_config_##inst, POST_KERNEL,           \
			      CONFIG_APPLICATION_INIT_PRIORITY, &gpio_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_STEPPER_MOTOR_CONTROLLER_DEFINE)
