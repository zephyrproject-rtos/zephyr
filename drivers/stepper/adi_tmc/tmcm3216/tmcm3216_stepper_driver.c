/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmcm3216_stepper_driver

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/logging/log.h>

#include <adi_tmcm_rs485.h>
#include "tmcm3216.h"
#include "tmcm3216_reg.h"

LOG_MODULE_DECLARE(tmcm3216, CONFIG_STEPPER_LOG_LEVEL);

static int tmcm3216_stepper_driver_enable(const struct device *dev)
{
	LOG_DBG("Enabling stepper %s", dev->name);
	const struct tmcm3216_stepper_driver_config *config = dev->config;
	int err;

	uint32_t run_current = 100; /* 100 = default run current (0-255) */

	err = tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_CURRENT, run_current);
	if (err != 0) {
		LOG_ERR("Failed to set run current");
		return err;
	}

	LOG_DBG("%s enabled with run current %d", dev->name, run_current);
	return 0;
}

static int tmcm3216_stepper_driver_disable(const struct device *dev)
{
	LOG_DBG("Disabling stepper %s", dev->name);
	const struct tmcm3216_stepper_driver_config *config = dev->config;
	struct tmcm_rs485_cmd cmd;
	uint32_t reply;
	int err;

	cmd.module_addr = 0; /* filled by send_command from parent */
	cmd.command_number = TMCL_CMD_MST;
	cmd.type_number = 0;
	cmd.bank_number = config->motor_index;
	cmd.data = 0;

	err = tmcm3216_send_command(dev, &cmd, &reply);
	if (err != 0) {
		LOG_ERR("Failed to stop motor");
		return err;
	}

	err = tmcm3216_sap(dev, config->motor_index, TMCL_AP_MAX_CURRENT, 0);
	if (err != 0) {
		LOG_ERR("Failed to set current to 0");
		return err;
	}

	err = tmcm3216_sap(dev, config->motor_index, TMCL_AP_STANDBY_CURRENT, 0);
	if (err != 0) {
		LOG_ERR("Failed to set standby current to 0");
		return err;
	}

	LOG_DBG("%s disabled (motor stopped, currents set to 0)", dev->name);
	return 0;
}

static int tmcm3216_stepper_driver_set_micro_step_res(const struct device *dev,
						      enum stepper_micro_step_resolution res)
{
	const struct tmcm3216_stepper_driver_config *config = dev->config;
	uint8_t mres_value;

	if (res > STEPPER_MICRO_STEP_256) {
		LOG_ERR("Invalid microstep resolution: %d", res);
		return -EINVAL;
	}

	mres_value = MICRO_STEP_RES_INDEX(res);

	return tmcm3216_sap(dev, config->motor_index, TMCL_AP_MICROSTEP_RESOLUTION, mres_value);
}

static int tmcm3216_stepper_driver_get_micro_step_res(const struct device *dev,
						      enum stepper_micro_step_resolution *res)
{
	const struct tmcm3216_stepper_driver_config *config = dev->config;
	uint32_t mres_value;
	int err;

	err = tmcm3216_gap(dev, config->motor_index, TMCL_AP_MICROSTEP_RESOLUTION, &mres_value);
	if (err != 0) {
		return err;
	}

	/* Convert TMCL value (0-8) to Zephyr enum */
	switch (mres_value) {
	case TMCL_MRES_FULLSTEP:
		*res = STEPPER_MICRO_STEP_1;
		break;
	case TMCL_MRES_HALFSTEP:
		*res = STEPPER_MICRO_STEP_2;
		break;
	case TMCL_MRES_4:
		*res = STEPPER_MICRO_STEP_4;
		break;
	case TMCL_MRES_8:
		*res = STEPPER_MICRO_STEP_8;
		break;
	case TMCL_MRES_16:
		*res = STEPPER_MICRO_STEP_16;
		break;
	case TMCL_MRES_32:
		*res = STEPPER_MICRO_STEP_32;
		break;
	case TMCL_MRES_64:
		*res = STEPPER_MICRO_STEP_64;
		break;
	case TMCL_MRES_128:
		*res = STEPPER_MICRO_STEP_128;
		break;
	case TMCL_MRES_256:
		*res = STEPPER_MICRO_STEP_256;
		break;
	default:
		LOG_ERR("Invalid microstep value from device: %d", mres_value);
		return -EIO;
	}

	LOG_DBG("%s get microstep resolution: 1/%d", dev->name, *res);
	return 0;
}

/* Initialize individual stepper motor */
static int tmcm3216_stepper_driver_init(const struct device *dev)
{
	const struct tmcm3216_stepper_driver_config *config = dev->config;
	int err;

	/* Verify parent controller is ready */
	if (!device_is_ready(config->controller)) {
		LOG_ERR("Parent controller not ready");
		return -ENODEV;
	}

	/* Set default microstep resolution */
	err = tmcm3216_stepper_driver_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		LOG_ERR("Failed to set default microstep resolution");
		return err;
	}

	LOG_INF("TMCM-3216 stepper %s initialized (motor: %d, microsteps: 1/%d)", dev->name,
		config->motor_index, config->default_micro_step_res);

	return 0;
}

static DEVICE_API(stepper, tmcm3216_stepper_driver_api) = {
	.enable = tmcm3216_stepper_driver_enable,
	.disable = tmcm3216_stepper_driver_disable,
	.set_micro_step_res = tmcm3216_stepper_driver_set_micro_step_res,
	.get_micro_step_res = tmcm3216_stepper_driver_get_micro_step_res,
};

#define TMCM3216_STEPPER_DRIVER_DEFINE(inst)                                                       \
	static struct tmcm3216_stepper_driver_data tmcm3216_stepper_driver_data_##inst;            \
	static const struct tmcm3216_stepper_driver_config tmcm3216_stepper_driver_config_##inst = \
		{                                                                                  \
			.controller = DEVICE_DT_GET(DT_INST_PARENT(inst)),                         \
			.motor_index = DT_INST_PROP(inst, idx),                                    \
			.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmcm3216_stepper_driver_init, NULL,                            \
			      &tmcm3216_stepper_driver_data_##inst,                                \
			      &tmcm3216_stepper_driver_config_##inst, POST_KERNEL,                 \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmcm3216_stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMCM3216_STEPPER_DRIVER_DEFINE)
