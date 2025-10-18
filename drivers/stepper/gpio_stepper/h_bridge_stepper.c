/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_h_bridge_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>

#include <gpio_stepper_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(h_bridge_stepper, CONFIG_STEPPER_LOG_LEVEL);

#define LUT_MAX_STEP_GAP 2
#define NUM_CONTROL_PINS 4

static const uint8_t
	half_step_lookup_table[NUM_CONTROL_PINS * LUT_MAX_STEP_GAP][NUM_CONTROL_PINS] = {
	 {1u, 1u, 0u, 0u}, {0u, 1u, 0u, 0u}, {0u, 1u, 1u, 0u}, {0u, 0u, 1u, 0u},
	 {0u, 0u, 1u, 1u}, {0u, 0u, 0u, 1u}, {1u, 0u, 0u, 1u}, {1u, 0u, 0u, 0u}};

struct h_bridge_stepper_config {
	struct gpio_stepper_common_config common;
	const struct gpio_dt_spec *control_pins;
	uint8_t step_gap;
};

struct h_bridge_stepper_data {
	struct gpio_stepper_common_data common;
	uint8_t coil_charge;
};

GPIO_STEPPER_STRUCT_CHECK(struct h_bridge_stepper_config, struct h_bridge_stepper_data);

static int stepper_motor_set_coil_charge(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	const struct h_bridge_stepper_config *config = dev->config;
	int ret;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		ret = gpio_pin_set_dt(&config->control_pins[i],
				      half_step_lookup_table[data->coil_charge][i]);
		if (ret < 0) {
			LOG_ERR("Failed to set control pin %d: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

static void increment_coil_charge(const struct device *dev)
{
	const struct h_bridge_stepper_config *config = dev->config;
	struct h_bridge_stepper_data *data = dev->data;

	if (data->coil_charge == NUM_CONTROL_PINS * LUT_MAX_STEP_GAP - config->step_gap) {
		data->coil_charge = 0;
	} else {
		data->coil_charge = data->coil_charge + config->step_gap;
	}
}

static void decrement_coil_charge(const struct device *dev)
{
	const struct h_bridge_stepper_config *config = dev->config;
	struct h_bridge_stepper_data *data = dev->data;

	if (data->coil_charge == 0) {
		data->coil_charge = NUM_CONTROL_PINS * LUT_MAX_STEP_GAP - config->step_gap;
	} else {
		data->coil_charge = data->coil_charge - config->step_gap;
	}
}

static void update_coil_charge(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;
	struct gpio_stepper_common_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		config->invert_direction ? decrement_coil_charge(dev) : increment_coil_charge(dev);
		atomic_inc(&data->actual_position);
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		config->invert_direction ? increment_coil_charge(dev) : decrement_coil_charge(dev);
		atomic_dec(&data->actual_position);
	}
}

static void stepper_work_step_handler(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;
	int ret;

	ret = stepper_motor_set_coil_charge(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set coil charge: %d", ret);
		return;
	}

	update_coil_charge(dev);
	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			gpio_stepper_common_update_remaining_steps(dev);
			gpio_stepper_common_position_mode_task(data->dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			gpio_stepper_common_velocity_mode_task(data->dev);
			break;
		default:
			LOG_WRN("Unsupported run mode %d", data->run_mode);
			break;
		}
	}
}

static int h_bridge_stepper_move_by(const struct device *dev, int32_t micro_steps)
{
	const struct gpio_stepper_common_config *config = dev->config;
	struct gpio_stepper_common_data *data = dev->data;
	int ret;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	if (micro_steps == 0) {
		gpio_stepper_trigger_callback(dev, STEPPER_EVENT_STEPS_COMPLETED);
		config->timing_source->stop(dev);
		return 0;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		atomic_set(&data->step_count, micro_steps);
		gpio_stepper_common_update_direction_from_step_count(dev);
		ret = config->timing_source->update(dev, data->microstep_interval_ns);
		if (ret < 0) {
			LOG_ERR("Failed to update timing source: %d", ret);
			K_SPINLOCK_BREAK;
		}

		ret = config->timing_source->start(dev);
		if (ret < 0) {
			LOG_ERR("Failed to start timing source: %d", ret);
		}
	}

	return 0;
}

static int h_bridge_stepper_set_microstep_interval(const struct device *dev,
						   uint64_t microstep_interval_ns)
{
	const struct gpio_stepper_common_config *config = dev->config;
	struct gpio_stepper_common_data *data = dev->data;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval is invalid.");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->microstep_interval_ns = microstep_interval_ns;
		config->timing_source->update(dev, microstep_interval_ns);
	}
	LOG_DBG("Setting Motor step interval to %llu", microstep_interval_ns);

	return 0;
}

