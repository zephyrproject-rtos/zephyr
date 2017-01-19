/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32F103 family processors.
 *
 * Based on reference manual:
 *   STM32L4x1, STM32L4x2, STM32L431xx STM32L443xx STM32L433xx, STM32L4x5,
 *   STM32l4x6 advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32L4X_SOC_H_
#define _STM32L4X_SOC_H_

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <device.h>
#include <stm32l4xx.h>

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#include "soc_irq.h"

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32l4xx_ll_usart.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L4X_SOC_H_ */
