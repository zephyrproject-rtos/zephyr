/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc522x_stepper_driver

#include <zephyr/drivers/stepper/stepper.h>

#include "tmc522x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc522x, CONFIG_STEPPER_LOG_LEVEL);

struct tmc522x_stepper_drv_data {
	stepper_event_cb_t drv_event_cb;
	void *drv_event_cb_user_data;
};

struct tmc522x_stepper_drv_config {
	const uint8_t index;
	const uint16_t default_micro_step_res;
	const int8_t sgt; /* StallGuard2 threshold */

	/* Hardware configuration properties (from child DT node) */
	const uint8_t fs_gain;
	const uint8_t fs_sense;
	const uint8_t global_scaler;
	const uint8_t ihold;
	const uint8_t irun;
	const uint8_t irundelay;
	const uint8_t iholddelay;
	const uint8_t tpowerdown;
	const uint8_t toff;
	const uint8_t tbl;
	const uint8_t hstrt;
	const uint8_t hend;

	/* Parent controller required for bus communication */
	const struct device *controller;
};

/* Set event callback */
static int tmc522x_stepper_drv_set_event_callback(const struct device *stepper,
						  stepper_event_cb_t callback, void *user_data)
{
	struct tmc522x_stepper_drv_data *data = stepper->data;

	data->drv_event_cb = callback;
	data->drv_event_cb_user_data = user_data;

	return 0;
}

