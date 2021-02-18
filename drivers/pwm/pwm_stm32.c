/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_pwm

#include <errno.h>

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_tim.h>
#include <drivers/pwm.h>
#include <device.h>
#include <kernel.h>
#include <init.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_stm32, CONFIG_PWM_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

/** PWM data. */
struct pwm_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
};

/** PWM configuration. */
struct pwm_stm32_config {
	/** Timer instance. */
	TIM_TypeDef *timer;
	/** Prescaler. */
	uint32_t prescaler;
	/** Clock configuration. */
	struct stm32_pclken pclken;
	/** pinctrl configurations. */
	const struct soc_gpio_pinctrl *pinctrl;
	/** Number of pinctrl configurations. */
	size_t pinctrl_len;
};

/** Series F3, F7, G0, G4, H7, L4, MP1 and WB have up to 6 channels, others up
 *  to 4.
 */
#if defined(CONFIG_SOC_SERIES_STM32F3X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32F7X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32G0X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32G4X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32H7X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32L4X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32MP1X) ||				       \
	defined(CONFIG_SOC_SERIES_STM32WBX)
#define TIMER_HAS_6CH 1
#else
#define TIMER_HAS_6CH 0
#endif

/** Maximum number of timer channels. */
#if TIMER_HAS_6CH
#define TIMER_MAX_CH 6u
#else
#define TIMER_MAX_CH 4u
#endif

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2,
	LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4,
#if TIMER_HAS_6CH
	LL_TIM_CHANNEL_CH5, LL_TIM_CHANNEL_CH6
#endif
};

/** Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *,
						     uint32_t) = {
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2,
	LL_TIM_OC_SetCompareCH3, LL_TIM_OC_SetCompareCH4,
#if TIMER_HAS_6CH
	LL_TIM_OC_SetCompareCH5, LL_TIM_OC_SetCompareCH6
#endif
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
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		return LL_TIM_OCPOLARITY_HIGH;
	}

	return LL_TIM_OCPOLARITY_LOW;
}

/**
 * Obtain timer clock speed.
 *
 * @param pclken  Timer clock control subsystem.
 * @param tim_clk Where computed timer clock will be stored.
 *
 * @return 0 on success, error code otherwise.
 */
static int get_tim_clk(const struct stm32_pclken *pclken, uint32_t *tim_clk)
{
	int r;
	const struct device *clk;
	uint32_t bus_clk, apb_psc;

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	r = clock_control_get_rate(clk, (clock_control_subsys_t *)pclken,
				   &bus_clk);
	if (r < 0) {
		return r;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE1;
	} else {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE2;
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_APB1_PRESCALER;
	}
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}
#endif
#endif

#if defined(RCC_DCKCFGR_TIMPRE) || defined(RCC_DCKCFGR1_TIMPRE) || \
	defined(RCC_CFGR_TIMPRE)
	/*
	 * There are certain series (some F4, F7 and H7) that have the TIMPRE
	 * bit to control the clock frequency of all the timers connected to
	 * APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to HCLK. This threshold value depends on TIMPRE setting
	 * (2 if TIMPRE=0, 4 if TIMPRE=1). Above threshold, timer clock is set
	 * to a multiple of the APB domain clock PCLK{1,2} (2 if TIMPRE=0, 4 if
	 * TIMPRE=1).
	 */

	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		/* TIMPRE = 0 */
		if (apb_psc <= 2u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 2u;
		}
	} else {
		/* TIMPRE = 1 */
		if (apb_psc <= 4u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 4u;
		}
	}
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1u) {
		*tim_clk = bus_clk;
	} else {
		*tim_clk = bus_clk * 2u;
	}
#endif

	return 0;
}

