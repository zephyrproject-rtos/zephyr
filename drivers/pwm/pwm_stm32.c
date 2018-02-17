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
	}
#ifndef CONFIG_SOC_SERIES_STM32F0X
	else {
		apb_psc = CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}
#endif

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

	/*
	 * The timer counts from 0 up to the value in the ARR register (16-bit).
	 * Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!counter_32b && (period_cycles > 0x10000)) {
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
	TimerHandle->Init.Period = period_cycles - 1;

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

#define PWM_DEVICE_INIT_STM32(n, apb, grp, prescaler)			  \
	static struct pwm_stm32_data pwm_stm32_dev_data_ ## n = {	  \
		/* Default case */					  \
		.pwm_prescaler = prescaler,				  \
	};								  \
									  \
	static const struct pwm_stm32_config pwm_stm32_dev_cfg_ ## n = {  \
		.pwm_base = TIM ## n ## _BASE,				  \
		.pclken = { .bus = STM32_CLOCK_BUS_ ## apb,		  \
			    .enr = LL_##apb##_##grp##_PERIPH_TIM##n },	  \
	};								  \
									  \
	DEVICE_AND_API_INIT(pwm_stm32_ ## n,				  \
			    CONFIG_PWM_STM32_ ## n ## _DEV_NAME,	  \
			    pwm_stm32_init,				  \
			    &pwm_stm32_dev_data_ ## n,			  \
			    &pwm_stm32_dev_cfg_ ## n,			  \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
			    &pwm_stm32_drv_api_funcs);

#ifdef CONFIG_PWM_STM32_1
/* 16-bit advanced-control timer */
#ifdef CONFIG_SOC_SERIES_STM32F0X
PWM_DEVICE_INIT_STM32(1, APB1, GRP2, CONFIG_PWM_STM32_1_PRESCALER)
#else
PWM_DEVICE_INIT_STM32(1, APB2, GRP1, CONFIG_PWM_STM32_1_PRESCALER)
#endif /*CONFIG_SOC_SERIES_STM32F0X */
#endif /* CONFIG_PWM_STM32_1 */

#ifdef CONFIG_PWM_STM32_2
/* 32-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(2, APB1, GRP1, CONFIG_PWM_STM32_2_PRESCALER)
#endif /* CONFIG_PWM_STM32_2 */

#ifdef CONFIG_PWM_STM32_3
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(3, APB1, GRP1, CONFIG_PWM_STM32_3_PRESCALER)
#endif /* CONFIG_PWM_STM32_3 */

#ifdef CONFIG_PWM_STM32_4
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(4, APB1, GRP1, CONFIG_PWM_STM32_4_PRESCALER)
#endif /* CONFIG_PWM_STM32_4 */

#ifdef CONFIG_PWM_STM32_5
/* 32-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(5, APB1, GRP1, CONFIG_PWM_STM32_5_PRESCALER)
#endif /* CONFIG_PWM_STM32_5 */

#ifdef CONFIG_PWM_STM32_6
/* 16-bit basic timer */
PWM_DEVICE_INIT_STM32(6, APB1, GRP1, CONFIG_PWM_STM32_6_PRESCALER)
#endif /* CONFIG_PWM_STM32_6 */

#ifdef CONFIG_PWM_STM32_7
/* 16-bit basic timer */
PWM_DEVICE_INIT_STM32(7, APB1, GRP1, CONFIG_PWM_STM32_7_PRESCALER)
#endif /* CONFIG_PWM_STM32_7 */

#ifdef CONFIG_PWM_STM32_8
/* 16-bit advanced-control timer */
PWM_DEVICE_INIT_STM32(8, APB2, GRP1, CONFIG_PWM_STM32_8_PRESCALER)
#endif /* CONFIG_PWM_STM32_8 */

#ifdef CONFIG_PWM_STM32_9
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(9, APB2, GRP1, CONFIG_PWM_STM32_9_PRESCALER)
#endif /* CONFIG_PWM_STM32_9 */

#ifdef CONFIG_PWM_STM32_10
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(10, APB2, GRP1, CONFIG_PWM_STM32_10_PRESCALER)
#endif /* CONFIG_PWM_STM32_10 */

#ifdef CONFIG_PWM_STM32_11
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(11, APB2, GRP1, CONFIG_PWM_STM32_11_PRESCALER)
#endif /* CONFIG_PWM_STM32_11 */

#ifdef CONFIG_PWM_STM32_12
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(12, APB1, GRP1, CONFIG_PWM_STM32_12_PRESCALER)
#endif /* CONFIG_PWM_STM32_12 */

#ifdef CONFIG_PWM_STM32_13
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(13, APB1, GRP1, CONFIG_PWM_STM32_13_PRESCALER)
#endif /* CONFIG_PWM_STM32_13 */

#ifdef CONFIG_PWM_STM32_14
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(14, APB1, GRP1, CONFIG_PWM_STM32_14_PRESCALER)
#endif /* CONFIG_PWM_STM32_14 */

#ifdef CONFIG_PWM_STM32_15
/* 16-bit general-purpose timer */
#ifdef CONFIG_SOC_SERIES_STM32F0X
PWM_DEVICE_INIT_STM32(15, APB1, GRP2, CONFIG_PWM_STM32_15_PRESCALER)
#else
PWM_DEVICE_INIT_STM32(15, APB2, GRP1, CONFIG_PWM_STM32_15_PRESCALER)
#endif /* CONFIG_SOC_SERIES_STM32F0X */
#endif /* CONFIG_PWM_STM32_15 */

#ifdef CONFIG_PWM_STM32_16
/* 16-bit general-purpose timer */
#ifdef CONFIG_SOC_SERIES_STM32F0X
PWM_DEVICE_INIT_STM32(16, APB1, GRP2, CONFIG_PWM_STM32_16_PRESCALER)
#else
PWM_DEVICE_INIT_STM32(16, APB2, GRP1, CONFIG_PWM_STM32_16_PRESCALER)
#endif /* CONFIG_SOC_SERIES_STM32F0X */
#endif /* CONFIG_PWM_STM32_16 */

#ifdef CONFIG_PWM_STM32_17
/* 16-bit general-purpose timer */
#ifdef CONFIG_SOC_SERIES_STM32F0X
PWM_DEVICE_INIT_STM32(17, APB1, GRP2, CONFIG_PWM_STM32_17_PRESCALER)
#else
PWM_DEVICE_INIT_STM32(17, APB2, GRP1, CONFIG_PWM_STM32_17_PRESCALER)
#endif /* CONFIG_SOC_SERIES_STM32F0X */
#endif /* CONFIG_PWM_STM32_17 */

#ifdef CONFIG_PWM_STM32_18
/* 16-bit advanced timer */
PWM_DEVICE_INIT_STM32(18, APB1, GRP1, CONFIG_PWM_STM32_18_PRESCALER)
#endif /* CONFIG_PWM_STM32_18 */

#ifdef CONFIG_PWM_STM32_19
/* 16-bit general-purpose timer */
PWM_DEVICE_INIT_STM32(19, APB2, GRP1, CONFIG_PWM_STM32_19_PRESCALER)
#endif /* CONFIG_PWM_STM32_19 */

#ifdef CONFIG_PWM_STM32_20
/* 16-bit advanced timer */
PWM_DEVICE_INIT_STM32(20, APB2, GRP1, CONFIG_PWM_STM32_20_PRESCALER)
#endif /* CONFIG_PWM_STM32_20 */
