/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the STM32 PWM driver.
 */

#ifndef __PWM_STM32_H__
#define __PWM_STM32_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration data */
struct pwm_stm32_config {
	u32_t pwm_base;
	/* clock subsystem driving this peripheral */
	struct stm32_pclken pclken;
};

/** Runtime driver data */
struct pwm_stm32_data {
	/* PWM peripheral handler */
	TIM_HandleTypeDef hpwm;
	/* Prescaler for PWM output clock
	 * Value used to divide the TIM clock.
	 * Min = 0x0000U, Max = 0xFFFFU
	 */
	u32_t pwm_prescaler;
	/* clock device */
	struct device *clock;
};

#ifdef __cplusplus
}
#endif

#endif /* __PWM_STM32_H__ */
