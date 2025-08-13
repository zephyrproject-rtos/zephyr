/*
 * Copyright 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_vref

#include <errno.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/clock/mspm0_clock.h>
#include <zephyr/dt-bindings/regulator/mspm0_vref.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/dl_vref.h>

LOG_MODULE_REGISTER(vref, CONFIG_LOG_DEFAULT_LEVEL);

#define STARTUP_TIME			200
#define VREF_STARTUP_TIME		(STARTUP_TIME << 1)
#define VREF_CLOCK_DIVIDE_RATIO(n)	CONCAT(DL_VREF_CLOCK_DIVIDE_, DT_INST_PROP(n, ti_clk_div))
#define VREF_1_4V	1400000
#define VREF_2_5V	2500000

struct regulator_mspm0_vref_data {
	struct regulator_common_data common;
	DL_VREF_Config vref_cfg;
};

struct regulator_mspm0_vref_config {
	struct regulator_common_config common;
	const struct pinctrl_dev_config *vref_pin;
	DL_VREF_ClockConfig vref_clock_cfg;
	VREF_Regs *regs;
};

static int regulator_mspm0_vref_enable(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	int ret;

	/* Enable internal reference buffer */
	DL_VREF_enableInternalRef(config->regs);

	k_busy_wait(VREF_STARTUP_TIME);

	ret = DL_VREF_getStatus(config->regs);
	if (ret != 1) {
		LOG_ERR("Device is not ready !");
		return -ENODEV;
	}

	return 0;
}

static int regulator_mspm0_vref_disable(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;

	DL_VREF_disableInternalRef(config->regs);
	return 0;
}

static int regulator_mspm0_vref_set_voltage(const struct device *dev, int32_t min_uv,
					    int32_t max_uv)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;

	if (min_uv <= 2500000 && max_uv >= 2500000) {
		data->vref_cfg.bufConfig = DL_VREF_BUFCONFIG_OUTPUT_2_5V;
	} else if (min_uv <= 1400000 && max_uv >= 1400000) {
		data->vref_cfg.bufConfig = DL_VREF_BUFCONFIG_OUTPUT_1_4V;
	} else {
		LOG_ERR("Invalid voltage range !!");
		return -EINVAL;
	}

	DL_VREF_disableInternalRef(config->regs);

	DL_VREF_configReference(config->regs, &data->vref_cfg);

	DL_VREF_enableInternalRef(config->regs);

	return 0;
}

static int regulator_mspm0_vref_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	VREF_Regs *regs = config->regs;

	if (volt_uv == NULL) {
		return -EINVAL;
	}
	if (regs->CTL0 & VREF_CTL0_BUFCONFIG_MASK) {
		*volt_uv = 1400000;
	} else {
		*volt_uv = 2500000;
	}
	return 0;
}

static int regulator_mspm0_vref_set_mode(const struct device *dev, regulator_mode_t mode)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;

	switch (mode) {
	case MSPM0_VREF_MODE_SHMODE:
		data->vref_cfg.shModeEnable = DL_VREF_SHMODE_ENABLE;
		break;
	case MSPM0_VREF_MODE_NORMAL:
		data->vref_cfg.shModeEnable = DL_VREF_SHMODE_DISABLE;
		break;
	default:
		return -EINVAL;
	}

	DL_VREF_disableInternalRef(config->regs);

	DL_VREF_configReference(config->regs, &data->vref_cfg);

	DL_VREF_enableInternalRef(config->regs);

	return 0;
}

static int regulator_mspm0_vref_get_mode(const struct device *dev, regulator_mode_t *mode)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	VREF_Regs *regs = config->regs;

	if (mode == NULL) {
		return -EINVAL;
	}

	if (regs->CTL0 & VREF_CTL0_SHMODE_MASK) {
		*mode = MSPM0_VREF_MODE_SHMODE;
	} else {
		*mode = MSPM0_VREF_MODE_NORMAL;
	}
	return 0;
}

static int regulator_mspm0_vref_init(const struct device *dev)
{
	const struct regulator_mspm0_vref_config *config = dev->config;
	struct regulator_mspm0_vref_data *data = dev->data;
	int ret;

	regulator_common_data_init(dev);

	ret = pinctrl_apply_state(config->vref_pin, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("pinctrl err");
		return ret;
	}

	/* Enable power */
	DL_VREF_enablePower(config->regs);
	ret = DL_VREF_isPowerEnabled(config->regs);
	if (!ret) {
		LOG_ERR("power enable failed !");
		return ret;
	}

	/* Set Vref Configs */
	DL_VREF_configReference(config->regs, &data->vref_cfg);
	DL_VREF_setClockConfig(config->regs, &config->vref_clock_cfg);

	ret = regulator_common_init(dev, false);
	if (ret) {
		LOG_ERR("common init err!!");
		return ret;
	}

	return 0;
}

static DEVICE_API(regulator, mspm0_vref_api) = {
	.enable = regulator_mspm0_vref_enable,
	.disable = regulator_mspm0_vref_disable,
	.set_voltage = regulator_mspm0_vref_set_voltage,
	.get_voltage = regulator_mspm0_vref_get_voltage,
	.set_mode = regulator_mspm0_vref_set_mode,
	.get_mode = regulator_mspm0_vref_get_mode,
};

#define REGULATOR_MSPM0_VREF_DEFINE(n)							\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static struct regulator_mspm0_vref_data data_##n = {				\
		.vref_cfg = {								\
			.vrefEnable = DL_VREF_ENABLE_DISABLE,				\
			.bufConfig = (DT_INST_PROP(n, regulator_uv) == VREF_1_4V	\
					? DL_VREF_BUFCONFIG_OUTPUT_1_4V			\
					: DL_VREF_BUFCONFIG_OUTPUT_2_5V),		\
			.shModeEnable = DT_INST_PROP(n, ti_sample_hold_enable)		\
				? DL_VREF_SHMODE_ENABLE : DL_VREF_SHMODE_DISABLE,	\
			.shCycleCount = DT_INST_PROP(n, ti_sample_cycles),		\
			.holdCycleCount = DT_INST_PROP(n, ti_hold_cycles),		\
		},									\
	};										\
											\
	static const struct regulator_mspm0_vref_config config_##n = {			\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(n),			\
		.vref_pin = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.regs = (VREF_Regs *)DT_INST_REG_ADDR(n),				\
		.vref_clock_cfg = {							\
			.clockSel =							\
				  (DT_INST_CLOCKS_CELL(n, clk) == MSPM0_CLOCK_LFCLK)	\
				? DL_VREF_CLOCK_LFCLK					\
				: (DT_INST_CLOCKS_CELL(n, clk) == MSPM0_CLOCK_MFCLK)	\
				? DL_VREF_CLOCK_MFCLK					\
				: DL_VREF_CLOCK_BUSCLK,					\
			.divideRatio = VREF_CLOCK_DIVIDE_RATIO(n),			\
		},									\
	};										\
											\
DEVICE_DT_INST_DEFINE(0, regulator_mspm0_vref_init, NULL, &data_##n,			\
		      &config_##n, POST_KERNEL,						\
		      CONFIG_REGULATOR_MSPM0_VREF_INIT_PRIORITY, &mspm0_vref_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_MSPM0_VREF_DEFINE)
