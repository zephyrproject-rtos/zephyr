/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_pwm

#include "pwm.h"
#include "clock.h"
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>


#define PWM_PCLK_SPEED      DT_INST_PROP(0, clock_frequency)
#define NUM_OF_CHANNELS     DT_INST_PROP(0, channels)

PINCTRL_DT_INST_DEFINE(0);

static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

/* API implementation: init */
static int pwm_b91_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t status = 0;
	uint8_t clk_32k_en = 0;
	uint32_t pwm_clk_div = 0;

	/* Calculate and check PWM clock divider */
	pwm_clk_div = sys_clk.pclk * 1000 * 1000 / PWM_PCLK_SPEED - 1;
	if (pwm_clk_div > 255) {
		return -EINVAL;
	}

	/* Set PWM Peripheral clock */
	pwm_set_clk((unsigned char) (pwm_clk_div & 0xFF));

	/* Set PWM 32k Channel clock if enabled */
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch0_enable) ? PWM_CLOCK_32K_CHN_PWM0 : 0;
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch1_enable) ? PWM_CLOCK_32K_CHN_PWM1 : 0;
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch2_enable) ? PWM_CLOCK_32K_CHN_PWM2 : 0;
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch3_enable) ? PWM_CLOCK_32K_CHN_PWM3 : 0;
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch4_enable) ? PWM_CLOCK_32K_CHN_PWM4 : 0;
	clk_32k_en |= DT_INST_PROP(0, clk32k_ch5_enable) ? PWM_CLOCK_32K_CHN_PWM5 : 0;
	pwm_32k_chn_en(clk_32k_en);

	/* Config PWM pins */
	status = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		return status;
	}

	return 0;
}

/* API implementation: set_cycles */
static int pwm_b91_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	ARG_UNUSED(dev);

	/* check pwm channel */
	if (channel >= NUM_OF_CHANNELS) {
		return -EINVAL;
	}

	/* check size of pulse and period (2 bytes) */
	if ((period_cycles > 0xFFFFu) ||
	    (pulse_cycles  > 0xFFFFu)) {
		return -EINVAL;
	}

	/* set polarity */
	if (flags & PWM_POLARITY_INVERTED) {
		pwm_invert_en(channel);
	} else {
		pwm_invert_dis(channel);
	}

	/* set pulse and period */
	pwm_set_tcmp(channel, pulse_cycles);
	pwm_set_tmax(channel, period_cycles);

	/* start pwm */
	pwm_start(channel);

	return 0;
}

/* API implementation: get_cycles_per_sec */
static int pwm_b91_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(dev);

	/* check pwm channel */
	if (channel >= NUM_OF_CHANNELS) {
		return -EINVAL;
	}

	if (
		((channel == 0u) && DT_INST_PROP(0, clk32k_ch0_enable)) ||
		((channel == 1u) && DT_INST_PROP(0, clk32k_ch1_enable)) ||
		((channel == 2u) && DT_INST_PROP(0, clk32k_ch2_enable)) ||
		((channel == 3u) && DT_INST_PROP(0, clk32k_ch3_enable)) ||
		((channel == 4u) && DT_INST_PROP(0, clk32k_ch4_enable)) ||
		((channel == 5u) && DT_INST_PROP(0, clk32k_ch5_enable))
		) {
		*cycles = 32000u;
	} else {
		*cycles = sys_clk.pclk * 1000 * 1000 / (reg_pwm_clkdiv + 1);
	}

	return 0;
}

/* PWM driver APIs structure */
static const struct pwm_driver_api pwm_b91_driver_api = {
	.set_cycles = pwm_b91_set_cycles,
	.get_cycles_per_sec = pwm_b91_get_cycles_per_sec,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported PWM instance");

/* PWM driver registration */
#define PWM_B91_INIT(n)							       \
									       \
	DEVICE_DT_INST_DEFINE(n, pwm_b91_init,				       \
			      NULL, NULL, NULL,				       \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &pwm_b91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_B91_INIT)
