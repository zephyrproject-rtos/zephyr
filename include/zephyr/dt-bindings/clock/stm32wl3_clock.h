/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_

/** Define system & low-speed clocks */
#include "stm32_common_clocks.h"

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB0	0x50
#define STM32_CLOCK_BUS_APB0	0x54
#define STM32_CLOCK_BUS_APB1	0x58
#define STM32_CLOCK_BUS_APB2	0x60

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB0
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2

/** @brief RCC_CFGR register offset */
#define CFGR_REG	0x08

/*
 * TODO: add WL3 clock source selection helpers
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_ */
