/*
 * Copyright (c) 2024 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8801_altctrl

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/dt-bindings/mfd/mfd_it8801_altctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_it8801_altctrl, LOG_LEVEL_ERR);

struct mfd_altfunc_config {
	/*  gpiocr register */
	uint8_t reg_gpiocr;
};

int mfd_it8801_configure_pins(const struct i2c_dt_spec *i2c_dev, const struct device *dev,
			      uint8_t pin, uint8_t func)
{
	const struct mfd_altfunc_config *config = dev->config;
	int ret;
	uint8_t reg_gpiocr = config->reg_gpiocr + pin;
	uint8_t alt_val;

	switch (func) {
	case IT8801_ALT_FUNC_1:
		/* Func1: Default GPIO setting */
		alt_val = IT8801_GPIOAFS_FUN1;
		break;
	case IT8801_ALT_FUNC_2:
		/* Func2: KSO or PWM setting */
		alt_val = IT8801_GPIOAFS_FUN2;
		break;
	case IT8801_ALT_FUNC_3:
		/* Func3: PWM setting */
		alt_val = IT8801_GPIOAFS_FUN3;
		break;
	case IT8801_ALT_DEFAULT:
		alt_val = IT8801_GPIOAFS_FUN1;
		break;
	default:
		LOG_ERR("This function is not supported.");
		return -EINVAL;
	}

	/* Common settings for alternate function. */
	ret = i2c_reg_update_byte_dt(i2c_dev, reg_gpiocr, GENMASK(7, 6), alt_val << 6);
	if (ret != 0) {
		LOG_ERR("Failed to update gpiocr (ret %d)", ret);
		return ret;
	}

	return 0;
}

#define MFD_IT8801_ALTCTRL_INIT(inst)                                                              \
	static const struct mfd_altfunc_config it8801_mfd_alt_cfg_##inst = {                       \
		.reg_gpiocr = DT_INST_REG_ADDR(inst),                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &it8801_mfd_alt_cfg_##inst, POST_KERNEL,     \
			      CONFIG_MFD_INIT_PRIORITY, NULL);
DT_INST_FOREACH_STATUS_OKAY(MFD_IT8801_ALTCTRL_INIT)
