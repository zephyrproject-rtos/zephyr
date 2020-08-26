/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32h7xx.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#ifdef CONFIG_STM32H7_DUAL_CORE
#include <stm32h7xx_ll_hsem.h>

#ifdef CONFIG_CPU_CORTEX_M4

#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_pwr.h>
#include <stm32h7xx_ll_cortex.h>

#endif /* CONFIG_CPU_CORTEX_M4 */

#endif /* CONFIG_STM32H7_DUAL_CORE */

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_rcc.h>
#include <stm32h7xx_ll_pwr.h>
#include <stm32h7xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_EXTI_STM32
#include <stm32h7xx_ll_exti.h>
#endif /* CONFIG_EXTI_STM32 */

#ifdef CONFIG_GPIO_STM32
#include <stm32h7xx_ll_gpio.h>
#include <stm32h7xx_ll_system.h>
#endif /* CONFIG_GPIO_STM32 */

#ifdef CONFIG_WWDG_STM32
#include <stm32h7xx_ll_wwdg.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32h7xx_ll_usart.h>
#endif /* CONFIG_SERIAL_HAS_DRIVER */

#if defined(CONFIG_HWINFO_STM32) || defined(CONFIG_CLOCK_CONTROL_STM32_CUBE)
#include <stm32h7xx_ll_utils.h>
#endif

#ifdef CONFIG_COUNTER_RTC_STM32
#include <stm32h7xx_ll_rtc.h>
#include <stm32h7xx_ll_exti.h>
#include <stm32h7xx_ll_pwr.h>
#endif /* CONFIG_COUNTER_RTC_STM32 */

#ifdef CONFIG_I2C_STM32
#include <stm32h7xx_ll_i2c.h>
#endif /* CONFIG_I2C_STM32 */

#ifdef CONFIG_ADC_STM32
#include <stm32h7xx_ll_adc.h>
#endif /* CONFIG_ADC_STM32 */

#ifdef CONFIG_PWM_STM32
#include <stm32h7xx_ll_tim.h>
#endif /* CONFIG_PWM_STM32 */

#ifdef CONFIG_ENTROPY_STM32_RNG
#include <stm32h7xx_ll_rng.h>
#endif /* CONFIG_ENTROPY_STM32_RNG */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H7_ */
