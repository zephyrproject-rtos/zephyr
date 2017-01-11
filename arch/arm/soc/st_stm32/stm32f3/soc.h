/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
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
 * @file SoC configuration macros for the STM32F3 family processors.
 *
 * Based on reference manual:
 *   STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC,
 *   STM32F398xE advanced ARM ® -based MCUs
 *   STM32F37xx advanced ARM ® -based MCUs
 *
 * Chapter 3.3: Memory organization
 */


#ifndef _STM32F3_SOC_H_
#define _STM32F3_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#include <stm32f3xx.h>

/* IO pin functions */
enum stm32f3x_pin_config_mode {
	STM32F3X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE = 0,
	STM32F3X_PIN_CONFIG_BIAS_PULL_UP,
	STM32F3X_PIN_CONFIG_BIAS_PULL_DOWN,
	STM32F3X_PIN_CONFIG_ANALOG,
	STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN,
	STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL,
	STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PU,
	STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PD,
	STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PU,
	STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PD,
	STM32F3X_PIN_CONFIG_AF,
};

#include "soc_irq.h"

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f3xx_ll_usart.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F3_SOC_H_ */
