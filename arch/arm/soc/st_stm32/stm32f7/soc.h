/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#define GPIO_REG_SIZE         0x400
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#include <stm32f7xx.h>

/* IO pin functions */
enum stm32f7x_pin_config_mode {
	STM32F7X_PIN_CONFIG_DRIVE_PUSH_PULL,
	STM32F7X_PIN_CONFIG_DRIVE_PUSH_UP,
	STM32F7X_PIN_CONFIG_DRIVE_PUSH_DOWN,
	STM32F7X_PIN_CONFIG_DRIVE_OPEN_DRAIN,
	STM32F7X_PIN_CONFIG_DRIVE_OPEN_UP,
	STM32F7X_PIN_CONFIG_DRIVE_OPEN_DOWN,
	STM32F7X_PIN_CONFIG_AF_PUSH_PULL,
	STM32F7X_PIN_CONFIG_AF_PUSH_UP,
	STM32F7X_PIN_CONFIG_AF_PUSH_DOWN,
	STM32F7X_PIN_CONFIG_AF_OPEN_DRAIN,
	STM32F7X_PIN_CONFIG_AF_OPEN_UP,
	STM32F7X_PIN_CONFIG_AF_OPEN_DOWN,
	STM32F7X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
	STM32F7X_PIN_CONFIG_BIAS_PULL_UP,
	STM32F7X_PIN_CONFIG_BIAS_PULL_DOWN,
	STM32F7X_PIN_CONFIG_ANALOG,
};

#include "soc_irq.h"

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f7xx_ll_utils.h>
#include <stm32f7xx_ll_bus.h>
#include <stm32f7xx_ll_rcc.h>
#include <stm32f7xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H_ */
