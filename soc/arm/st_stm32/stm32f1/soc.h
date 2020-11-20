/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32F1 family processors.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 3.3: Memory Map
 */


#ifndef _STM32F1_SOC_H_
#define _STM32F1_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f1xx.h>

#include <st_stm32_dt.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32f1xx_ll_exti.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f1xx_ll_utils.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_rcc.h>
#include <stm32f1xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_GPIO_STM32
#include <stm32f1xx_ll_gpio.h>
#endif

#ifdef CONFIG_DMA_STM32
#include <stm32f1xx_ll_dma.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F1_SOC_H_ */
