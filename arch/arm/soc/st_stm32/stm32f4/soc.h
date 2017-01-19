/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
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

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#include <stm32f4xx.h>

/* IO pin functions */
enum stm32f4x_pin_config_mode {
	STM32F4X_PIN_CONFIG_DRIVE_PUSH_PULL,
	STM32F4X_PIN_CONFIG_DRIVE_PUSH_UP,
	STM32F4X_PIN_CONFIG_DRIVE_PUSH_DOWN,
	STM32F4X_PIN_CONFIG_DRIVE_OPEN_DRAIN,
	STM32F4X_PIN_CONFIG_DRIVE_OPEN_UP,
	STM32F4X_PIN_CONFIG_DRIVE_OPEN_DOWN,
	STM32F4X_PIN_CONFIG_AF_PUSH_PULL,
	STM32F4X_PIN_CONFIG_AF_PUSH_UP,
	STM32F4X_PIN_CONFIG_AF_PUSH_DOWN,
	STM32F4X_PIN_CONFIG_AF_OPEN_DRAIN,
	STM32F4X_PIN_CONFIG_AF_OPEN_UP,
	STM32F4X_PIN_CONFIG_AF_OPEN_DOWN,
	STM32F4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
	STM32F4X_PIN_CONFIG_BIAS_PULL_UP,
	STM32F4X_PIN_CONFIG_BIAS_PULL_DOWN,
	STM32F4X_PIN_CONFIG_ANALOG,
};

#include "soc_irq.h"

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F4_SOC_H_ */
