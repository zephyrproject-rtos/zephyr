/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx_stepper_driver

#include "tmc50xx.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/drivers/stepper/stepper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

struct tmc50xx_stepper_driver_data {
	stepper_event_cb_t drv_event_cb;
	void *drv_event_cb_user_data;
};

struct tmc50xx_stepper_driver_config {
	const uint8_t index;
	const uint16_t default_micro_step_res;
	const int8_t sg_threshold;
	/* parent controller required for bus communication */
	const struct device *controller;
};

void tmc50xx_stepper_driver_trigger_cb(const struct device *dev, const enum stepper_event event)
{
	if (dev == NULL) {
		return;
	}

	struct tmc50xx_stepper_driver_data *data = dev->data;

	if (!data->drv_event_cb) {
		LOG_WRN_ONCE("No stepper driver callback registered");
		return;
	}

	data->drv_event_cb(dev, event, data->drv_event_cb_user_data);
}

static int tmc50xx_stepper_driver_set_event_cb(const struct device *stepper,
						  stepper_event_cb_t callback, void *user_data)
{
	struct tmc50xx_stepper_driver_data *data = stepper->data;

	data->drv_event_cb = callback;
	data->drv_event_cb_user_data = user_data;

	return 0;
}

static int tmc50xx_stepper_driver_enable(const struct device *dev)
{
	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	const struct tmc50xx_stepper_driver_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value |= TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
}

static int tmc50xx_stepper_driver_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	const struct tmc50xx_stepper_driver_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
}

static int tmc50xx_stepper_driver_set_micro_step_res(const struct device *dev,
						  enum stepper_micro_step_resolution res)
{
	const struct tmc50xx_stepper_driver_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(res))
		      << TMC5XXX_CHOPCONF_MRES_SHIFT);

	err = tmc50xx_write(config->controller, TMC50XX_CHOPCONF(config->index), reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc50xx_stepper_driver_get_micro_step_res(const struct device *dev,
						  enum stepper_micro_step_resolution *res)
{
	const struct tmc50xx_stepper_driver_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc50xx_read(config->controller, TMC50XX_CHOPCONF(config->index), &reg_value);
	if (err != 0) {
		return -EIO;
	}
	reg_value &= TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC5XXX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
	return 0;
}

static int tmc50xx_stepper_driver_init(const struct device *dev)
{
	const struct tmc50xx_stepper_driver_config *config = dev->config;
	int err;

	LOG_DBG("Controller: %s, Stepper: %s", config->controller->name, dev->name);
	if (!IN_RANGE(config->sg_threshold, TMC5XXX_SG_MIN_VALUE, TMC5XXX_SG_MAX_VALUE)) {
		LOG_ERR("Stallguard threshold out of range");
		return -EINVAL;
	}

	int32_t stall_guard_threshold = (int32_t)config->sg_threshold;

	err = tmc50xx_write(config->controller, TMC50XX_COOLCONF(config->index),
			    stall_guard_threshold << TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
	if (err != 0) {
		return -EIO;
	}
	err = tmc50xx_stepper_driver_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Setting stallguard %d", config->sg_threshold);
	return 0;
}

static DEVICE_API(stepper, tmc50xx_stepper_driver_api) = {
	.enable = tmc50xx_stepper_driver_enable,
	.disable = tmc50xx_stepper_driver_disable,
	.set_micro_step_res = tmc50xx_stepper_driver_set_micro_step_res,
	.get_micro_step_res = tmc50xx_stepper_driver_get_micro_step_res,
	.set_event_cb = tmc50xx_stepper_driver_set_event_cb,
};

#define TMC50XX_STEPPER_DRV_DEFINE(inst)                                                           \
	COND_CODE_1(DT_PROP_EXISTS(inst, stallguard_threshold_velocity),                           \
	BUILD_ASSERT(DT_PROP(inst, stallguard_threshold_velocity),                                 \
			"stallguard threshold velocity must be a positive value"), ());            \
	static const struct tmc50xx_stepper_driver_config tmc50xx_stepper_driver_config_##inst = { \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.index = DT_INST_PROP(inst, idx),                                                  \
		.sg_threshold = DT_INST_PROP(inst, stallguard2_threshold),                         \
	};                                                                                         \
	static struct tmc50xx_stepper_driver_data tmc50xx_stepper_driver_data_##inst;              \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_stepper_driver_init, NULL,                             \
			      &tmc50xx_stepper_driver_data_##inst,                                 \
			      &tmc50xx_stepper_driver_config_##inst, POST_KERNEL,                  \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc50xx_stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_STEPPER_DRV_DEFINE)
