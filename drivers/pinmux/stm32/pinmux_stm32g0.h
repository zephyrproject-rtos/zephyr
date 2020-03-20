/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 * Copyright (c) 2019 ST Microelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_
#define ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_
/**
 * @file Header for STM32G0 pin multiplexing helper
 */

#if defined(CONFIG_SOC_STM32G071XX) || defined(CONFIG_SOC_STM32G081XX)
/*This section is for STM32G071XX or STM32G081XX*/
#include "pinmux_stm32g071x.h"
#elif defined(CONFIG_SOC_STM32G070XX)
#error "GPIO ALT MODE FUNCTIONS NOT DEFINED FOR SOC_STM32G070XX"
#elif defined(CONFIG_SOC_STM32G041XX) || defined(CONFIG_SOC_STM32G031XX)
/*This section is for STM32G031XX or STM32G041XX*/
#include "pinmux_stm32g041x.h"
#elif defined(CONFIG_SOC_STM32G030XX)
#error "GPIO ALT MODE FUNCTIONS NOT DEFINED FOR SOC_STM32G030XX"
#else
#error "CONFIG_GPIO_STM32 enabled, but unknown GO type"
#endif /* SOC_STM32G071XX */

#endif /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_ */
