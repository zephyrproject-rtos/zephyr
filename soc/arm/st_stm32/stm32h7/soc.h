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

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_rcc.h>
#include <stm32h7xx_ll_pwr.h>
#include <stm32h7xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_EXTI_STM32
#include <stm32h7xx_ll_exti.h>
#endif /* CONFIG_EXTI_STM32 */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H7_ */
