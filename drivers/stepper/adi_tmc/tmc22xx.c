/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../motion_controller/stepper_motion_controller.h"
#include "../interface/stepper_interface_step_dir.h"

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc22xx, CONFIG_STEPPER_LOG_LEVEL);

#define MSX_PIN_COUNT       2
#define MSX_PIN_STATE_COUNT 4

struct tmc22xx_config {
	// Motion controller configuration must be first
	const struct stepper_motion_controller_config motion_controller_config;
	// Step/dir interface configuration
	const struct stepper_interface_step_dir interface_config;
	// TMC22xx specific config
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec *msx_pins;
	enum stepper_micro_step_resolution *msx_resolutions;
};

struct tmc22xx_data {
	// Motion controller data must be first
	struct stepper_motion_controller_data motion_controller_data;
	// TMC22xx specific data
	enum stepper_micro_step_resolution resolution;
	stepper_event_callback_t event_callback;
	void *event_callback_user_data;
};

static int tmc22xx_stepper_enable(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;
	
	/* Direct GPIO control for enable pin */
	return gpio_pin_set_dt(&config->en_pin, 1);
}

static int tmc22xx_stepper_disable(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;
	
	/* Direct GPIO control for enable pin */
	return gpio_pin_set_dt(&config->en_pin, 0);
}

static int tmc22xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc22xx_data *data = dev->data;

	data->event_callback = callback;
	data->event_callback_user_data = user_data;

	return 0;
}

static int tmc22xx_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *micro_step_res)
{
	const struct tmc22xx_data *data = dev->data;

	*micro_step_res = data->resolution;
	return 0;
}

