/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ST STM32F7 family processors.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 2.3: Memory Map
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <stm32f7xx.h>

#include "soc_irq.h"

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f7xx_ll_utils.h>
#include <stm32f7xx_ll_bus.h>
#include <stm32f7xx_ll_rcc.h>
#include <stm32f7xx_ll_system.h>
#include <stm32f7xx_ll_spi.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f7xx_ll_usart.h>
#endif

#ifdef CONFIG_IWDG_STM32
#include <stm32f7xx_ll_iwdg.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM3274_SOC_H_ */
