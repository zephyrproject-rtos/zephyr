/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2023 Nobleo Technology
 * Copyright (c) 2025 CodeWrights GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_lptim_pwm

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_lptim.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/dt-bindings/pwm/stm32_pwm.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_stm32_lptim, CONFIG_PWM_LOG_LEVEL);

/** PWM data. */
struct pwm_stm32_lptim_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
	/* Reset controller device configuration */
	const struct reset_dt_spec reset;
};

/** PWM configuration. */
struct pwm_stm32_lptim_config {
	LPTIM_TypeDef *timer;
	uint32_t prescaler;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
};

/** Maximum number of timer channels */
#define TIMER_MAX_CH 2u

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_LPTIM_CHANNEL_CH1,
	LL_LPTIM_CHANNEL_CH2,
};

/** Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(LPTIM_TypeDef *, uint32_t) = {
	LL_LPTIM_OC_SetCompareCH1,
	LL_LPTIM_OC_SetCompareCH2,
};

/**
 * Obtain LL polarity from PWM flags.
 *
 * @param flags PWM flags.
 *
 * @return LL polarity.
 */
static uint32_t get_polarity(pwm_flags_t flags)
{
	/* PWM_POLARITY_NORMAL (active-high) pulses correspond to
	 * LL_LPTIM_OUTPUT_POLARITY_INVERSE and vice versa.
	 */
	return (flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL
		       ? LL_LPTIM_OUTPUT_POLARITY_INVERSE
		       : LL_LPTIM_OUTPUT_POLARITY_REGULAR;
}

static int pwm_stm32_lptim_set_cycles(const struct device *dev, uint32_t channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	LPTIM_TypeDef *timer = cfg->timer;
	uint32_t polarity;
	uint32_t ll_channel;

	if (channel < 1u || channel > TIMER_MAX_CH) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	/*
	 * Timers count from 0 up to the value in the ARR register (16-bit).
	 * Thus period_cycles cannot be greater than UINT16_MAX.
	 */
	if (period_cycles > UINT16_MAX) {
		LOG_ERR("Cannot set PWM output, value exceeds 16-bit timer limit.");
		return -ENOTSUP;
	}

	ll_channel = ch2ll[channel - 1u];

	if (period_cycles == 0u) {
		LL_LPTIM_CC_DisableChannel(timer, ll_channel);
		return 0;
	}

	polarity = get_polarity(flags);

	if (pulse_cycles == 0U) {
		/* hardware does not support generating 0% duty cycle
		 * workaround by setting to 100% and inverting the polarity
		 */
		polarity = (polarity == LL_LPTIM_OUTPUT_POLARITY_REGULAR)
				   ? LL_LPTIM_OUTPUT_POLARITY_INVERSE
				   : LL_LPTIM_OUTPUT_POLARITY_REGULAR;
		/* set to max to have 100% duty cycle (of inverse) */
		pulse_cycles = 0xffff;
	} else if (pulse_cycles == period_cycles) {
		/* set to max to have 100% duty cycle */
		pulse_cycles = 0xffff;
	} else {
		/* remove 1 pulse cycle, accounts for 1 extra low cycle */
		pulse_cycles -= 1U;
	}

	/* remove 1 period cycle, accounts for 1 extra low cycle */
	period_cycles -= 1U;

	LL_LPTIM_OC_SetPolarity(timer, ll_channel, polarity);
	set_timer_compare[channel - 1u](timer, pulse_cycles);
	LL_LPTIM_SetAutoReload(timer, period_cycles);

	if (!LL_LPTIM_CC_IsEnabledChannel(timer, ll_channel)) {
		LL_LPTIM_CC_SetChannelMode(timer, ll_channel, LL_LPTIM_CCMODE_OUTPUT_PWM);
		LL_LPTIM_CC_EnableChannel(timer, ll_channel);
	}

	return 0;
}

static int pwm_stm32_lptim_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	struct pwm_stm32_lptim_data *data = dev->data;
	const struct pwm_stm32_lptim_config *cfg = dev->config;

	*cycles = (uint64_t)(data->tim_clk / cfg->prescaler);

	return 0;
}

static DEVICE_API(pwm, pwm_stm32_lptim_driver_api) = {
	.set_cycles = pwm_stm32_lptim_set_cycles,
	.get_cycles_per_sec = pwm_stm32_lptim_get_cycles_per_sec,
};

static int pwm_stm32_lptim_init(const struct device *dev)
{
	struct pwm_stm32_lptim_data *data = dev->data;
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	LPTIM_TypeDef *timer = cfg->timer;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int r;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable clock and store its speed */
	r = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}

	/* Enable Timer clock source */
	r = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
	if (r != 0) {
		LOG_ERR("Could not configure clock (%d)", r);
		return r;
	}

	r = clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[1], &data->tim_clk);
	if (r < 0) {
		LOG_ERR("Timer clock rate get error (%d)", r);
		return r;
	}

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&data->reset);

	/* Configure pinmux */
	r = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", r);
		return r;
	}

	/* Initialize timer */
	LL_LPTIM_SetClockSource(timer, LL_LPTIM_CLK_SOURCE_INTERNAL);
	LL_LPTIM_SetPrescaler(timer, __CLZ(__RBIT(cfg->prescaler)) << LPTIM_CFGR_PRESC_Pos);
	LL_LPTIM_SetAutoReload(timer, 0U);
	LL_LPTIM_SetUpdateMode(timer, LL_LPTIM_UPDATE_MODE_ENDOFPERIOD);

	LL_LPTIM_Enable(timer);

	/* Start the LPTIM counter in continuous mode */
	LL_LPTIM_StartCounter(timer, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

	return 0;
}

#define PWM(index) DT_DRV_INST(index)

#define PWM_DEVICE_INIT(index)                                                                     \
	static struct pwm_stm32_lptim_data pwm_stm32_lptim_data_##index = {                        \
		.reset = RESET_DT_SPEC_GET(PWM(index)),                                            \
	};                                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	BUILD_ASSERT(DT_NUM_CLOCKS(PWM(index)) == 2, "Timer clock source is required");            \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_CLOCKS(PWM(index));           \
                                                                                                   \
	static const struct pwm_stm32_lptim_config pwm_stm32_lptim_config_##index = {              \
		.timer = (LPTIM_TypeDef *)DT_REG_ADDR(PWM(index)),                                 \
		.prescaler = DT_PROP(PWM(index), st_prescaler),                                    \
		.pclken = pclken_##index,                                                          \
		.pclk_len = DT_NUM_CLOCKS(PWM(index)),                                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &pwm_stm32_lptim_init, NULL, &pwm_stm32_lptim_data_##index,   \
			      &pwm_stm32_lptim_config_##index, POST_KERNEL,                        \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_stm32_lptim_driver_api);
DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
