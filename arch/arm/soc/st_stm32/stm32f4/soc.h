/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file SoC configuration macros for the ST STM32F4 family processors.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.3: Memory Map
 */

#ifndef _STM32F4_SOC_H_
#define _STM32F4_SOC_H_

/* peripherals start address */
#define PERIPH_BASE           0x40000000

/* use naming consistent with STMF4 Peripherals Library */
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x10000)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x20000)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000)

/* UART */
#define USART1_ADDR           (APB2PERIPH_BASE + 0x1000)
#define USART2_ADDR           (APB1PERIPH_BASE + 0x4400)
#define USART6_ADDR           (APB2PERIPH_BASE + 0x1400)

/* Reset and Clock Control */
#define RCC_BASE              (AHB1PERIPH_BASE + 0x3800)

#define GPIO_REG_SIZE         0x400
#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE            (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE            (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE            (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE            (AHB1PERIPH_BASE + 0x1000)
#define GPIOH_BASE            (AHB1PERIPH_BASE + 0x1C00)
#define GPIOI_BASE            (AHB1PERIPH_BASE + 0x2000)
#define GPIOJ_BASE            (AHB1PERIPH_BASE + 0x2400)
#define GPIOK_BASE            (AHB1PERIPH_BASE + 0x2800)

/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

/* EXTI */
#define EXTI_BASE             (APB2PERIPH_BASE + 0x3C00)

/* IWDG */
#define IWDG_BASE             (APB1PERIPH_BASE + 0x3000)

/* FLASH */
#define FLASH_R_BASE          (AHB1PERIPH_BASE + 0x3C00)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#include "soc_irq.h"

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F4_SOC_H_ */