static int tmc22xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution micro_step_res)
{
	struct tmc22xx_data *data = dev->data;
	const struct tmc22xx_config *config = dev->config;
	int ret;

	if (!config->msx_pins) {
		LOG_ERR("Microstep resolution pins are not configured");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < MSX_PIN_STATE_COUNT; i++) {
		if (micro_step_res != config->msx_resolutions[i]) {
			continue;
		}

		ret = gpio_pin_set_dt(&config->msx_pins[0], i & 0x01);
		if (ret < 0) {
			LOG_ERR("Failed to set MS1 pin: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->msx_pins[1], (i & 0x02) >> 1);
		if (ret < 0) {
			LOG_ERR("Failed to set MS2 pin: %d", ret);
			return ret;
		}

		data->resolution = micro_step_res;
		return 0;
	}

	LOG_ERR("Unsupported microstep resolution: %d", micro_step_res);
	return -ENOTSUP;
}

static int tmc22xx_stepper_configure_msx_pins(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;
	int ret;

	for (uint8_t i = 0; i < MSX_PIN_COUNT; i++) {
		if (!gpio_is_ready_dt(&config->msx_pins[i])) {
			LOG_ERR("MSX pin %u are not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->msx_pins[i], GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure msx pin %u", i);
			return ret;
		}
	}
	return 0;
}

static int tmc22xx_stepper_init(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;
	const struct tmc22xx_data *data = dev->data;
	int ret = 0;

	/* Initialize step/dir pins using inline interface function */
	ret = step_dir_interface_init(&config->interface_config);
	if (ret < 0) {
		LOG_ERR("Failed to init step/dir interface: %d", ret);
		return ret;
	}

	/* Initialize enable pin directly in TMC22xx driver */
	if (!gpio_is_ready_dt(&config->en_pin)) {
		LOG_ERR("Enable pin is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure enable pin: %d", ret);
		return ret;
	}

	if (config->msx_pins) {
		ret = tmc22xx_stepper_configure_msx_pins(dev);
		if (ret < 0) {
			LOG_ERR("Failed to configure MSX pins: %d", ret);
			return ret;
		}

		ret = tmc22xx_stepper_set_micro_step_res(dev, data->resolution);
		if (ret < 0) {
			LOG_ERR("Failed to set microstep resolution: %d", ret);
			return ret;
		}
	}

	ret = stepper_motion_controller_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init motion controller: %d", ret);
		return ret;
	}

	return 0;
}

// Motion controller callbacks for step/dir interface
static void tmc22xx_step_callback(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;
	/* Use inline version for maximum stepping performance */
	step_dir_interface_step(&config->interface_config);
}

static void tmc22xx_set_direction_callback(const struct device *dev,
					   enum stepper_direction direction)
{
	const struct tmc22xx_config *config = dev->config;
	/* Use inline version for maximum direction setting performance */
	step_dir_interface_set_dir(&config->interface_config, direction);
}

static void tmc22xx_event_callback(const struct device *dev, enum stepper_event event)
{
	struct tmc22xx_data *data = dev->data;

	if (data->event_callback) {
		data->event_callback(dev, event, data->event_callback_user_data);
	}
}

static const struct stepper_motion_controller_callbacks_api motion_controller_callbacks = {
	.step = tmc22xx_step_callback,
	.set_direction = tmc22xx_set_direction_callback,
	.event = tmc22xx_event_callback,
};

static DEVICE_API(stepper, tmc22xx_stepper_api) = {
	.enable = tmc22xx_stepper_enable,
	.disable = tmc22xx_stepper_disable,
	.move_by = stepper_motion_controller_move_by,
	.is_moving = stepper_motion_controller_is_moving,
	.set_reference_position = stepper_motion_controller_set_position,
	.get_actual_position = stepper_motion_controller_get_position,
	.move_to = stepper_motion_controller_move_to,
	.run = stepper_motion_controller_run,
	.stop = stepper_motion_controller_stop,
	.set_event_callback = tmc22xx_stepper_set_event_callback,
	.set_micro_step_res = tmc22xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc22xx_stepper_get_micro_step_res,
	.set_ramp = stepper_motion_controller_set_ramp,
};

#define TMC22XX_STEPPER_DEFINE(inst, msx_table)                                                      \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios), (                                       \
	static const struct gpio_dt_spec tmc22xx_stepper_msx_pins_##inst[] = {                     \
		DT_INST_FOREACH_PROP_ELEM_SEP(                                                     \
			inst, msx_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)                              \
		),                                                                                 \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(tmc22xx_stepper_msx_pins_##inst) == MSX_PIN_COUNT,                      \
		"Two microstep config pins needed");                                               \
	)) \
	STEPPER_TIMING_SOURCE_DT_INST_DEFINE(inst)                                                   \
	static const struct tmc22xx_config tmc22xx_config_##inst = {                                 \
		.motion_controller_config =                                                          \
			{                                                                            \
				.timing_source = STEPPER_TIMING_SOURCE_DT_INST_GET(inst),            \
				.callbacks = &motion_controller_callbacks,                           \
			},                                                                           \
		.interface_config =                                                                  \
			{                                                                            \
				.step_pin = GPIO_DT_SPEC_INST_GET(inst, step_gpios),                 \
				.dir_pin = GPIO_DT_SPEC_INST_GET(inst, dir_gpios),                   \
				.invert_direction =                                                  \
					DT_INST_PROP_OR(inst, invert_direction, false),              \
				.dual_edge_step =                                                    \
					DT_INST_PROP_OR(inst, dual_edge_step, false),                \
			},                                                                           \
		.en_pin = GPIO_DT_SPEC_INST_GET(inst, en_gpios),                                 \
		.msx_resolutions = msx_table,                                                        \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),				   \
			(.msx_pins = tmc22xx_stepper_msx_pins_##inst)) };     \
	static struct tmc22xx_data tmc22xx_data_##inst = {                                           \
		.motion_controller_data = {},                                                        \
		.resolution = DT_INST_PROP(inst, micro_step_res),                                    \
	};                                                                                           \
	DEVICE_DT_INST_DEFINE(inst, tmc22xx_stepper_init, NULL, &tmc22xx_data_##inst,                \
			      &tmc22xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,     \
			      &tmc22xx_stepper_api);

#define DT_DRV_COMPAT adi_tmc2209
static enum stepper_micro_step_resolution tmc2209_msx_resolutions[MSX_PIN_STATE_COUNT] = {
	STEPPER_MICRO_STEP_8,
	STEPPER_MICRO_STEP_32,
	STEPPER_MICRO_STEP_64,
	STEPPER_MICRO_STEP_16,
};

DT_INST_FOREACH_STATUS_OKAY_VARGS(TMC22XX_STEPPER_DEFINE, tmc2209_msx_resolutions)

#undef DT_DRV_COMPAT
