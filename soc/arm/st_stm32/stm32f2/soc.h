/*
 * Copyright (c) 2018 qianfan Zhao <qianfanguijin@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the stm32f2 family processors.
 *
 * Based on reference manual:
 *   stm32f2X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32F2_SOC_H_
#define _STM32F2_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <stm32f2xx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f2xx_ll_utils.h>
#include <stm32f2xx_ll_bus.h>
#include <stm32f2xx_ll_rcc.h>
#include <stm32f2xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f2xx_ll_usart.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F2_SOC_H_ */
