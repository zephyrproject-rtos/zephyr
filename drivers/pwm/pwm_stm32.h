/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	uint32_t pwm_base;
	/* clock subsystem driving this peripheral */
#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32L4X)
	clock_control_subsys_t clock_subsys;
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_pclken pclken;
#endif
};

/** Runtime driver data */
struct pwm_stm32_data {
	/* PWM peripheral handler */
	TIM_HandleTypeDef hpwm;
	/* Prescaler for PWM output clock
	 * Value used to divide the TIM clock.
	 * Min = 0x0000U, Max = 0xFFFFU
	 */
	uint32_t pwm_prescaler;
	/* clock device */
	struct device *clock;
};

#ifdef __cplusplus
}
#endif

#endif /* __PWM_STM32_H__ */
