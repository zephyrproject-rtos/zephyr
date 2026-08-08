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

/** Other fixed clocks.
 * - CLKSLOWMUX: used to query slow clock tree frequency
 * - CLK16MHZ: CLK_ROOT_DIV, secondary clock for USART, I2C, ADC, LPUART and SPI3/I2S
 * - CLKSYS: CLK_ROOT divided by CLKSYSDIV, clocks the CPU, AHB and APB
 * - CLKROOT_DIV2: CLK_ROOT / 2, secondary clock for SPI3/I2S
 */
#define STM32_SRC_CLKSLOWMUX	(STM32_SRC_LSI + 1)
#define STM32_SRC_CLK16MHZ	(STM32_SRC_CLKSLOWMUX + 1)
#define STM32_SRC_CLKSYS	(STM32_SRC_CLK16MHZ + 1)
#define STM32_SRC_CLKROOT_DIV2	(STM32_SRC_CLKSYS + 1)

/* Bus clocks: offset of the RCC clock enable register for each bus */
#define STM32_CLOCK_BUS_AHB0	0x50	/**< RCC_AHBENR offset. */
#define STM32_CLOCK_BUS_APB0	0x54	/**< RCC_APB0ENR offset. */
#define STM32_CLOCK_BUS_APB1	0x58	/**< RCC_APB1ENR offset. */
#define STM32_CLOCK_BUS_APB2	0x60	/**< RCC_APB2ENR offset. */

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB0	/**< Lowest bus register offset. */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2	/**< Highest bus register offset. */

/** @brief RCC_CFGR register offset */
#define CFGR_REG	0x08

/** @endcond */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL3_CLOCK_H_ */
