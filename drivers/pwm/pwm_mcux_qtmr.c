/*
 * Copyright (c) 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qtmr_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <fsl_qtmr.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_qtmr, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT TMR_CNTR_COUNT

struct pwm_mcux_qtmr_config {
	TMR_Type *base;
	uint32_t prescaler;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct pwm_mcux_qtmr_data {
	struct k_mutex lock;
};

static int mcux_qtmr_pwm_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	uint32_t periodCount, highCount, lowCount;
	uint16_t reg;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	/* Counter values to generate a PWM signal */
	periodCount = period_cycles;
	highCount = pulse_cycles;
	lowCount = period_cycles - pulse_cycles;

	if (highCount > 0U) {
		highCount -= 1U;
	}
	if (lowCount > 0U) {
		lowCount -= 1U;
	}

	if ((highCount > 0xFFFFU) || (lowCount > 0xFFFFU)) {
		/* This should not be a 16-bit overflow value. If it is, change to a larger divider
		 * for clock source.
		 */
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Set OFLAG pin for output mode and force out a low on the pin */
	config->base->CHANNEL[channel].SCTRL |= (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK);

	QTMR_StopTimer(config->base, channel);

	/* Setup the compare registers for PWM output */
	config->base->CHANNEL[channel].COMP1 = (uint16_t)lowCount;
	config->base->CHANNEL[channel].COMP2 = (uint16_t)highCount;

	/* Setup the pre-load registers for PWM output */
	config->base->CHANNEL[channel].CMPLD1 = (uint16_t)lowCount;
	config->base->CHANNEL[channel].CMPLD2 = (uint16_t)highCount;

	reg = config->base->CHANNEL[channel].CSCTRL;
	/* Setup the compare load control for COMP1 and COMP2.
	 * Load COMP1 when CSCTRL[TCF2] is asserted, load COMP2 when CSCTRL[TCF1] is asserted
	 */
	reg &= (uint16_t)(~(TMR_CSCTRL_CL1_MASK | TMR_CSCTRL_CL2_MASK));
	reg |= (TMR_CSCTRL_CL1(kQTMR_LoadOnComp2) | TMR_CSCTRL_CL2(kQTMR_LoadOnComp1));
	config->base->CHANNEL[channel].CSCTRL = reg;

	reg = config->base->CHANNEL[channel].CTRL;
	reg &= ~(uint16_t)TMR_CTRL_OUTMODE_MASK;
	if (highCount == periodCount) {
		/* Set OFLAG output on compare */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_SetOnCompare));
	} else if (periodCount == 0U) {
		/* Clear OFLAG output on compare */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ClearOnCompare));
	} else {
		/* Toggle OFLAG output using alternating compare register */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ToggleOnAltCompareReg));
	}

	config->base->CHANNEL[channel].CTRL = reg;

	QTMR_StartTimer(config->base, channel, kQTMR_PriSrcRiseEdge);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_qtmr_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					    uint64_t *cycles)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	uint32_t clock_freq;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}

	*cycles = clock_freq / config->prescaler;

	return 0;
}

static int mcux_qtmr_pwm_init(const struct device *dev)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	qtmr_config_t qtmr_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	k_mutex_init(&data->lock);

	QTMR_GetDefaultConfig(&qtmr_config);
	qtmr_config.primarySource = kQTMR_ClockDivide_1 + (31 - __builtin_clz(config->prescaler));

	for (int i = 0; i < CHANNEL_COUNT; i++) {
		QTMR_Init(config->base, i, &qtmr_config);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_mcux_qtmr_driver_api) = {
	.set_cycles = mcux_qtmr_pwm_set_cycles,
	.get_cycles_per_sec = mcux_qtmr_pwm_get_cycles_per_sec,
};

#define PWM_MCUX_QTMR_DEVICE_INIT(n)                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct pwm_mcux_qtmr_data pwm_mcux_qtmr_data_##n;                                   \
                                                                                                   \
	static const struct pwm_mcux_qtmr_config pwm_mcux_qtmr_config_##n = {                      \
		.base = (TMR_Type *)DT_INST_REG_ADDR(n),                                           \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_qtmr_pwm_init, NULL, &pwm_mcux_qtmr_data_##n,                \
			      &pwm_mcux_qtmr_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &pwm_mcux_qtmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_QTMR_DEVICE_INIT)
