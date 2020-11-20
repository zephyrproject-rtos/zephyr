/*
 * Copyright (c) 2018 Endre Karlson <endre.karlson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32L0 family processors.
 *
 * Based on reference manual:
 *   STM32L0X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32L0_SOC_H_
#define _STM32L0_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32l0xx.h>

#include <st_stm32_dt.h>

#include <stm32l0xx_ll_system.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32l0xx_ll_exti.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32l0xx_ll_utils.h>
#include <stm32l0xx_ll_bus.h>
#include <stm32l0xx_ll_rcc.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L0_SOC_H_ */
