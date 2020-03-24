/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_pwm

#include <errno.h>

#include <soc.h>
#include <drivers/pwm.h>
#include <device.h>
#include <kernel.h>
#include <init.h>

#include <drivers/clock_control/stm32_clock_control.h>

#include "pwm_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_stm32);

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
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
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
	if (apb_psc == 1U) {
		tim_clk = bus_clk;
	} else	{
		tim_clk = bus_clk * 2U;
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
			     u32_t period_cycles, u32_t pulse_cycles,
			     pwm_flags_t flags)
{
	struct pwm_stm32_data *data = DEV_DATA(dev);
	TIM_HandleTypeDef *TimerHandle = &data->hpwm;
	TIM_OC_InitTypeDef sConfig;
	u32_t channel;
	bool counter_32b;

	if (period_cycles == 0U || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
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
	if (clock_control_get_rate(data->clock,
			(clock_control_subsys_t *)&cfg->pclken, &bus_clk) < 0) {
		LOG_ERR("Failed call clock_control_get_rate");
		return -EIO;
	}

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
	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	return 0;
}

#define PWM_DEVICE_INIT_STM32(index)					\
	static struct pwm_stm32_data pwm_stm32_dev_data_##index = {	\
		/* Default case */					\
		.pwm_prescaler = DT_INST_PROP(index, st_prescaler),\
	};								\
									\
	static const struct pwm_stm32_config pwm_stm32_dev_cfg_##index = {\
		.pwm_base = DT_REG_ADDR(DT_INST(index, st_stm32_timers)),\
		.pclken = {						\
			.bus = DT_CLOCKS_CELL(DT_INST(index, st_stm32_timers), bus),\
			.enr = DT_CLOCKS_CELL(DT_INST(index, st_stm32_timers), bits)\
		},\
	};								\
									\
	DEVICE_AND_API_INIT(pwm_stm32_##index,				\
			    DT_INST_LABEL(index),	\
			    pwm_stm32_init,				\
			    &pwm_stm32_dev_data_##index,		\
			    &pwm_stm32_dev_cfg_##index,			\
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
			    &pwm_stm32_drv_api_funcs)

#if DT_HAS_DRV_INST(0)
PWM_DEVICE_INIT_STM32(0);
#endif /* DT_HAS_DRV_INST(0) */

#if DT_HAS_DRV_INST(1)
PWM_DEVICE_INIT_STM32(1);
#endif /* DT_HAS_DRV_INST(1) */

#if DT_HAS_DRV_INST(2)
PWM_DEVICE_INIT_STM32(2);
#endif /* DT_HAS_DRV_INST(2) */

#if DT_HAS_DRV_INST(3)
PWM_DEVICE_INIT_STM32(3);
#endif /* DT_HAS_DRV_INST(3) */

#if DT_HAS_DRV_INST(4)
PWM_DEVICE_INIT_STM32(4);
#endif /* DT_HAS_DRV_INST(4) */

#if DT_HAS_DRV_INST(5)
PWM_DEVICE_INIT_STM32(5);
#endif /* DT_HAS_DRV_INST(5) */

#if DT_HAS_DRV_INST(6)
PWM_DEVICE_INIT_STM32(6);
#endif /* DT_HAS_DRV_INST(6) */

#if DT_HAS_DRV_INST(7)
PWM_DEVICE_INIT_STM32(7);
#endif /* DT_HAS_DRV_INST(7) */

#if DT_HAS_DRV_INST(8)
PWM_DEVICE_INIT_STM32(8);
#endif /* DT_HAS_DRV_INST(8) */

#if DT_HAS_DRV_INST(9)
PWM_DEVICE_INIT_STM32(9);
#endif /* DT_HAS_DRV_INST(9) */

#if DT_HAS_DRV_INST(10)
PWM_DEVICE_INIT_STM32(10);
#endif /* DT_HAS_DRV_INST(10) */

#if DT_HAS_DRV_INST(11)
PWM_DEVICE_INIT_STM32(11);
#endif /* DT_HAS_DRV_INST(11) */

#if DT_HAS_DRV_INST(12)
PWM_DEVICE_INIT_STM32(12);
#endif /* DT_HAS_DRV_INST(12) */

#if DT_HAS_DRV_INST(13)
PWM_DEVICE_INIT_STM32(13);
#endif /* DT_HAS_DRV_INST(13) */

#if DT_HAS_DRV_INST(14)
PWM_DEVICE_INIT_STM32(14);
#endif /* DT_HAS_DRV_INST(14) */

#if DT_HAS_DRV_INST(15)
PWM_DEVICE_INIT_STM32(15);
#endif /* DT_HAS_DRV_INST(15) */

#if DT_HAS_DRV_INST(16)
PWM_DEVICE_INIT_STM32(16);
#endif /* DT_HAS_DRV_INST(16) */

#if DT_HAS_DRV_INST(17)
PWM_DEVICE_INIT_STM32(17);
#endif /* DT_HAS_DRV_INST(17) */

#if DT_HAS_DRV_INST(18)
PWM_DEVICE_INIT_STM32(18);
#endif /* DT_HAS_DRV_INST(18) */

#if DT_HAS_DRV_INST(19)
PWM_DEVICE_INIT_STM32(19);
#endif /* DT_HAS_DRV_INST(19) */
