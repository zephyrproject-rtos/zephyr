/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 * Copyright (c) 2019 ST Microelectronics
 * Copyright (c) Framework Computer LLC <ktl@frame.work>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_
#define ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_

/**
 * @file Header for STM32G0 pin multiplexing helper
 */

#include "pinmux_stm32g0_common.h"

#if defined(CONFIG_SOC_STM32G071XX)
/* Same for stm32g071xx and stm32g081xx */
#include "pinmux_stm32g071xx.h"

#elif defined(CONFIG_SOC_STM32G031XX)
/* Same for stm32g031xx and stm32g041xx*/
#include "pinmux_stm32g031xx.h"

#endif  /* SOC_STM32G071XX */

#endif  /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32G0_H_ */
