/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../step_dir/step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc22xx, CONFIG_STEPPER_LOG_LEVEL);

#define MSX_PIN_COUNT       2
#define MSX_PIN_STATE_COUNT 4

struct tmc22xx_config {
	struct step_dir_stepper_common_config common;
	const struct gpio_dt_spec enable_pin;
	const struct gpio_dt_spec *msx_pins;
	enum stepper_micro_step_resolution *msx_resolutions;
};

struct tmc22xx_data {
	struct step_dir_stepper_common_data common;
	enum stepper_micro_step_resolution resolution;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct tmc22xx_config, struct tmc22xx_data);

static int tmc22xx_stepper_enable(const struct device *dev, const bool enable)
{
	const struct tmc22xx_config *config = dev->config;

	LOG_DBG("Stepper motor controller %s %s", dev->name, enable ? "enabled" : "disabled");
	if (enable) {
		return gpio_pin_set_dt(&config->enable_pin, 1);
	} else {
		return gpio_pin_set_dt(&config->enable_pin, 0);
	}
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
	return -EINVAL;
}

static int tmc22xx_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *micro_step_res)
{
	struct tmc22xx_data *data = dev->data;

	*micro_step_res = data->resolution;
	return 0;
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
	struct tmc22xx_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->enable_pin)) {
		LOG_ERR("GPIO pins are not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->enable_pin, GPIO_OUTPUT);
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

	ret = step_dir_stepper_common_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init step dir common stepper: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(stepper, tmc22xx_stepper_api) = {
	.enable = tmc22xx_stepper_enable,
	.move_by = step_dir_stepper_common_move_by,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = step_dir_stepper_common_move_to,
	.set_max_velocity = step_dir_stepper_common_set_max_velocity,
	.run = step_dir_stepper_common_run,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = tmc22xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc22xx_stepper_get_micro_step_res,
};

#define TMC22XX_STEPPER_DEFINE(inst, msx_table)                                                    \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios), (                                       \
	static const struct gpio_dt_spec tmc22xx_stepper_msx_pins_##inst[] = {                     \
		DT_INST_FOREACH_PROP_ELEM_SEP(                                                     \
			inst, msx_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)                              \
		),                                                                                 \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(tmc22xx_stepper_msx_pins_##inst) == MSX_PIN_COUNT,                      \
		"Two microstep config pins needed");                                               \
	))                                                                                         \
                                                                                                   \
	static const struct tmc22xx_config tmc22xx_config_##inst = {                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.enable_pin = GPIO_DT_SPEC_INST_GET(inst, en_gpios),	                           \
		.msx_resolutions = msx_table,                                                      \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),				   \
		(.msx_pins = tmc22xx_stepper_msx_pins_##inst))					   \
	};                                                                                         \
	static struct tmc22xx_data tmc22xx_data_##inst = {                                         \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                         \
		.resolution = DT_INST_PROP(inst, micro_step_res),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc22xx_stepper_init, NULL, &tmc22xx_data_##inst,              \
			      &tmc22xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
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