/* Enable driver outputs */
static int tmc522x_stepper_drv_enable(const struct device *dev)
{
	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	const struct tmc522x_stepper_drv_config *config = dev->config;
	uint32_t reg_value;
	int err;

	/* Read CHOPCONF register */
	err = tmc522x_read(config->controller, TMC522X_REG_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	/* Enable software driver control (DRV_EN_SW bit) */
	reg_value |= TMC522X_CHOPCONF_DRV_EN_SW_MASK;

	/* Write back */
	err = tmc522x_write(config->controller, TMC522X_REG_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	/* Enable via GCONF register - set drv_enn bit (bit 9) */
	err = tmc522x_read(config->controller, TMC522X_REG_GCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value |= TMC522X_GCONF_DRV_ENN;

	err = tmc522x_write(config->controller, TMC522X_REG_GCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("TMC522X driver enabled");
	return 0;
}

/* Disable driver outputs */
static int tmc522x_stepper_drv_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	const struct tmc522x_stepper_drv_config *config = dev->config;
	uint32_t reg_value;
	int err;

	/* Disable via GCONF register - clear drv_enn bit (bit 9) */
	err = tmc522x_read(config->controller, TMC522X_REG_GCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC522X_GCONF_DRV_ENN;

	err = tmc522x_write(config->controller, TMC522X_REG_GCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("TMC522X driver disabled");
	return 0;
}

/* Set microstep resolution */
static int tmc522x_stepper_drv_set_micro_step_res(const struct device *dev,
						  enum stepper_micro_step_resolution res)
{
	const struct tmc522x_stepper_drv_config *config = dev->config;
	uint32_t reg_value;
	uint8_t mres;
	int err;

	/* Convert enum to MRES register value
	 * MRES encoding: 0=256, 1=128, 2=64, 3=32, 4=16, 5=8, 6=4, 7=2, 8=fullstep
	 */
	mres = 8 - LOG2(res);

	err = tmc522x_read(config->controller, TMC522X_REG_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC522X_CHOPCONF_MRES_MASK;
	reg_value |= ((uint32_t)mres << TMC522X_CHOPCONF_MRES_SHIFT) & TMC522X_CHOPCONF_MRES_MASK;

	err = tmc522x_write(config->controller, TMC522X_REG_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 1/%d", dev->name, res);
	return 0;
}

/* Get microstep resolution */
static int tmc522x_stepper_drv_get_micro_step_res(const struct device *dev,
						  enum stepper_micro_step_resolution *res)
{
	const struct tmc522x_stepper_drv_config *config = dev->config;
	uint32_t reg_value;
	uint8_t mres;
	int err;

	err = tmc522x_read(config->controller, TMC522X_REG_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	mres = (reg_value & TMC522X_CHOPCONF_MRES_MASK) >> TMC522X_CHOPCONF_MRES_SHIFT;
	*res = (1 << (8 - mres));

	LOG_DBG("Stepper motor controller %s get micro step resolution: 1/%d", dev->name, *res);
	return 0;
}

/* Configure hardware registers (moved from MFD parent) */
static int tmc522x_stepper_drv_configure_hardware(const struct device *dev)
{
	const struct tmc522x_stepper_drv_config *config = dev->config;
	int err;
	uint32_t reg_val;

	LOG_DBG("Configuring TMC522x hardware registers for %s", dev->name);

	/* 1. Configure DRVCONF (FS_GAIN, FS_SENSE) */
	reg_val = FIELD_PREP(TMC522X_DRVCONF_FS_GAIN_MASK, config->fs_gain) |
		  FIELD_PREP(TMC522X_DRVCONF_FS_SENSE_MASK, config->fs_sense);
	err = tmc522x_write(config->controller, TMC522X_REG_DRVCONF, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write DRVCONF");
		return err;
	}
	LOG_DBG("DRVCONF configured: fs_gain=%u, fs_sense=%u", config->fs_gain, config->fs_sense);

	/* 2. Configure GLOBALSCALER */
	reg_val = config->global_scaler & 0xFF;
	err = tmc522x_write(config->controller, TMC522X_REG_GLOBALSCALER, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write GLOBALSCALER");
		return err;
	}
	LOG_DBG("GLOBALSCALER configured: %u", config->global_scaler);

	/* 3. Configure IHOLD_IRUN (CRITICAL - motor current settings) */
	reg_val = FIELD_PREP(TMC522X_IHOLD_MASK, config->ihold) |
		  FIELD_PREP(TMC522X_IRUN_MASK, config->irun) |
		  FIELD_PREP(TMC522X_IHOLDDELAY_MASK, config->iholddelay) |
		  FIELD_PREP(TMC522X_IRUNDELAY_MASK, config->irundelay);
	err = tmc522x_write(config->controller, TMC522X_REG_IHOLD_IRUN, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write IHOLD_IRUN");
		return err;
	}
	LOG_DBG("IHOLD_IRUN configured: ihold=%u, irun=%u, iholddelay=%u, irundelay=%u",
		config->ihold, config->irun, config->iholddelay, config->irundelay);

	/* 4. Configure TPOWERDOWN */
	reg_val = config->tpowerdown & 0xFF;
	err = tmc522x_write(config->controller, TMC522X_REG_TPOWERDOWN, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write TPOWERDOWN");
		return err;
	}
	LOG_DBG("TPOWERDOWN configured: %u", config->tpowerdown);

	/* 5. Configure CHOPCONF (CRITICAL - enables chopper and software control) */
	reg_val = FIELD_PREP(TMC522X_CHOPCONF_TOFF_MASK, config->toff) |
		  FIELD_PREP(TMC522X_CHOPCONF_TBL_MASK, config->tbl) |
		  FIELD_PREP(TMC522X_CHOPCONF_HSTRT_TFD210_MASK, config->hstrt) |
		  FIELD_PREP(TMC522X_CHOPCONF_HEND_OFFSET_MASK, config->hend) |
		  TMC522X_CHOPCONF_DRV_EN_SW_MASK; /* Enable software control */
	/* Note: MRES (microstep resolution) will be set later */
	err = tmc522x_write(config->controller, TMC522X_REG_CHOPCONF, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write CHOPCONF");
		return err;
	}
	LOG_DBG("CHOPCONF configured: toff=%u, tbl=%u, hstrt=%u, hend=%u, DRV_EN_SW=1",
		config->toff, config->tbl, config->hstrt, config->hend);

	/* 6. Configure PWMCONF for StealthChop */
	/* Use recommended StealthChop2 settings with automatic tuning */
	reg_val = FIELD_PREP(TMC522X_PWMCONF_PWM_OFS_MASK, 30) | /* PWM offset amplitude */
		  FIELD_PREP(TMC522X_PWMCONF_PWM_GRAD_MASK, 1) | /* PWM gradient */
		  FIELD_PREP(TMC522X_PWMCONF_FREQ_MASK, 0) |     /* PWM frequency (lowest) */
		  TMC522X_PWMCONF_AUTOSCALE_MASK |               /* Enable auto amplitude scaling */
		  TMC522X_PWMCONF_AUTOGRAD_MASK;                 /* Enable auto gradient tuning */
	err = tmc522x_write(config->controller, TMC522X_REG_PWMCONF, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write PWMCONF");
		return err;
	}
	LOG_DBG("PWMCONF configured for StealthChop2 (auto scaling enabled)");

	LOG_DBG("TMC522x hardware configuration complete for %s", dev->name);
	return 0;
}

/* Initialization */
static int tmc522x_stepper_drv_init(const struct device *dev)
{
	const struct tmc522x_stepper_drv_config *config = dev->config;
	uint32_t val;
	int err;

	LOG_DBG("Controller: %s, Stepper drv: %s", config->controller->name, dev->name);

	/* Configure hardware registers (CRITICAL - must be done first) */
	err = tmc522x_stepper_drv_configure_hardware(dev);
	if (err < 0) {
		LOG_ERR("Failed to configure hardware registers");
		return err;
	}

	/* Validate StallGuard threshold range (-64 to 63 for 7-bit signed) */
	if (!IN_RANGE(config->sgt, -64, 63)) {
		LOG_ERR("StallGuard threshold out of range: %d", config->sgt);
		return -EINVAL;
	}

	/* Configure StallGuard2 threshold in COOLCONF register */
	err = tmc522x_read(config->controller, TMC522X_REG_COOLCONF, &val);
	if (err) {
		LOG_ERR("Failed to read COOLCONF");
		return err;
	}

	val &= ~TMC522X_COOLCONF_SGT_MASK;
	val |= ((uint32_t)(config->sgt & 0x7F) << TMC522X_COOLCONF_SGT_SHIFT) &
	       TMC522X_COOLCONF_SGT_MASK;

	err = tmc522x_write(config->controller, TMC522X_REG_COOLCONF, val);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Setting StallGuard2 threshold: %d", config->sgt);

	/* Set microstep resolution */
	err = tmc522x_stepper_drv_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("TMC522x stepper_drv initialized (microsteps=1/%d, sgt=%d)",
		config->default_micro_step_res, config->sgt);

	return 0;
}

/* Stepper_drv API */
static DEVICE_API(stepper, tmc522x_stepper_driver_api) = {
	.enable = tmc522x_stepper_drv_enable,
	.disable = tmc522x_stepper_drv_disable,
	.set_micro_step_res = tmc522x_stepper_drv_set_micro_step_res,
	.get_micro_step_res = tmc522x_stepper_drv_get_micro_step_res,
	.set_event_cb = tmc522x_stepper_drv_set_event_callback,
};

#define TMC522X_STEPPER_DRV_DEFINE(inst)                                                           \
	static const struct tmc522x_stepper_drv_config tmc522x_stepper_drv_config_##inst = {       \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.index = DT_INST_PROP(inst, idx),                                                  \
		.sgt = DT_INST_PROP(inst, stallguard2_threshold),                                  \
		.fs_gain = DT_INST_PROP(inst, fs_gain),                                            \
		.fs_sense = DT_INST_PROP(inst, fs_sense),                                          \
		.global_scaler = DT_INST_PROP(inst, global_scaler),                                \
		.ihold = DT_INST_PROP(inst, ihold),                                                \
		.irun = DT_INST_PROP(inst, irun),                                                  \
		.irundelay = DT_INST_PROP(inst, irundelay),                                        \
		.iholddelay = DT_INST_PROP(inst, iholddelay),                                      \
		.tpowerdown = DT_INST_PROP(inst, tpowerdown),                                      \
		.toff = DT_INST_PROP(inst, toff),                                                  \
		.tbl = DT_INST_PROP(inst, tbl),                                                    \
		.hstrt = DT_INST_PROP(inst, hstrt),                                                \
		.hend = DT_INST_PROP(inst, hend),                                                  \
	};                                                                                         \
	static struct tmc522x_stepper_drv_data tmc522x_stepper_drv_data_##inst;                    \
	DEVICE_DT_INST_DEFINE(inst, tmc522x_stepper_drv_init, NULL,                                \
			      &tmc522x_stepper_drv_data_##inst,                                    \
			      &tmc522x_stepper_drv_config_##inst, POST_KERNEL,                     \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc522x_stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMC522X_STEPPER_DRV_DEFINE)
