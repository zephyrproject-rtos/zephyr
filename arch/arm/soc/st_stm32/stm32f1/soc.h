/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32F103 family processors.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 3.3: Memory Map
 */


#ifndef _STM32F1_SOC_H_
#define _STM32F1_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <random/rand32.h>

#include <stm32f1xx.h>


#include "soc_irq.h"

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f1xx_ll_usart.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f1xx_ll_utils.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_rcc.h>
#include <stm32f1xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_I2C
#include <stm32f1xx_ll_i2c.h>
#endif

#ifdef CONFIG_IWDG_STM32
#include <stm32f1xx_ll_iwdg.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F1_SOC_H_ */
