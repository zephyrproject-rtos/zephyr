/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_steppers

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(gpio_stepper_motor_controller, CONFIG_STEPPER_LOG_LEVEL);

#define MAX_MICRO_STEP_RES STEPPER_MICRO_STEP_2
#define NUM_CONTROL_PINS   4

static const uint8_t
	half_step_lookup_table[NUM_CONTROL_PINS * MAX_MICRO_STEP_RES][NUM_CONTROL_PINS] = {
		{1u, 1u, 0u, 0u}, {0u, 1u, 0u, 0u}, {0u, 1u, 1u, 0u}, {0u, 0u, 1u, 0u},
		{0u, 0u, 1u, 1u}, {0u, 0u, 0u, 1u}, {1u, 0u, 0u, 1u}, {1u, 0u, 0u, 0u}};

struct gpio_stepper_config {
	const struct gpio_dt_spec *control_pins;
};

struct gpio_stepper_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	uint8_t step_gap;
	uint8_t coil_charge;
	struct k_work_delayable stepper_dwork;
	stepper_event_callback_t callback;
	int32_t actual_position;
	uint32_t delay_in_us;
	int32_t step_count;
	void *event_cb_user_data;
};

static int stepper_motor_set_coil_charge(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;
	const struct gpio_stepper_config *config = dev->config;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		(void)gpio_pin_set_dt(&config->control_pins[i],
				      half_step_lookup_table[data->coil_charge][i]);
	}
	return 0;
}

static void update_coil_charge(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		data->coil_charge = data->coil_charge == 0
					    ? NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap
					    : data->coil_charge - data->step_gap;
		data->actual_position++;
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		data->coil_charge =
			data->coil_charge == NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap
				? 0
				: data->coil_charge + data->step_gap;
		data->actual_position--;
	}
}

static void update_remaining_steps(struct gpio_stepper_data *data)
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
		data->callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
	}
}

static void update_direction_from_step_count(struct gpio_stepper_data *data)
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
	struct gpio_stepper_data *data = dev->data;

	if (data->step_count) {
		(void)stepper_motor_set_coil_charge(dev);
		update_coil_charge(dev);
	}
	update_remaining_steps(dev->data);
}

static void velocity_mode_task(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;

	(void)stepper_motor_set_coil_charge(dev);
	update_coil_charge(dev);
	(void)k_work_reschedule(&data->stepper_dwork, K_USEC(data->delay_in_us));
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gpio_stepper_data *data =
		CONTAINER_OF(dwork, struct gpio_stepper_data, stepper_dwork);

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

static int gpio_stepper_move(const struct device *dev, int32_t micro_steps)
{
	struct gpio_stepper_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}
	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		update_direction_from_step_count(data);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int gpio_stepper_set_actual_position(const struct device *dev, int32_t position)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
	}
	return 0;
}

static int gpio_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int gpio_stepper_set_target_position(const struct device *dev, int32_t position)
{
	struct gpio_stepper_data *data = dev->data;

	if (data->delay_in_us == 0) {
		LOG_ERR("Velocity not set or invalid velocity set");
		return -EINVAL;
	}
	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = position - data->actual_position;
		update_direction_from_step_count(data);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int gpio_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	struct gpio_stepper_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_dwork);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

static int gpio_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	struct gpio_stepper_data *data = dev->data;

	if (velocity == 0) {
		LOG_ERR("Velocity cannot be zero");
		return -EINVAL;
	}

	if (velocity > USEC_PER_SEC) {
		LOG_ERR("Velocity cannot be greater than %d micro_steps_per_second", USEC_PER_SEC);
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->delay_in_us = USEC_PER_SEC / velocity;
	}
	LOG_DBG("Setting Motor Speed to %d", velocity);
	return 0;
}

static int gpio_stepper_enable_constant_velocity_mode(const struct device *dev,
						      const enum stepper_direction direction,
						      const uint32_t value)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		if (value != 0) {
			data->delay_in_us = USEC_PER_SEC / value;
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		} else {
			(void)k_work_cancel_delayable(&data->stepper_dwork);
		}
	}
	return 0;
}

static int gpio_stepper_set_micro_step_res(const struct device *dev,
					   enum stepper_micro_step_resolution micro_step_res)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		switch (micro_step_res) {
		case STEPPER_MICRO_STEP_1:
		case STEPPER_MICRO_STEP_2:
			data->step_gap = MAX_MICRO_STEP_RES >> (micro_step_res - 1);
			break;
		default:
			LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
			return -ENOTSUP;
		}
	}
	return 0;
}

