/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32L4 family processors.
 *
 * Based on reference manual:
 *   STM32L4x1, STM32L4x2, STM32L431xx STM32L443xx STM32L433xx, STM32L4x5,
 *   STM32l4x6 advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32L4X_SOC_H_
#define _STM32L4X_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32l4xx.h>

/* Add include for DTS generated information */
#include <st_stm32_dt.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32l4xx_ll_exti.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32l4xx_ll_utils.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>
#include <stm32l4xx_ll_pwr.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32l4xx_ll_rtc.h>
#include <stm32l4xx_ll_exti.h>
#include <stm32l4xx_ll_pwr.h>
#endif

#ifdef CONFIG_USB
/* Required to remove USB transceiver supply isolation */
#include <stm32l4xx_ll_pwr.h>
#endif /* CONFIG_USB */

#ifdef CONFIG_GPIO_STM32
#include <stm32l4xx_ll_gpio.h>
/* Required to enable VDDio2 for port G */
#include <stm32l4xx_ll_pwr.h>
#endif

#ifdef CONFIG_DMA_STM32
#include <stm32l4xx_ll_dma.h>
#endif

#ifdef CONFIG_STM32_LPTIM_TIMER
#include <stm32l4xx_ll_lptim.h>
#include <stm32l4xx_ll_system.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L4X_SOC_H_ */
