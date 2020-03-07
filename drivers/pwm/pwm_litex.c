/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/pwm.h>
#include <zephyr/types.h>

#define REG_EN_ENABLE             0x1
#define REG_EN_DISABLE            0x0

/* PWM device in LiteX has only one channel */
#define NUMBER_OF_CHANNELS        1

struct pwm_litex_cfg {
	u32_t reg_en_size;
	u32_t reg_width_size;
	u32_t reg_period_size;
	volatile u32_t *reg_en;
	volatile u32_t *reg_width;
	volatile u32_t *reg_period;
};

#define GET_PWM_CFG(dev)				       \
	((const struct pwm_litex_cfg *) dev->config->config_info)

static void litex_set_reg(volatile u32_t *reg, u32_t reg_size, u32_t val)
{
	u32_t shifted_data;
	volatile u32_t *reg_addr;

	for (int i = 0; i < reg_size; ++i) {
		shifted_data = val >> ((reg_size - i - 1) * 8);
		reg_addr = ((volatile u32_t *) reg) + i;
		*(reg_addr) = shifted_data;
	}
}

int pwm_litex_init(struct device *dev)
{
	const struct pwm_litex_cfg *cfg = GET_PWM_CFG(dev);

	litex_set_reg(cfg->reg_en, cfg->reg_en_size, REG_EN_ENABLE);
	return 0;
}

int pwm_litex_pin_set(struct device *dev, u32_t pwm, u32_t period_cycles,
		      u32_t pulse_cycles, pwm_flags_t flags)
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

int pwm_litex_get_cycles_per_sec(struct device *dev, u32_t pwm, u64_t *cycles)
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
		  (volatile u32_t *)                                           \
			DT_INST_##n##_LITEX_PWM_ENABLE_BASE_ADDRESS,           \
		.reg_en_size = DT_INST_##n##_LITEX_PWM_ENABLE_SIZE / 4,        \
		.reg_width =						       \
		  (volatile u32_t *)                                           \
			DT_INST_##n##_LITEX_PWM_WIDTH_BASE_ADDRESS,            \
		.reg_width_size = DT_INST_##n##_LITEX_PWM_WIDTH_SIZE / 4,      \
		.reg_period  =						       \
		  (volatile u32_t *)                                           \
			DT_INST_##n##_LITEX_PWM_PERIOD_BASE_ADDRESS,           \
		.reg_period_size = DT_INST_##n##_LITEX_PWM_PERIOD_SIZE / 4,    \
	};								       \
									       \
	DEVICE_AND_API_INIT(pwm_##n,					       \
			    DT_INST_##n##_LITEX_PWM_LABEL,		       \
			    pwm_litex_init,				       \
			    NULL,					       \
			    &pwm_litex_cfg_##n,				       \
			    POST_KERNEL,				       \
			    CONFIG_PWM_LITEX_INIT_PRIORITY,		       \
			    &pwm_litex_driver_api			       \
			   )

#ifdef DT_INST_0_LITEX_PWM_LABEL
PWM_LITEX_INIT(0);
#endif

#ifdef DT_INST_1_LITEX_PWM_LABEL
PWM_LITEX_INIT(1);
#endif

#ifdef DT_INST_2_LITEX_PWM_LABEL
PWM_LITEX_INIT(2);
#endif

#ifdef DT_INST_3_LITEX_PWM_LABEL
PWM_LITEX_INIT(3);
#endif

#ifdef DT_INST_4_LITEX_PWM_LABEL
PWM_LITEX_INIT(4);
#endif

#ifdef DT_INST_5_LITEX_PWM_LABEL
PWM_LITEX_INIT(5);
#endif

#ifdef DT_INST_6_LITEX_PWM_LABEL
PWM_LITEX_INIT(6);
#endif

#ifdef DT_INST_7_LITEX_PWM_LABEL
PWM_LITEX_INIT(7);
#endif

#ifdef DT_INST_8_LITEX_PWM_LABEL
PWM_LITEX_INIT(8);
#endif
