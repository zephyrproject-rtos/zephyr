/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_lgpt_pwm

#include <stdint.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>

#include <inc/hw_gptimer.h>
#include <inc/hw_memmap.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc35xx_lgpt
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define GPTIMER_O_CXCC(index)  (GPTIMER_O_C0CC + ((index) * 4))
#define GPTIMER_O_CXCFG(index) (GPTIMER_O_C0CFG + ((index) * 4))
#define TI_CC35XX_CNTR_START   0x01
#define TI_CC35XX_CNTR_STOP    0x00

struct pwm_cc35xx_config {
	const uint32_t base;
	const struct pinctrl_dev_config *pcfg;
	uint32_t freq;
	uint32_t prescale;
	uint8_t channel;
};

static int pwm_cc35xx_set_cycles(const struct device *dev, uint32_t channel, uint32_t period,
				 uint32_t pulse, pwm_flags_t flags)
{
	const struct pwm_cc35xx_config *config = dev->config;
	uint32_t chcfg = GPTIMER_C1CFG_OUT0_EN << channel | 0xa;
	uint32_t reg_ctl_val = sys_read32(config->base + GPTIMER_O_CTL);

	ARG_UNUSED(flags);

	if (channel >= 3) {
		LOG_ERR("Invalid channel %u, must be 0-2", channel);
		return -EINVAL;
	}

	LOG_DBG("set cycles period[%x] pulse[%x]", period, pulse);

	sys_write32(period, config->base + GPTIMER_O_TGT);
	sys_write32(pulse, config->base + GPTIMER_O_CXCC(channel));
	sys_write32(chcfg, config->base + GPTIMER_O_CXCFG(channel));
	reg_ctl_val &= ~GPTIMER_CTL_MODE_M;
	reg_ctl_val |= GPTIMER_CTL_MODE_UP_PER;
	sys_write32(reg_ctl_val, config->base + GPTIMER_O_CTL);
	sys_write32(TI_CC35XX_CNTR_START, config->base + GPTIMER_O_STARTCFG);

	return 0;
}

static int pwm_cc35xx_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	const struct pwm_cc35xx_config *config = dev->config;

	ARG_UNUSED(channel);
	*cycles = config->freq / (config->prescale + 1);

	return 0;
}

static DEVICE_API(pwm, pwm_cc35xx_api) = {
	.set_cycles = pwm_cc35xx_set_cycles,
	.get_cycles_per_sec = pwm_cc35xx_get_cycles_per_sec,
};

#define DT_TIMER(n)           DT_INST_PARENT(n)
#define DT_TIMER_BASE_ADDR(n) (DT_REG_ADDR(DT_TIMER(n)))

#define PWM_CC35XX_INIT_FUNC(n)                                                                    \
	static int pwm_cc35xx_init_##n(const struct device *dev)                                   \
	{                                                                                          \
		const struct pwm_cc35xx_config *config = dev->config;                              \
		int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                \
		if (ret < 0) {                                                                     \
			LOG_ERR("[ERR] failed to setup PWM pinctrl");                              \
			return ret;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}

#define PWM_CC35XX_INIT(n)                                                                         \
	PWM_CC35XX_INIT_FUNC(n);                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, n, CONFIG_PWM_LOG_LEVEL);                           \
                                                                                                   \
	static const struct pwm_cc35xx_config cc35xx_pwm_config_##n = {                            \
		.base = DT_TIMER_BASE_ADDR(n),                                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.freq = DT_PROP(DT_PATH(gptimer_clock), clock_frequency),                          \
		.prescale = DT_PROP(DT_INST_PARENT(n), clk_prescale),                              \
		.channel = n,                                                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &pwm_cc35xx_init_##n, NULL, NULL, &cc35xx_pwm_config_##n,         \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_cc35xx_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_CC35XX_INIT);
