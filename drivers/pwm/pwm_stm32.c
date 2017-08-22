/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <board.h>
#include <pwm.h>
#include <device.h>
#include <kernel.h>
#include <init.h>

#include <clock_control/stm32_clock_control.h>

#include "pwm_stm32.h"

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct pwm_stm32_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct pwm_stm32_data * const)(dev)->driver_data)
#define PWM_STRUCT(dev)					\
	((TIM_TypeDef *)(DEV_CFG(dev))->pwm_base)

#define CHANNEL_LENGTH 4

static u32_t __get_tim_clk(u32_t bus_clk,
			      clock_control_subsys_t *sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	u32_t tim_clk, apb_psc;

	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_APB1_PRESCALER;
	} else {
		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}

	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1) {
		tim_clk = bus_clk;
	} else	{
		tim_clk = 2 * bus_clk;
	}

	return tim_clk;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 *
 * return 0, or negative errno code
 */
static int pwm_stm32_pin_set(struct device *dev, u32_t pwm,
			     u32_t period_cycles, u32_t pulse_cycles)
{
	struct pwm_stm32_data *data = DEV_DATA(dev);
	TIM_HandleTypeDef *TimerHandle = &data->hpwm;
	TIM_OC_InitTypeDef sConfig;
	u32_t channel;
	bool counter_32b;

	if (period_cycles == 0 || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	/* configure channel */
	channel = (pwm - 1)*CHANNEL_LENGTH;

	if (!IS_TIM_INSTANCE(PWM_STRUCT(dev)) ||
		!IS_TIM_CHANNELS(channel)) {
		return -ENOTSUP;
	}

#ifdef CONFIG_SOC_SERIES_STM32F1X
	/* FIXME: IS_TIM_32B_COUNTER_INSTANCE not available on
	 * SMT32F1 Cube HAL since all timer counters are 16 bits
	 */
	counter_32b = 0;
#else
	counter_32b = IS_TIM_32B_COUNTER_INSTANCE(PWM_STRUCT(dev));
#endif

	if (!counter_32b && (period_cycles > 0xFFFF)) {
		/* 16 bits counter does not support requested period
		 * You might want to adapt PWM output clock to adjust
		 * cycle durations to fit requested period into 16 bits
		 * counter
		 */
		return -ENOTSUP;
	}

	/* Configure Timer IP */
	TimerHandle->Instance = PWM_STRUCT(dev);
	TimerHandle->Init.Prescaler = data->pwm_prescaler;
	TimerHandle->Init.ClockDivision = 0;
	TimerHandle->Init.CounterMode = TIM_COUNTERMODE_UP;
	TimerHandle->Init.RepetitionCounter = 0;

	/* Set period value */
	TimerHandle->Init.Period = period_cycles;

	HAL_TIM_PWM_Init(TimerHandle);

	/* Configure PWM channel */
	sConfig.OCMode       = TIM_OCMODE_PWM1;
	sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
	sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;

	/* Set the pulse value */
	sConfig.Pulse = pulse_cycles;

	HAL_TIM_PWM_ConfigChannel(TimerHandle, &sConfig, channel);

	return HAL_TIM_PWM_Start(TimerHandle, channel);
}

/*
 * Get the clock rate (cycles per second) for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number
 * cycles: Pointer to the memory to store clock rate (cycles per second)
 *
 * return 0, or negative errno code
 */
static int pwm_stm32_get_cycles_per_sec(struct device *dev, u32_t pwm,
					u64_t *cycles)
{
	const struct pwm_stm32_config *cfg = DEV_CFG(dev);
	struct pwm_stm32_data *data = DEV_DATA(dev);
	u32_t bus_clk, tim_clk;

	if (cycles == NULL) {
		return -EINVAL;
	}

	/* Timer clock depends on APB prescaler */
	clock_control_get_rate(data->clock,
			(clock_control_subsys_t *)&cfg->pclken, &bus_clk);

	tim_clk = __get_tim_clk(bus_clk,
			(clock_control_subsys_t *)&cfg->pclken);

	*cycles = (u64_t)(tim_clk / (data->pwm_prescaler + 1));

	return 0;
}

static const struct pwm_driver_api pwm_stm32_drv_api_funcs = {
	.pin_set = pwm_stm32_pin_set,
	.get_cycles_per_sec = pwm_stm32_get_cycles_per_sec,
};


static inline void __pwm_stm32_get_clock(struct device *dev)
{
	struct pwm_stm32_data *data = DEV_DATA(dev);
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}


static int pwm_stm32_init(struct device *dev)
{
	const struct pwm_stm32_config *config = DEV_CFG(dev);
	struct pwm_stm32_data *data = DEV_DATA(dev);

	__pwm_stm32_get_clock(dev);

	/* enable clock */
	clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken);

	return 0;
}


#ifdef CONFIG_PWM_STM32_1
static struct pwm_stm32_data pwm_stm32_dev_data_1 = {
	/* Default case */
	.pwm_prescaler = 10000,
};

static const struct pwm_stm32_config pwm_stm32_dev_cfg_1 = {
	.pwm_base = TIM1_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB2,
		    .enr = LL_APB2_GRP1_PERIPH_TIM1 },
};

DEVICE_AND_API_INIT(pwm_stm32_1, CONFIG_PWM_STM32_1_DEV_NAME,
		    pwm_stm32_init,
		    &pwm_stm32_dev_data_1, &pwm_stm32_dev_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_stm32_drv_api_funcs);
#endif /* CONFIG_PWM_STM32_1 */


#ifdef CONFIG_PWM_STM32_2
static struct pwm_stm32_data pwm_stm32_dev_data_2 = {
	/* Default case */
	.pwm_prescaler = 0,
};

static const struct pwm_stm32_config pwm_stm32_dev_cfg_2 = {
	.pwm_base = TIM2_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr = LL_APB1_GRP1_PERIPH_TIM2 },
};

DEVICE_AND_API_INIT(pwm_stm32_2, CONFIG_PWM_STM32_2_DEV_NAME,
		    pwm_stm32_init,
		    &pwm_stm32_dev_data_2, &pwm_stm32_dev_cfg_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_stm32_drv_api_funcs);
#endif /* CONFIG_PWM_STM32_2 */


#ifdef CONFIG_PWM_STM32_3
static struct pwm_stm32_data pwm_stm32_dev_data_3 = {
	/* Default case */
	.pwm_prescaler = 10000,
};

static const struct pwm_stm32_config pwm_stm32_dev_cfg_3 = {
	.pwm_base = TIM3_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr = LL_APB1_GRP1_PERIPH_TIM3 },
};

DEVICE_AND_API_INIT(pwm_stm32_2, CONFIG_PWM_STM32_3_DEV_NAME,
		    pwm_stm32_init,
		    &pwm_stm32_dev_data_3, &pwm_stm32_dev_cfg_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_stm32_drv_api_funcs);
#endif /* CONFIG_PWM_STM32_3 */