static int gpio_stepper_get_micro_step_res(const struct device *dev,
					   enum stepper_micro_step_resolution *micro_step_res)
{
	struct gpio_stepper_data *data = dev->data;
	*micro_step_res = MAX_MICRO_STEP_RES >> (data->step_gap - 1);
	return 0;
}

static int gpio_stepper_set_event_callback(const struct device *dev,
					   stepper_event_callback_t callback, void *user_data)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->callback = callback;
	}
	data->event_cb_user_data = user_data;
	return 0;
}

static int gpio_stepper_enable(const struct device *dev, bool enable)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		if (enable) {
			(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
		} else {
			(void)k_work_cancel_delayable(&data->stepper_dwork);
		}
	}
	return 0;
}

static int gpio_stepper_motor_controller_init(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;
	const struct gpio_stepper_config *config = dev->config;

	data->dev = dev;
	LOG_DBG("Initializing %s gpio_stepper_motor_controller with %d pin", dev->name,
		NUM_CONTROL_PINS);
	for (uint8_t n_pin = 0; n_pin < NUM_CONTROL_PINS; n_pin++) {
		(void)gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
	}
	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);
	return 0;
}

#define GPIO_STEPPER_DEVICE_DATA_DEFINE(child)                                                     \
	static struct gpio_stepper_data gpio_stepper_data_##child = {                              \
		.step_gap = MAX_MICRO_STEP_RES >> (DT_PROP(child, micro_step_res) - 1),            \
	};                                                                                         \
	BUILD_ASSERT(DT_PROP(child, micro_step_res) <= STEPPER_MICRO_STEP_2,                       \
		     "gpio_stepper_controller driver supports up to 2 micro steps");

#define GPIO_STEPPER_DEVICE_CONFIG_DEFINE(child)                                                   \
	static const struct gpio_dt_spec gpio_stepper_motor_control_pins_##child[] = {             \
		DT_FOREACH_PROP_ELEM_SEP(child, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),              \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(gpio_stepper_motor_control_pins_##child) == 4,                          \
		"gpio_stepper_controller driver currently supports only 4 wire configuration");    \
	static const struct gpio_stepper_config gpio_stepper_config_##child = {                    \
		.control_pins = gpio_stepper_motor_control_pins_##child};

#define GPIO_STEPPER_API_DEFINE(child)                                                             \
	static const struct stepper_driver_api gpio_stepper_api_##child = {                        \
		.enable = gpio_stepper_enable,                                                     \
		.move = gpio_stepper_move,                                                         \
		.is_moving = gpio_stepper_is_moving,                                               \
		.set_actual_position = gpio_stepper_set_actual_position,                           \
		.get_actual_position = gpio_stepper_get_actual_position,                           \
		.set_target_position = gpio_stepper_set_target_position,                           \
		.set_max_velocity = gpio_stepper_set_max_velocity,                                 \
		.enable_constant_velocity_mode = gpio_stepper_enable_constant_velocity_mode,       \
		.set_micro_step_res = gpio_stepper_set_micro_step_res,                             \
		.get_micro_step_res = gpio_stepper_get_micro_step_res,                             \
		.set_event_callback = gpio_stepper_set_event_callback, };

#define GPIO_STEPPER_DEVICE_DEFINE(child)                                                          \
	DEVICE_DT_DEFINE(child, gpio_stepper_motor_controller_init, NULL,                          \
			 &gpio_stepper_data_##child, &gpio_stepper_config_##child, POST_KERNEL,    \
			 CONFIG_STEPPER_INIT_PRIORITY, &gpio_stepper_api_##child);

#define GPIO_STEPPER_CONTROLLER_DEFINE(inst)                                                       \
	DT_INST_FOREACH_CHILD(inst, GPIO_STEPPER_DEVICE_CONFIG_DEFINE);                            \
	DT_INST_FOREACH_CHILD(inst, GPIO_STEPPER_DEVICE_DATA_DEFINE);                              \
	DT_INST_FOREACH_CHILD(inst, GPIO_STEPPER_API_DEFINE);                                      \
	DT_INST_FOREACH_CHILD(inst, GPIO_STEPPER_DEVICE_DEFINE);

DT_INST_FOREACH_STATUS_OKAY(GPIO_STEPPER_CONTROLLER_DEFINE)
