/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_pwm

#include <device.h>
#include <drivers/pwm.h>
#include <zephyr/types.h>

#define REG_EN_ENABLE             0x1
#define REG_EN_DISABLE            0x0

/* PWM device in LiteX has only one channel */
#define NUMBER_OF_CHANNELS        1

struct pwm_litex_cfg {
	uint32_t reg_en_size;
	uint32_t reg_width_size;
	uint32_t reg_period_size;
	volatile uint32_t *reg_en;
	volatile uint32_t *reg_width;
	volatile uint32_t *reg_period;
};

#define GET_PWM_CFG(dev)				       \
	((const struct pwm_litex_cfg *) dev->config)

static void litex_set_reg(volatile uint32_t *reg, uint32_t reg_size, uint32_t val)
{
	uint32_t shifted_data;
	volatile uint32_t *reg_addr;

	for (int i = 0; i < reg_size; ++i) {
		shifted_data = val >> ((reg_size - i - 1) * 8);
		reg_addr = ((volatile uint32_t *) reg) + i;
		*(reg_addr) = shifted_data;
	}
}

int pwm_litex_init(const struct device *dev)
{
	const struct pwm_litex_cfg *cfg = GET_PWM_CFG(dev);

	litex_set_reg(cfg->reg_en, cfg->reg_en_size, REG_EN_ENABLE);
	return 0;
}

int pwm_litex_pin_set(const struct device *dev, uint32_t pwm,
		      uint32_t period_cycles,
		      uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_litex_cfg *cfg = GET_PWM_CFG(dev);

	if (pwm >= NUMBER_OF_CHANNELS) {
		return -EINVAL;
	}

	litex_set_reg(cfg->reg_en, cfg->reg_en_size, REG_EN_DISABLE);
	litex_set_reg(cfg->reg_width, cfg->reg_width_size, pulse_cycles);
	litex_set_reg(cfg->reg_period, cfg->reg_period_size, period_cycles);
	litex_set_reg(cfg->reg_en, cfg->reg_en_size, REG_EN_ENABLE);

	return 0;
}

int pwm_litex_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				 uint64_t *cycles)
{
	if (pwm >= NUMBER_OF_CHANNELS) {
		return -EINVAL;
	}

	*cycles = sys_clock_hw_cycles_per_sec();
	return 0;
}

static const struct pwm_driver_api pwm_litex_driver_api = {
	.pin_set             = pwm_litex_pin_set,
	.get_cycles_per_sec  = pwm_litex_get_cycles_per_sec,
};

/* Device Instantiation */

/* LiteX regisers use only first byte from 4-bytes register, that's why they
 * occupy larger space in memory. We need to know the size that is
 * actually used, that is why the register size from dts is divided by 4.
 */

#define PWM_LITEX_INIT(n)						       \
	static const struct pwm_litex_cfg pwm_litex_cfg_##n = {		       \
		.reg_en =						       \
		  (volatile uint32_t *)                                           \
			DT_INST_REG_ADDR_BY_NAME(n, enable),           \
		.reg_en_size = DT_INST_REG_SIZE_BY_NAME(n, enable) / 4,        \
		.reg_width =						       \
		  (volatile uint32_t *)                                           \
			DT_INST_REG_ADDR_BY_NAME(n, width),            \
		.reg_width_size = DT_INST_REG_SIZE_BY_NAME(n, width) / 4,      \
		.reg_period  =						       \
		  (volatile uint32_t *)                                           \
			DT_INST_REG_ADDR_BY_NAME(n, period),           \
		.reg_period_size = DT_INST_REG_SIZE_BY_NAME(n, period) / 4,    \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n,					       \
			    pwm_litex_init,				       \
			    device_pm_control_nop,			       \
			    NULL,					       \
			    &pwm_litex_cfg_##n,				       \
			    POST_KERNEL,				       \
			    CONFIG_PWM_LITEX_INIT_PRIORITY,		       \
			    &pwm_litex_driver_api			       \
			   );

DT_INST_FOREACH_STATUS_OKAY(PWM_LITEX_INIT)
