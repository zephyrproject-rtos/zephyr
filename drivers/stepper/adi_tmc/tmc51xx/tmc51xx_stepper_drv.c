/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/gpio.h>
#include "tmc51xx.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc51xx, CONFIG_STEPPER_LOG_LEVEL);

#define DT_DRV_COMPAT adi_tmc51xx_stepper_drv

struct tmc51xx_stepper_drv_config {
	const uint16_t default_micro_step_res;
	const int8_t sg_threshold;
	const struct gpio_dt_spec en_gpio;
	/* parent controller required for bus communication */
	const struct device *controller;
};

struct tmc51xx_stepper_drv_data {
	stepper_drv_event_cb_t drv_event_cb;
	void *drv_event_cb_user_data;
};

void tmc51xx_stepper_drv_trigger_cb(const struct device *dev, const enum stepper_drv_event event)
{
	struct tmc51xx_stepper_drv_data *data = dev->data;

	if (!data->drv_event_cb) {
		LOG_WRN_ONCE("No stepper driver callback registered");
		return;
	}
	data->drv_event_cb(dev, event, data->drv_event_cb_user_data);
}

static int tmc51xx_stepper_drv_set_event_callback(const struct device *stepper,
						  stepper_drv_event_cb_t callback, void *user_data)
{
	struct tmc51xx_stepper_drv_data *data = stepper->data;

	data->drv_event_cb = callback;
	data->drv_event_cb_user_data = user_data;

	return 0;
}

static int tmc51xx_stepper_drv_enable(const struct device *dev)
{
	const struct tmc51xx_stepper_drv_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	LOG_DBG("Enabling Stepper motor controller %s", dev->name);

	if (config->en_gpio.port != NULL) {
		/* Assert enable line first so the driver can energize even if SPI config
		 * later fails (e.g., bus wiring issues). The register write still runs so
		 * we get useful errors in logs.
		 */
		err = gpio_pin_set_dt(&config->en_gpio, 1);
		if (err < 0) {
			LOG_ERR("Failed to assert enable GPIO (%d)", err);
			return err;
		}
	}

	err = tmc51xx_read(controller, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value |= TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc51xx_write(controller, TMC51XX_CHOPCONF, reg_value);
}

static int tmc51xx_stepper_drv_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	const struct tmc51xx_stepper_drv_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	if (config->en_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->en_gpio, 0);
		if (err < 0) {
			LOG_ERR("Failed to deassert enable GPIO (%d)", err);
			return err;
		}
	}

	err = tmc51xx_read(controller, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc51xx_write(controller, TMC51XX_CHOPCONF, reg_value);
}

static int tmc51xx_stepper_drv_set_micro_step_res(const struct device *dev,
					      enum stepper_drv_micro_step_resolution res)
{
	const struct tmc51xx_stepper_drv_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(controller, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_DRV_MICRO_STEP_256) - LOG2(res))
		      << TMC5XXX_CHOPCONF_MRES_SHIFT);

	err = tmc51xx_write(controller, TMC51XX_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc51xx_stepper_drv_get_micro_step_res(const struct device *dev,
					      enum stepper_drv_micro_step_resolution *res)
{
	const struct tmc51xx_stepper_drv_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(controller, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}
	reg_value &= TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC5XXX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_DRV_MICRO_STEP_256) - reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
	return 0;
}

static int tmc51xx_stepper_drv_init(const struct device *dev)
{
	const struct tmc51xx_stepper_drv_config *config = dev->config;
	const struct device *controller = config->controller;
	int err;

	if (config->en_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->en_gpio)) {
			LOG_ERR("Enable GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure enable GPIO (%d)", err);
			return err;
		}
	}

	if (!IN_RANGE(config->sg_threshold, TMC5XXX_SG_MIN_VALUE, TMC5XXX_SG_MAX_VALUE)) {
		LOG_ERR("Stallguard threshold out of range");
		return -EINVAL;
	}

	int32_t stall_guard_threshold = (int32_t)config->sg_threshold;

	err = tmc51xx_write(controller, TMC51XX_COOLCONF,
			    stall_guard_threshold << TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_stepper_drv_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Setting stallguard %d", config->sg_threshold);
	return 0;
}

static DEVICE_API(stepper_drv, tmc51xx_stepper_drv_api) = {
	.enable = tmc51xx_stepper_drv_enable,
	.disable = tmc51xx_stepper_drv_disable,
	.set_micro_step_res = tmc51xx_stepper_drv_set_micro_step_res,
	.get_micro_step_res = tmc51xx_stepper_drv_get_micro_step_res,
	.set_event_cb = tmc51xx_stepper_drv_set_event_callback,
};

#define TMC51XX_STEPPER_DRV_DEFINE(inst)                                                           \
	COND_CODE_1(DT_PROP_EXISTS(inst, stallguard_threshold_velocity),                           \
	BUILD_ASSERT(DT_PROP(inst, stallguard_threshold_velocity),                                 \
			"stallguard threshold velocity must be a positive value"), ());            \
	static const struct tmc51xx_stepper_drv_config tmc51xx_stepper_drv_config_##inst = {       \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.sg_threshold = DT_INST_PROP(inst, stallguard2_threshold),                         \
		.en_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                          \
	};                                                                                         \
	static struct tmc51xx_stepper_drv_data tmc51xx_stepper_drv_data_##inst;                    \
	DEVICE_DT_INST_DEFINE(inst, tmc51xx_stepper_drv_init, NULL,                                \
			      &tmc51xx_stepper_drv_data_##inst,                                    \
			      &tmc51xx_stepper_drv_config_##inst, POST_KERNEL,                     \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc51xx_stepper_drv_api);

DT_INST_FOREACH_STATUS_OKAY(TMC51XX_STEPPER_DRV_DEFINE)
