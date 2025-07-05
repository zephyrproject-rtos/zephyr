/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_h_bridge_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(h_bridge_driver, CONFIG_STEPPER_LOG_LEVEL);

#define MAX_MICRO_STEP_RES STEPPER_MICRO_STEP_2
#define NUM_CONTROL_PINS   4

static const uint8_t
	half_step_lookup_table[NUM_CONTROL_PINS * MAX_MICRO_STEP_RES][NUM_CONTROL_PINS] = {
		{1u, 1u, 0u, 0u}, {0u, 1u, 0u, 0u}, {0u, 1u, 1u, 0u}, {0u, 0u, 1u, 0u},
		{0u, 0u, 1u, 1u}, {0u, 0u, 0u, 1u}, {1u, 0u, 0u, 1u}, {1u, 0u, 0u, 0u}};

struct h_bridge_config {
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec *control_pins;
	bool invert_direction;
};

struct h_bridge_data {
	struct k_spinlock lock;
	enum stepper_direction direction;
	uint8_t step_gap;
	uint8_t coil_charge;
	void *event_cb_user_data;
};

static int stepper_motor_set_coil_charge(const struct device *dev)
{
	struct h_bridge_data *data = dev->data;
	const struct h_bridge_config *config = dev->config;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		(void)gpio_pin_set_dt(&config->control_pins[i],
				      half_step_lookup_table[data->coil_charge][i]);
	}
	return 0;
}

static void increment_coil_charge(const struct device *dev)
{
	struct h_bridge_data *data = dev->data;

	if (data->coil_charge == NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap) {
		data->coil_charge = 0;
	} else {
		data->coil_charge = data->coil_charge + data->step_gap;
	}
}

static void decrement_coil_charge(const struct device *dev)
{
	struct h_bridge_data *data = dev->data;

	if (data->coil_charge == 0) {
		data->coil_charge = NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap;
	} else {
		data->coil_charge = data->coil_charge - data->step_gap;
	}
}

static int deenergize_coils(const struct device *dev)
{
	const struct h_bridge_config *config = dev->config;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		const int err = gpio_pin_set_dt(&config->control_pins[i], 0);

		if (err != 0) {
			LOG_ERR("Failed to power down coil %d", i);
			return err;
		}
	}
	return 0;
}

static void update_coil_charge(const struct device *dev)
{
	const struct h_bridge_config *config = dev->config;
	struct h_bridge_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		config->invert_direction ? decrement_coil_charge(dev) : increment_coil_charge(dev);
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		config->invert_direction ? increment_coil_charge(dev) : decrement_coil_charge(dev);
	}
}

static int h_bridge_set_micro_step_res(const struct device *dev,
				       enum stepper_micro_step_resolution micro_step_res)
{
	struct h_bridge_data *data = dev->data;
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

static int h_bridge_get_micro_step_res(const struct device *dev,
				       enum stepper_micro_step_resolution *micro_step_res)
{
	struct h_bridge_data *data = dev->data;
	*micro_step_res = MAX_MICRO_STEP_RES >> (data->step_gap - 1);
	return 0;
}

static int h_bridge_enable(const struct device *dev)
{
	const struct h_bridge_config *config = dev->config;
	struct h_bridge_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		if (config->en_pin.port != NULL) {
			err = gpio_pin_set_dt(&config->en_pin, 1);
		} else {
			LOG_DBG("No en_pin detected");
			err = -ENOTSUP;
		}
	}
	return err;
}

static int h_bridge_disable(const struct device *dev)
{
	const struct h_bridge_config *config = dev->config;
	struct h_bridge_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		(void)deenergize_coils(dev);
		if (config->en_pin.port != NULL) {
			err = gpio_pin_set_dt(&config->en_pin, 0);
		} else {
			LOG_DBG("No en_pin detected, power stages will not be turned off if "
				"stepper is in motion");
			err = -ENOTSUP;
		}
	}
	return err;
}

static int h_bridge_set_direction(const struct device *dev, const enum stepper_direction direction)
{
	struct h_bridge_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->direction = direction;
	}

	return 0;
}

static int h_bridge_step(const struct device *dev)
{
	struct h_bridge_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		stepper_motor_set_coil_charge(dev);
		update_coil_charge(dev);
	}

	return 0;
}

static int h_bridge_init(const struct device *dev)
{
	const struct h_bridge_config *config = dev->config;

	LOG_DBG("Initializing %s h_bridge with %d pin", dev->name, NUM_CONTROL_PINS);
	for (uint8_t n_pin = 0; n_pin < NUM_CONTROL_PINS; n_pin++) {
		if (!gpio_is_ready_dt(&config->control_pins[n_pin])) {
			LOG_ERR("Control pin %d is not ready", n_pin);
			return -ENODEV;
		}
		(void)gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
	}
	return 0;
}

static DEVICE_API(stepper_drv, h_bridge_api) = {
	.enable = h_bridge_enable,
	.disable = h_bridge_disable,
	.set_micro_step_res = h_bridge_set_micro_step_res,
	.get_micro_step_res = h_bridge_get_micro_step_res,
	.set_direction = h_bridge_set_direction,
	.step = h_bridge_step,
};

#define H_BRIDGE_DEFINE(inst)                                                                      \
	static const struct gpio_dt_spec h_bridge_motor_control_pins_##inst[] = {                  \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),          \
	};                                                                                         \
	BUILD_ASSERT(ARRAY_SIZE(h_bridge_motor_control_pins_##inst) == 4,                          \
		     "h_bridge stepper driver currently supports only 4 wire configuration");      \
	static const struct h_bridge_config h_bridge_config_##inst = {                             \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                           \
		.invert_direction = DT_INST_PROP(inst, invert_direction),                          \
		.control_pins = h_bridge_motor_control_pins_##inst};                               \
	static struct h_bridge_data h_bridge_data_##inst = {                                       \
		.step_gap = MAX_MICRO_STEP_RES >> (DT_INST_PROP(inst, micro_step_res) - 1),        \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, micro_step_res) <= STEPPER_MICRO_STEP_2,                   \
		     "h_bridge stepper driver supports up to 2 micro steps");                      \
	DEVICE_DT_INST_DEFINE(inst, h_bridge_init, NULL, &h_bridge_data_##inst,                    \
			      &h_bridge_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,  \
			      &h_bridge_api);

DT_INST_FOREACH_STATUS_OKAY(H_BRIDGE_DEFINE)
