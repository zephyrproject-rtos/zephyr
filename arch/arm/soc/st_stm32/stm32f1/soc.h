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
 *   advanced ARM ® -based 32-bit MCUs
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
#include <drivers/rand32.h>

#include <stm32f1xx.h>

/* IO pin functions are mostly common across STM32 devices. Notable
 * exception is STM32F1 as these MCUs do not have registers for
 * configuration of pin's alternate function. The configuration is
 * done implicitly by setting specific mode and config in MODE and CNF
 * registers for particular pin.
 */
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
