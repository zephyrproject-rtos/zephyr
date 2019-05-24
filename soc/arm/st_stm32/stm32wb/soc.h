/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32WB family processors.
 *
 * Based on reference manual:
 *   TODO: Provide reference when known
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32WBX_SOC_H_
#define _STM32WBX_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32wbxx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef CONFIG_GPIO_STM32
#include <stm32wbxx_ll_gpio.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32wbxx_ll_usart.h>
#include <stm32wbxx_ll_lpuart.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32wbxx_ll_utils.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBX_SOC_H_ */