static int h_bridge_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	const struct gpio_stepper_common_config *config = dev->config;
	struct gpio_stepper_common_data *data = dev->data;
	int ret;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		ret = config->timing_source->update(dev, data->microstep_interval_ns);
		if (ret < 0) {
			LOG_ERR("Failed to update timing source: %d", ret);
			K_SPINLOCK_BREAK;
		}

		ret = config->timing_source->start(dev);
		if (ret < 0) {
			LOG_ERR("Failed to start timing source: %d", ret);
		}
	}

	return 0;
}

static int h_bridge_stepper_stop(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;
	struct gpio_stepper_common_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		err = config->timing_source->stop(dev);
		gpio_stepper_trigger_callback(dev, STEPPER_EVENT_STOPPED);
	}

	return err;
}

static int h_bridge_stepper_init(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;
	const struct h_bridge_stepper_config *config = dev->config;
	int err;

	data->dev = dev;
	LOG_DBG("Initializing %s h_bridge_stepper with %d pin", dev->name, NUM_CONTROL_PINS);
	for (uint8_t n_pin = 0; n_pin < NUM_CONTROL_PINS; n_pin++) {
		err = gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure control pin %d: %d", n_pin, err);
			return -ENODEV;
		}
	}

	return gpio_stepper_common_init(dev);
}

static DEVICE_API(stepper, h_bridge_stepper_api) = {
	.set_reference_position = gpio_stepper_common_set_reference_position,
	.get_actual_position = gpio_stepper_common_get_actual_position,
	.set_event_callback = gpio_stepper_common_set_event_callback,
	.set_microstep_interval = h_bridge_stepper_set_microstep_interval,
	.move_by = h_bridge_stepper_move_by,
	.move_to = gpio_stepper_common_move_to,
	.run = h_bridge_stepper_run,
	.stop = h_bridge_stepper_stop,
	.is_moving = gpio_stepper_common_is_moving,
};

#define H_BRIDGE_STEPPER_DEFINE(inst)                                                              \
	static const struct gpio_dt_spec h_bridge_stepper_motor_control_pins_##inst[] = {          \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),          \
	};                                                                                         \
	BUILD_ASSERT(ARRAY_SIZE(h_bridge_stepper_motor_control_pins_##inst) == 4,                  \
		     "h_bridge stepper driver currently supports only 4 wire configuration");      \
	static const struct h_bridge_stepper_config h_bridge_stepper_config_##inst = {             \
		.common = GPIO_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                           \
		.common.timing_source_cb = stepper_work_step_handler,                              \
		.step_gap = DT_INST_PROP(inst, lut_step_gap),                                      \
		.control_pins = h_bridge_stepper_motor_control_pins_##inst};                       \
	static struct h_bridge_stepper_data h_bridge_stepper_data_##inst;                          \
	DEVICE_DT_INST_DEFINE(inst, h_bridge_stepper_init, NULL, &h_bridge_stepper_data_##inst,    \
			      &h_bridge_stepper_config_##inst, POST_KERNEL,                        \
			      CONFIG_STEPPER_INIT_PRIORITY, &h_bridge_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(H_BRIDGE_STEPPER_DEFINE)