static int pwm_stm32_pin_set(const struct device *dev, uint32_t pwm,
			     uint32_t period_cycles, uint32_t pulse_cycles,
			     pwm_flags_t flags)
{
	const struct pwm_stm32_config *cfg = dev->config;

	uint32_t channel;

	if (pwm < 1u || pwm > TIMER_MAX_CH) {
		LOG_ERR("Invalid channel (%d)", pwm);
		return -EINVAL;
	}

	if (pulse_cycles > period_cycles) {
		LOG_ERR("Invalid combination of pulse and period cycles");
		return -EINVAL;
	}

	/*
	 * Non 32-bit timers count from 0 up to the value in the ARR register
	 * (16-bit). Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->timer) &&
	    (period_cycles > UINT16_MAX + 1)) {
		return -ENOTSUP;
	}

	channel = ch2ll[pwm - 1u];

	if (period_cycles == 0u) {
		LL_TIM_CC_DisableChannel(cfg->timer, channel);
		return 0;
	}

	if (!LL_TIM_CC_IsEnabledChannel(cfg->timer, channel)) {
		LL_TIM_OC_InitTypeDef oc_init;

		LL_TIM_OC_StructInit(&oc_init);

		oc_init.OCMode = LL_TIM_OCMODE_PWM1;
		oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
		oc_init.CompareValue = pulse_cycles;
		oc_init.OCPolarity = get_polarity(flags);

		if (LL_TIM_OC_Init(cfg->timer, channel, &oc_init) != SUCCESS) {
			LOG_ERR("Could not initialize timer channel output");
			return -EIO;
		}

		LL_TIM_EnableARRPreload(cfg->timer);
		LL_TIM_OC_EnablePreload(cfg->timer, channel);
		LL_TIM_SetAutoReload(cfg->timer, period_cycles - 1u);
		LL_TIM_GenerateEvent_UPDATE(cfg->timer);
	} else {
		LL_TIM_OC_SetPolarity(cfg->timer, channel, get_polarity(flags));
		set_timer_compare[pwm - 1u](cfg->timer, pulse_cycles);
		LL_TIM_SetAutoReload(cfg->timer, period_cycles - 1u);
	}

	return 0;
}

static int pwm_stm32_get_cycles_per_sec(const struct device *dev,
					uint32_t pwm,
					uint64_t *cycles)
{
	struct pwm_stm32_data *data = dev->data;
	const struct pwm_stm32_config *cfg = dev->config;

	*cycles = (uint64_t)(data->tim_clk / (cfg->prescaler + 1));

	return 0;
}

static const struct pwm_driver_api pwm_stm32_driver_api = {
	.pin_set = pwm_stm32_pin_set,
	.get_cycles_per_sec = pwm_stm32_get_cycles_per_sec,
};

static int pwm_stm32_init(const struct device *dev)
{
	struct pwm_stm32_data *data = dev->data;
	const struct pwm_stm32_config *cfg = dev->config;

	int r;
	const struct device *clk;
	LL_TIM_InitTypeDef init;

	/* enable clock and store its speed */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	r = clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}

	r = get_tim_clk(&cfg->pclken, &data->tim_clk);
	if (r < 0) {
		LOG_ERR("Could not obtain timer clock (%d)", r);
		return r;
	}

	/* configure pinmux */
	r = stm32_dt_pinctrl_configure(cfg->pinctrl,
				       cfg->pinctrl_len,
				       (uint32_t)cfg->timer);
	if (r < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", r);
		return r;
	}

	/* initialize timer */
	LL_TIM_StructInit(&init);

	init.Prescaler = cfg->prescaler;
	init.CounterMode = LL_TIM_COUNTERMODE_UP;
	init.Autoreload = 0u;
	init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

	if (LL_TIM_Init(cfg->timer, &init) != SUCCESS) {
		LOG_ERR("Could not initialize timer");
		return -EIO;
	}

#ifndef CONFIG_SOC_SERIES_STM32L0X
	/* enable outputs and counter */
	if (IS_TIM_BREAK_INSTANCE(cfg->timer)) {
		LL_TIM_EnableAllOutputs(cfg->timer);
	}
#endif

	LL_TIM_EnableCounter(cfg->timer);

	return 0;
}

#define DT_INST_CLK(index, inst)                                               \
	{                                                                      \
		.bus = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bus),     \
		.enr = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bits)     \
	}

#define PWM_DEVICE_INIT(index)                                                 \
	static struct pwm_stm32_data pwm_stm32_data_##index;                   \
									       \
	static const struct soc_gpio_pinctrl pwm_pins_##index[] =	       \
		ST_STM32_DT_INST_PINCTRL(index, 0);			       \
									       \
	static const struct pwm_stm32_config pwm_stm32_config_##index = {      \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(                           \
			DT_PARENT(DT_DRV_INST(index))),                        \
		.prescaler = DT_INST_PROP(index, st_prescaler),                \
		.pclken = DT_INST_CLK(index, timer),                           \
		.pinctrl = pwm_pins_##index,                                   \
		.pinctrl_len = ARRAY_SIZE(pwm_pins_##index),                   \
	};                                                                     \
									       \
	DEVICE_DT_INST_DEFINE(index, &pwm_stm32_init, device_pm_control_nop,   \
			    &pwm_stm32_data_##index,                           \
			    &pwm_stm32_config_##index, POST_KERNEL,            \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                \
			    &pwm_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
