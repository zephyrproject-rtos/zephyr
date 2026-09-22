/*
 * Copyright (c) 2024 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8801_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ite_it8801, CONFIG_PWM_LOG_LEVEL);

struct it8801_pwm_map {
	uint8_t pushpull_en;
};
const static struct it8801_pwm_map it8801_pwm_gpio_map[] = {
	[1] = {.pushpull_en = BIT(0)}, [2] = {.pushpull_en = BIT(1)}, [3] = {.pushpull_en = BIT(2)},
	[4] = {.pushpull_en = BIT(3)}, [7] = {.pushpull_en = BIT(4)}, [8] = {.pushpull_en = BIT(5)},
	[9] = {.pushpull_en = BIT(6)},
};

struct it8801_mfd_pwm_altctrl_cfg {
	/* GPIO control device structure */
	const struct device *gpiocr;
	/* GPIO control pin */
	uint8_t pin;
	/* GPIO function select */
	uint8_t alt_func;
};

struct pwm_it8801_config {
	/* IT8801 controller dev */
	const struct device *mfd;
	/* I2C device for the MFD parent */
	const struct i2c_dt_spec i2c_dev;
	/* PWM alternate configuration */
	const struct it8801_mfd_pwm_altctrl_cfg *altctrl;
	int mfdctrl_len;
	int channel;
	/* PWM mode control register */
	uint8_t reg_mcr;
	/* PWM duty cycle register */
	uint8_t reg_dcr;
	/* PWM prescale LSB register */
	uint8_t reg_prslr;
	/* PWM prescale MSB register */
	uint8_t reg_prsmr;
};

static void pwm_enable(const struct device *dev, int enabled)
{
	const struct pwm_it8801_config *config = dev->config;
	int ret;

	if (enabled) {
		ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_mcr,
					     IT8801_PWMMCR_MCR_MASK, IT8801_PWMMCR_MCR_BLINKING);
	} else {
		ret = i2c_reg_update_byte_dt(&config->i2c_dev, config->reg_mcr,
					     IT8801_PWMMCR_MCR_MASK, 0);
	}
	if (ret != 0) {
		LOG_ERR("Failed to enable pwm (ret %d)", ret);
		return;
	}
}

static int pwm_it8801_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	*cycles = (uint64_t)PWM_IT8801_FREQ;

	return 0;
}

static int pwm_it8801_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	ARG_UNUSED(channel);

	const struct pwm_it8801_config *config = dev->config;
	int ch = config->channel, ret;
	uint8_t duty, mask;

	/* Enable PWM output push-pull */
	if (flags & PWM_IT8801_PUSH_PULL) {
		mask = it8801_pwm_gpio_map[ch].pushpull_en;

		ret = i2c_reg_update_byte_dt(&config->i2c_dev, IT8801_REG_PWMODDSR, mask, mask);
		if (ret != 0) {
			LOG_ERR("Failed to set push-pull (ret %d)", ret);
			return ret;
		}
	}

	/* Set PWM channel duty cycle */
	if (pulse_cycles == 0) {
		duty = 0;
	} else {
		duty = pulse_cycles * 256 / period_cycles - 1;
	}
	LOG_DBG("IT8801 pwm duty cycles = %d", duty);

	ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_dcr, duty);
	if (ret != 0) {
		LOG_ERR("Failed to set cycles (ret %d)", ret);
		return ret;
	}

	/* PWM channel clock source not gating */
	pwm_enable(dev, 1);

	return 0;
}

static int pwm_it8801_init(const struct device *dev)
{
	const struct pwm_it8801_config *config = dev->config;
	int ret;

	/* Verify multi-function parent is ready */
	if (!device_is_ready(config->mfd)) {
		LOG_ERR("(pwm)%s is not ready", config->mfd->name);
		return -ENODEV;
	}

	/* PWM channel clock source gating before configuring */
	pwm_enable(dev, 0);

	for (int i = 0; i < config->mfdctrl_len; i++) {
		/* Switching the pin to PWM alternate function */
		ret = mfd_it8801_configure_pins(&config->i2c_dev, config->altctrl[i].gpiocr,
						config->altctrl[i].pin,
						config->altctrl[i].alt_func);
		if (ret != 0) {
			LOG_ERR("Failed to configure pins (ret %d)", ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(pwm, pwm_it8801_api) = {
	.set_cycles = pwm_it8801_set_cycles,
	.get_cycles_per_sec = pwm_it8801_get_cycles_per_sec,
};

#define PWM_IT8801_INIT(inst)                                                                      \
	static const struct it8801_mfd_pwm_altctrl_cfg                                             \
		it8801_pwm_altctrl##inst[IT8801_DT_INST_MFDCTRL_LEN(inst)] =                       \
			IT8801_DT_MFD_ITEMS_LIST(inst);                                            \
	static const struct pwm_it8801_config pwm_it8801_cfg_##inst = {                            \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.i2c_dev = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                  \
		.altctrl = it8801_pwm_altctrl##inst,                                               \
		.mfdctrl_len = IT8801_DT_INST_MFDCTRL_LEN(inst),                                   \
		.channel = DT_INST_PROP(inst, channel),                                            \
		.reg_mcr = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                       \
		.reg_dcr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                       \
		.reg_prslr = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                     \
		.reg_prsmr = DT_INST_REG_ADDR_BY_IDX(inst, 3),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &pwm_it8801_init, NULL, NULL, &pwm_it8801_cfg_##inst,          \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, &pwm_it8801_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_IT8801_INIT)
