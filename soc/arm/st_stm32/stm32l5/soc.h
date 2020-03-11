/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32L5 family processors.
 *
 * Based on reference manual:
 *   STM32L5X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32L5_SOC_H_
#define _STM32L5_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32l5xx.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32l5xx_ll_utils.h>
#include <stm32l5xx_ll_bus.h>
#include <stm32l5xx_ll_rcc.h>
#include <stm32l5xx_ll_system.h>
#include <stm32l5xx_ll_pwr.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_EXTI_STM32
#include <stm32l5xx_ll_exti.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <stm32l5xx_ll_gpio.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32l5xx_ll_usart.h>
#include <stm32l5xx_ll_lpuart.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L5_SOC_H_ */
