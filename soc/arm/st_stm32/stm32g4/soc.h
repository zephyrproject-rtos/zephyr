/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32G4 family processors.
 *
 * Based on reference manual:
 *   STM32G4xx advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32G4_SOC_H_
#define _STM32G4_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32g4xx.h>

/* Add include for DTS generated information */
#include <st_stm32_dt.h>

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32g4xx_ll_utils.h>
#include <stm32g4xx_ll_bus.h>
#include <stm32g4xx_ll_rcc.h>
#include <stm32g4xx_ll_system.h>
#include <stm32g4xx_ll_pwr.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_EXTI_STM32
#include <stm32g4xx_ll_exti.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <stm32g4xx_ll_gpio.h>
#endif

#ifdef CONFIG_SPI_STM32
#include <stm32g4xx_ll_spi.h>
#endif

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32g4xx_ll_rtc.h>
#include <stm32g4xx_ll_exti.h>
#include <stm32g4xx_ll_pwr.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32G4_SOC_H_ */
