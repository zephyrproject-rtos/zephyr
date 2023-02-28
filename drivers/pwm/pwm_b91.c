/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_pwm

#include <pwm.h>
#include <clock.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>

struct pwm_b91_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t clock_frequency;
	uint8_t channels;
	uint8_t clk32k_ch_enable;
};

/* API implementation: init */
static int pwm_b91_init(const struct device *dev)
{
	const struct pwm_b91_config *config = dev->config;

	int status = 0;
	uint32_t pwm_clk_div;

	/* Calculate and check PWM clock divider */
	pwm_clk_div = sys_clk.pclk * 1000 * 1000 / config->clock_frequency - 1;
	if (pwm_clk_div > 255) {
		return -EINVAL;
	}

	/* Set PWM Peripheral clock */
	pwm_set_clk((unsigned char) (pwm_clk_div & 0xFF));

	#if DT_NODE_EXISTS(DT_PATH_INTERNAL(pwm_leds))
		/* Start PWM from device tree */
		static const struct pwm_dt_spec pwm_leds[] = {
			DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH_INTERNAL(pwm_leds),
				PWM_DT_SPEC_GET, (,))
		};

		for (size_t i = 0; !status && i < ARRAY_SIZE(pwm_leds); i++) {
			if (dev == pwm_leds[i].dev) {
				status = pwm_set(dev,
					pwm_leds[i].channel, pwm_leds[i].period, 0,
					pwm_leds[i].flags);
			}
		}
	#endif /* DT_NODE_EXISTS(DT_PATH_INTERNAL(pwm_leds)) */

	/* Config PWM pins */
	if (!status) {
		status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	}

	return status;
}

/* API implementation: set_cycles */
static int pwm_b91_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct pwm_b91_config *config = dev->config;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}

	/* check size of pulse and period (2 bytes) */
	if ((period_cycles > 0xFFFFu) ||
	    (pulse_cycles  > 0xFFFFu)) {
		return -EINVAL;
	}

	/* set polarity */
	if (flags & PWM_POLARITY_INVERTED) {
		pwm_set_polarity_en(channel);
	} else {
		pwm_set_polarity_dis(channel);
	}

	/* set pulse and period */
	pwm_set_tcmp(channel, pulse_cycles);
	pwm_set_tmax(channel, period_cycles);

	/* start pwm */
	pwm_start(channel);

	/* switch to 32K */
	if ((config->clk32k_ch_enable & BIT(channel)) != 0U) {
		pwm_32k_chn_en(BIT(channel));
	}

	return 0;
}

/* API implementation: get_cycles_per_sec */
static int pwm_b91_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	const struct pwm_b91_config *config = dev->config;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}

	if ((config->clk32k_ch_enable & BIT(channel)) != 0U) {
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

/* PWM driver registration */
#define PWM_B91_INIT(n)							       \
	PINCTRL_DT_INST_DEFINE(n);					       \
									       \
	static const struct pwm_b91_config config##n = {		       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		       \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),	       \
		.channels = DT_INST_PROP(n, channels),			       \
		.clk32k_ch_enable =					       \
			((DT_INST_PROP(n, clk32k_ch0_enable) << 0U) |	       \
			 (DT_INST_PROP(n, clk32k_ch1_enable) << 1U) |	       \
			 (DT_INST_PROP(n, clk32k_ch2_enable) << 2U) |	       \
			 (DT_INST_PROP(n, clk32k_ch3_enable) << 3U) |	       \
			 (DT_INST_PROP(n, clk32k_ch4_enable) << 4U) |	       \
			 (DT_INST_PROP(n, clk32k_ch5_enable) << 5U)),	       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, pwm_b91_init,				       \
			      NULL, NULL, &config##n,			       \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	       \
			      &pwm_b91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_B91_INIT)
