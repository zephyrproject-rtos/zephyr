/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32F0 family processors.
 *
 * Based on reference manual:
 *   STM32F030x4/x6/x8/xC,
 *   STM32F070x6/xB advanced ARM ® -based MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32F0_SOC_H_
#define _STM32F0_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <stm32f0xx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f0xx_ll_usart.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f0xx_ll_utils.h>
#include <stm32f0xx_ll_bus.h>
#include <stm32f0xx_ll_rcc.h>
#include <stm32f0xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_IWDG_STM32
#include <stm32f0xx_ll_iwdg.h>
#endif

#ifdef CONFIG_I2C_STM32_V2
#include <stm32f0xx_ll_i2c.h>
#endif

#ifdef CONFIG_SPI_STM32
#include <stm32f0xx_ll_spi.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F0_SOC_H_ */
