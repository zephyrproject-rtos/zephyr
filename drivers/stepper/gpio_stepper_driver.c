/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_stepper_motor_controller, CONFIG_STEPPER_LOG_LEVEL);

#define MAX_MICRO_STEP_RES STEPPER_MICRO_STEP_2
#define NUM_CONTROL_PINS   4

static const uint8_t
	half_step_lookup_table[NUM_CONTROL_PINS * MAX_MICRO_STEP_RES][NUM_CONTROL_PINS] = {
		{1u, 1u, 0u, 0u}, {0u, 1u, 0u, 0u}, {0u, 1u, 1u, 0u}, {0u, 0u, 1u, 0u},
		{0u, 0u, 1u, 1u}, {0u, 0u, 0u, 1u}, {1u, 0u, 0u, 1u}, {1u, 0u, 0u, 0u}};

struct gpio_stepper_config {
	const struct gpio_dt_spec *control_pins;
	bool invert_direction;
};

struct gpio_stepper_data {
	struct k_spinlock lock;
	enum stepper_direction direction;
	uint8_t step_gap;
	uint8_t coil_charge;
	bool is_enabled;
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

static void increment_coil_charge(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;

	if (data->coil_charge == NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap) {
		data->coil_charge = 0;
	} else {
		data->coil_charge = data->coil_charge + data->step_gap;
	}
}

static void decrement_coil_charge(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;

	if (data->coil_charge == 0) {
		data->coil_charge = NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap;
	} else {
		data->coil_charge = data->coil_charge - data->step_gap;
	}
}

static int energize_coils(const struct device *dev, const bool energized)
{
	const struct gpio_stepper_config *config = dev->config;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		const int err = gpio_pin_set_dt(&config->control_pins[i], energized);

		if (err != 0) {
			LOG_ERR("Failed to power down coil %d", i);
			return err;
		}
	}
	return 0;
}

static void update_coil_charge(const struct device *dev)
{
	const struct gpio_stepper_config *config = dev->config;
	struct gpio_stepper_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		config->invert_direction ? decrement_coil_charge(dev) : increment_coil_charge(dev);
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		config->invert_direction ? increment_coil_charge(dev) : decrement_coil_charge(dev);
	}
}

static int gpio_stepper_set_micro_step_res(const struct device *dev,
					   enum stepper_micro_step_resolution micro_step_res)
{
	struct gpio_stepper_data *data = dev->data;
	int err = 0;

	K_SPINLOCK(&data->lock) {
		switch (micro_step_res) {
		case STEPPER_MICRO_STEP_1:
		case STEPPER_MICRO_STEP_2:
			data->step_gap = MAX_MICRO_STEP_RES >> (micro_step_res - 1);
			break;
		default:
			LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
			err = -ENOTSUP;
		}
	}
	return err;
}

static int gpio_stepper_get_micro_step_res(const struct device *dev,
					   enum stepper_micro_step_resolution *micro_step_res)
{
	struct gpio_stepper_data *data = dev->data;
	*micro_step_res = MAX_MICRO_STEP_RES >> (data->step_gap - 1);
	return 0;
}

static int gpio_stepper_enable(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;
	int err;

	if (data->is_enabled) {
		LOG_WRN("Stepper motor is already enabled");
		return 0;
	}

	K_SPINLOCK(&data->lock) {
		err = energize_coils(dev, true);
		if (err == 0) {
			data->is_enabled = true;
		}
	}
	return err;
}

static int gpio_stepper_disable(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		err = energize_coils(dev, false);
		if (err == 0) {
			data->is_enabled = false;
		}
	}
	return err;
}

static int gpio_stepper_set_direction(const struct device *dev,
				      const enum stepper_direction direction)
{
	struct gpio_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->direction = direction;
	}

	return 0;
}

static int gpio_stepper_step(const struct device *dev)
{
	struct gpio_stepper_data *data = dev->data;

	if (!data->is_enabled) {
		LOG_ERR("Stepper motor is not enabled");
		return -ECANCELED;
	}

	K_SPINLOCK(&data->lock) {
		if (data->direction == STEPPER_DIRECTION_POSITIVE) {
			increment_coil_charge(dev);
		} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
			decrement_coil_charge(dev);
		}
		stepper_motor_set_coil_charge(dev);
		update_coil_charge(dev);
	}

	return 0;
}

static int gpio_stepper_init(const struct device *dev)
{
	const struct gpio_stepper_config *config = dev->config;

	LOG_DBG("Initializing %s gpio_stepper with %d pin", dev->name, NUM_CONTROL_PINS);
	for (uint8_t n_pin = 0; n_pin < NUM_CONTROL_PINS; n_pin++) {
		(void)gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
	}
	return 0;
}

static DEVICE_API(stepper, gpio_stepper_api) = {
	.enable = gpio_stepper_enable,
	.disable = gpio_stepper_disable,
	.set_micro_step_res = gpio_stepper_set_micro_step_res,
	.get_micro_step_res = gpio_stepper_get_micro_step_res,
	.set_direction = gpio_stepper_set_direction,
	.step = gpio_stepper_step,
};

#define GPIO_STEPPER_DEFINE(inst)                                                                  \
	static const struct gpio_dt_spec gpio_stepper_motor_control_pins_##inst[] = {              \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),          \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(gpio_stepper_motor_control_pins_##inst) == 4,                           \
		"gpio_stepper_controller driver currently supports only 4 wire configuration");    \
	static const struct gpio_stepper_config gpio_stepper_config_##inst = {                     \
		.invert_direction = DT_INST_PROP(inst, invert_direction),                          \
		.control_pins = gpio_stepper_motor_control_pins_##inst};                           \
	static struct gpio_stepper_data gpio_stepper_data_##inst = {                               \
		.step_gap = MAX_MICRO_STEP_RES >> (DT_INST_PROP(inst, micro_step_res) - 1),        \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, micro_step_res) <= STEPPER_MICRO_STEP_2,                   \
		     "gpio_stepper_controller driver supports up to 2 micro steps");               \
	DEVICE_DT_INST_DEFINE(inst, gpio_stepper_init, NULL, &gpio_stepper_data_##inst,            \
			      &gpio_stepper_config_##inst, POST_KERNEL,                            \
			      CONFIG_STEPPER_INIT_PRIORITY, &gpio_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_STEPPER_DEFINE)
