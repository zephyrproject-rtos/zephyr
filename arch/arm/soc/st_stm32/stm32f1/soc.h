/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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
 * @file SoC configuration macros for the STM32F103 family processors.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 3.3: Memory Map
 */


#ifndef _STM32F1_SOC_H_
#define _STM32F1_SOC_H_

/* peripherals start address */
#define PERIPH_BASE           0x40000000

/* use naming consistent with STMF10x Peripherals Library */
#define APB1PERIPH_BASE        PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x10000)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x20000)

/* UART */
#define USART1_ADDR           (APB2PERIPH_BASE + 0x3800)
#define USART2_ADDR           (APB1PERIPH_BASE + 0x4400)
#define USART3_ADDR           (APB1PERIPH_BASE + 0x4800)

/* Reset and Clock Control */
#define RCC_BASE              (AHBPERIPH_BASE + 0x1000)

#define GPIO_REG_SIZE         0x400
#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x0C00)
#define GPIOC_BASE            (APB2PERIPH_BASE + 0x1000)
#define GPIOD_BASE            (APB2PERIPH_BASE + 0x1400)
#define GPIOE_BASE            (APB2PERIPH_BASE + 0x1800)
#define GPIOF_BASE            (APB2PERIPH_BASE + 0x1C00)
#define GPIOG_BASE            (APB2PERIPH_BASE + 0x2000)
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

/* EXTI */
#define EXTI_BASE            (APB2PERIPH_BASE + 0x0400)

/* AFIO */
#define AFIO_BASE            (APB2PERIPH_BASE + 0x0000)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

/* IO pin functions */
enum stm32f10x_pin_config_mode {
	STM32F10X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
	STM32F10X_PIN_CONFIG_BIAS_PULL_UP,
	STM32F10X_PIN_CONFIG_BIAS_PULL_DOWN,
	STM32F10X_PIN_CONFIG_ANALOG,
	STM32F10X_PIN_CONFIG_DRIVE_OPEN_DRAIN,
	STM32F10X_PIN_CONFIG_DRIVE_PUSH_PULL,
	STM32F10X_PIN_CONFIG_AF_PUSH_PULL,
	STM32F10X_PIN_CONFIG_AF_OPEN_DRAIN,
};

#include "soc_irq.h"

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F1_SOC_H_ */
