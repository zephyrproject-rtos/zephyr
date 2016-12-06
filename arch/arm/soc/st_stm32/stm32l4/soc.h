/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
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
