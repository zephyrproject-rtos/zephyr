/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <step_dir_stepper_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc22xx, CONFIG_STEPPER_LOG_LEVEL);

#define MSX_PIN_COUNT       2
#define MSX_PIN_STATE_COUNT 4

struct tmc22xx_config {
	struct step_dir_stepper_common_config common;
	const struct gpio_dt_spec enable_pin;
	struct gpio_dt_spec m0_pin;
	struct gpio_dt_spec m1_pin;
	enum stepper_micro_step_resolution *msx_resolutions;
};

struct tmc22xx_data {
	struct step_dir_stepper_common_data common;
	enum stepper_micro_step_resolution resolution;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct tmc22xx_config, struct tmc22xx_data);

static int tmc22xx_stepper_enable(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;

	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	return gpio_pin_set_dt(&config->enable_pin, 1);
}

static int tmc22xx_stepper_disable(const struct device *dev)
{
	const struct tmc22xx_config *config = dev->config;

	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	return gpio_pin_set_dt(&config->enable_pin, 0);
}

static int tmc22xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution micro_step_res)
{
	struct tmc22xx_data *data = dev->data;
	const struct tmc22xx_config *config = dev->config;
	int ret;

	if ((config->m0_pin.port == NULL) || (config->m1_pin.port == NULL)) {
		LOG_ERR("%s: Failed to set microstep resolution: microstep pins are not defined "
			"(error: %d)",
			dev->name, -ENOTSUP);
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < MSX_PIN_STATE_COUNT; i++) {
		if (micro_step_res != config->msx_resolutions[i]) {
			continue;
		}

		ret = gpio_pin_set_dt(&config->m0_pin, i & 0x01);
		if (ret < 0) {
			LOG_ERR("Failed to set MS1 pin: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->m1_pin, (i & 0x02) >> 1);
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

	if (!gpio_is_ready_dt(&config->m0_pin)) {
		LOG_ERR("MS1 pin not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->m0_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure ms1 pin");
		return ret;
	}

	if (!gpio_is_ready_dt(&config->m1_pin)) {
		LOG_ERR("MS2 pin not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->m1_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure ms2 pin");
		return ret;
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

	if ((config->m0_pin.port != NULL) && (config->m1_pin.port != NULL)) {
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
	.disable = tmc22xx_stepper_disable,
	.move_by = step_dir_stepper_common_move_by,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = step_dir_stepper_common_move_to,
	.set_microstep_interval = step_dir_stepper_common_set_microstep_interval,
	.run = step_dir_stepper_common_run,
	.stop = step_dir_stepper_common_stop,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = tmc22xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc22xx_stepper_get_micro_step_res,
};

#define TMC22XX_STEPPER_DEFINE(inst, msx_table)                                                    \
	static const struct tmc22xx_config tmc22xx_config_##inst = {                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.enable_pin = GPIO_DT_SPEC_INST_GET(inst, en_gpios),	                           \
		.msx_resolutions = msx_table,                                                      \
		.m0_pin = GPIO_DT_SPEC_INST_GET_OR(inst, m0_gpios, {0}),                           \
		.m1_pin = GPIO_DT_SPEC_INST_GET_OR(inst, m1_gpios, {0}),                           \
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
