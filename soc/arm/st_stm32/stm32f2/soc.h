/*
 * Copyright (c) 2018 qianfan Zhao <qianfanguijin@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the stm32f2 family processors.
 *
 * Based on reference manual:
 *   stm32f2X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32F2_SOC_H_
#define _STM32F2_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f2xx.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32f2xx_ll_exti.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f2xx_ll_utils.h>
#include <stm32f2xx_ll_bus.h>
#include <stm32f2xx_ll_rcc.h>
#include <stm32f2xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f2xx_ll_usart.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <stm32f2xx_ll_gpio.h>
#endif

#ifdef CONFIG_IWDG_STM32
#include <stm32f2xx_ll_iwdg.h>
#endif

#ifdef CONFIG_WWDG_STM32
#include <stm32f2xx_ll_wwdg.h>
#endif

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32f2xx_ll_rtc.h>
#include <stm32f2xx_ll_exti.h>
#include <stm32f2xx_ll_pwr.h>
#endif

#ifdef CONFIG_ADC_STM32
#include <stm32f2xx_ll_adc.h>
#endif

#ifdef CONFIG_DAC_STM32
#include <stm32f2xx_ll_dac.h>
#endif

#ifdef CONFIG_DMA_STM32
#include <stm32f2xx_ll_dma.h>
#endif

#ifdef CONFIG_HWINFO_STM32
#include <stm32f2xx_ll_utils.h>
#endif

#ifdef CONFIG_PWM_STM32
#include <stm32f2xx_ll_tim.h>
#endif /* CONFIG_PWM_STM32 */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F2_SOC_H_ */
