/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DT bindings for STM32WL3 clock system
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_
/** @cond INTERNAL_HIDDEN */

/** Define system & low-speed clocks */
#include "stm32_common_clocks.h"

/** CLK_ROOT is the name given to the system clock. */
#define STM32_SRC_CLK_ROOT	STM32_SRC_SYSCLK

/** Other fixed clocks.
 * - CLKSLOWMUX: used to query slow clock tree frequency
 * - CLK_ROOT_DIV: kernel clock for USART, I2C, RNG, LPAWUR, ADC and LPUART
 * - CLKSYS: CLK_ROOT divided by CLKSYSDIV, clocks the CPU, AHB and APB
 */
#define STM32_SRC_CLKSLOWMUX	(STM32_SRC_LSI + 1)
#define STM32_SRC_CLK_ROOT_DIV	(STM32_SRC_CLKSLOWMUX + 1)
#define STM32_SRC_CLKSYS	(STM32_SRC_CLK_ROOT_DIV + 1)

/* Bus clocks: offset of the RCC clock enable register for each bus */
#define STM32_CLOCK_BUS_AHB0	0x50	/**< RCC_AHBENR offset. */
#define STM32_CLOCK_BUS_APB0	0x54	/**< RCC_APB0ENR offset. */
#define STM32_CLOCK_BUS_APB1	0x58	/**< RCC_APB1ENR offset. */
#define STM32_CLOCK_BUS_APB2	0x60	/**< RCC_APB2ENR offset. */

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB0	/**< Lowest bus register offset. */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2	/**< Highest bus register offset. */

/** @endcond */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_ */
