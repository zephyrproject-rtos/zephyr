/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_

/** Define system & low-speed clocks */
#include "stm32_common_clocks.h"

/** Other fixed clocks.
 * - CLKSLOWMUX: used to query slow clock tree frequency
 * - CLK16MHZ: secondary clock for LPUART, SPI3/I2S and BLE
 * - CLK32MHZ: secondary clock for SPI3/I2S and BLE
 */
#define STM32_SRC_CLKSLOWMUX		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for CLK16MHZ */
#define STM32_SRC_CLK16MHZ		(STM32_SRC_CLKSLOWMUX + 1)
/** @brief Clock source identifier for CLK32MHZ */
#define STM32_SRC_CLK32MHZ		(STM32_SRC_CLK16MHZ + 1)

/* Bus clocks */
/** @brief AHB0 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB0	0x50
/** @brief APB0 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB0	0x54
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1	0x58
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2	0x60

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB0
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2

/** @brief RCC_CFGR register offset */
#define CFGR_REG	0x08

/** @brief RCC_APB2ENR register offset */
#define APB2ENR_REG	0x60

/** @brief Device clk sources selection helpers */

/* WB05/WB09 only */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 13, 13, CFGR_REG)
/* WB06/WB07 only */
/** @brief Kernel clock source selection for SPI2_I2S2 */
#define SPI2_I2S2_SEL(val)	STM32_DT_CLOCK_SELECT((val), 22, 22, CFGR_REG)
/* `msb` is only 22 for WB06/WB07, but a single definition with msb=23 is acceptable */
/** @brief Kernel clock source selection for SPI3_I2S3 */
#define SPI3_I2S3_SEL(val)	STM32_DT_CLOCK_SELECT((val), 23, 22, CFGR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_ */
