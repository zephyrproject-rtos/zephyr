/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Shlomi Vaknin.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_timer_trigger

#include <errno.h>

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_tim.h>
#include <drivers/hwtrig.h>
#include <device.h>
#include <kernel.h>
#include <init.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stm32_timer_trigger, CONFIG_HWTRIG_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

/** HWTRIG data. */
struct hwtrig_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
};

/** HWTRIG configuration. */
struct hwtrig_stm32_config {
	/** Timer instance. */
	TIM_TypeDef *timer;
	/** Prescaler. */
	uint32_t prescaler;
	/** Clock configuration. */
	struct stm32_pclken pclken;
};

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

static int hwtrig_stm32_enable(const struct device *dev, void *arg)
{
	const struct hwtrig_stm32_config *cfg = dev->config;
	struct hwtrig_stm32_data *data = dev->data;

	uint32_t sampling_frequency = *((uint32_t *)arg);

	uint32_t cycles_per_sec = data->tim_clk / (cfg->prescaler + 1);
	uint32_t cycles_per_sample = cycles_per_sec / sampling_frequency;

	/*
	 * Non 32-bit timers count from 0 up to the value in the ARR register
	 * (16-bit). Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->timer) &&
	    (cycles_per_sample > UINT16_MAX + 1)) {
		return -ENOTSUP;
	}

	LL_TIM_EnableARRPreload(cfg->timer);
	LL_TIM_EnableARRPreload(cfg->timer);
	LL_TIM_SetAutoReload(cfg->timer, cycles_per_sample - 1u);
	LL_TIM_GenerateEvent_UPDATE(cfg->timer);

	return 0;
}

static const struct hwtrig_driver_api hwtrig_stm32_driver_api = {
	.enable = hwtrig_stm32_enable,
};

static int hwtrig_stm32_init(const struct device *dev)
{
	struct hwtrig_stm32_data *data = dev->data;
	const struct hwtrig_stm32_config *cfg = dev->config;

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

	LL_TIM_SetTriggerOutput(cfg->timer, LL_TIM_TRGO_UPDATE);

#ifndef CONFIG_SOC_SERIES_STM32L0X
	/* enable outputs and counter */
	if (IS_TIM_BREAK_INSTANCE(cfg->timer)) {
		LL_TIM_EnableAllOutputs(cfg->timer);
	}
#endif

	LL_TIM_EnableCounter(cfg->timer);

	return 0;
}

#define DT_INST_CLK(index, inst)                                              \
	{                                                                      \
		.bus = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bus),     \
		.enr = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bits)     \
	}

#define HWTRIG_DEVICE_INIT(index)                                             \
	static struct hwtrig_stm32_data hwtrig_stm32_data_##index = {			\
	};                   \
									       \
	static const struct hwtrig_stm32_config hwtrig_stm32_config_##index = {   \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(                           \
			DT_PARENT(DT_DRV_INST(index))),                        \
		.prescaler = DT_INST_PROP(index, st_prescaler),                \
		.pclken = DT_INST_CLK(index, timer),                           \
	};                                                                     \
									       \
	DEVICE_DT_INST_DEFINE(index, &hwtrig_stm32_init, device_pm_control_nop,   \
			    &hwtrig_stm32_data_##index,                           \
			    &hwtrig_stm32_config_##index, POST_KERNEL,            \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                \
			    &hwtrig_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HWTRIG_DEVICE_INIT)
