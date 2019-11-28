/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32h7xx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef CONFIG_STM32H7_DUAL_CORE

#define LL_HSEM_ID_0   (0U) /* HW semaphore 0 */
#define LL_HSEM_MASK_0 (1 << LL_HSEM_ID_0)
#define LL_HSEM_ID_1   (1U) /* HW semaphore 1 */
#define LL_HSEM_MASK_1 (1 << LL_HSEM_ID_1)

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

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H7_ */
