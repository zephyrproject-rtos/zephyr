/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/types.h>

#include <soc.h>

#define REG_EN_ENABLE             0x1
#define REG_EN_DISABLE            0x0

/* PWM device in LiteX has only one channel */
#define NUMBER_OF_CHANNELS        1

struct pwm_litex_cfg {
	uint32_t reg_en;
	uint32_t reg_width;
	uint32_t reg_period;
};

int pwm_litex_init(const struct device *dev)
{
	const struct pwm_litex_cfg *cfg = dev->config;

	litex_write8(REG_EN_ENABLE, cfg->reg_en);
	return 0;
}

int pwm_litex_set_cycles(const struct device *dev, uint32_t channel,
			 uint32_t period_cycles,
			 uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_litex_cfg *cfg = dev->config;

	if (channel >= NUMBER_OF_CHANNELS) {
		return -EINVAL;
	}

	litex_write8(REG_EN_DISABLE, cfg->reg_en);
	litex_write32(pulse_cycles, cfg->reg_width);
	litex_write32(period_cycles, cfg->reg_period);
	litex_write8(REG_EN_ENABLE, cfg->reg_en);

	return 0;
}

int pwm_litex_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				 uint64_t *cycles)
{
	if (channel >= NUMBER_OF_CHANNELS) {
		return -EINVAL;
	}

	*cycles = sys_clock_hw_cycles_per_sec();
	return 0;
}

static DEVICE_API(pwm, pwm_litex_driver_api) = {
	.set_cycles = pwm_litex_set_cycles,
	.get_cycles_per_sec = pwm_litex_get_cycles_per_sec,
};

/* Device Instantiation */

#define PWM_LITEX_INIT(n)						       \
	static const struct pwm_litex_cfg pwm_litex_cfg_##n = {		       \
		.reg_en = DT_INST_REG_ADDR_BY_NAME(n, enable),		       \
		.reg_width = DT_INST_REG_ADDR_BY_NAME(n, width),	       \
		.reg_period = DT_INST_REG_ADDR_BY_NAME(n, period),	       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n,					       \
			    pwm_litex_init,				       \
			    NULL,					       \
			    NULL,					       \
			    &pwm_litex_cfg_##n,				       \
			    POST_KERNEL,				       \
			    CONFIG_PWM_LITEX_INIT_PRIORITY,		       \
			    &pwm_litex_driver_api			       \
			   );

DT_INST_FOREACH_STATUS_OKAY(PWM_LITEX_INIT)
