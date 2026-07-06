/*
 * Copyright (c) 2023 Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_

#include "stm32_common_clocks.h"

/* Bus clocks */
/** @brief IOP bus clock enable register offset */
#define STM32_CLOCK_BUS_IOP     0x034
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x038
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x03c
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x040

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_IOP
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1_2

/** Domain clocks */
/* RM0490, §5.4.21/22 Clock configuration register (RCC_CCIPRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h
 * STM32_SRC_HSI relates to HSI48 clock mentioned in RM0490 Reference Manual
 * STM32_SRC_HSI48 relates to HSIUSB48 mentioned in RM0490 Reference Manual
 */
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSI48 */
#define STM32_SRC_HSI48		(STM32_SRC_HSI + 1)
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_HSI48 + 1)
/* Peripheral bus clock */
/** @brief Clock source identifier for PCLK */
#define STM32_SRC_PCLK		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK + 1)

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x54
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0x58

/** @brief RCC_CSR1 register offset */
#define CSR1_REG		0x5C

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x08

/** @brief Device domain clocks selection helpers */
/* CCIPR devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR_REG)
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR_REG)
/** @brief Kernel clock source selection for I2C2_I2S1 */
#define I2C2_I2S1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for USB */
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 12, 12, CCIPR2_REG)
/* CSR1 devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CSR1_REG)

/* CFGR1 devices */
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 19, 16, CFGR1_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 23, 20, CFGR1_REG)
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 27, 24, CFGR1_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 31, 28, CFGR1_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1   0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2   1
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4   2
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8   3
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16  4
/** @brief MCO prescaler division factor: 32 */
#define MCO_PRE_DIV_32  5
/** @brief MCO prescaler division factor: 64 */
#define MCO_PRE_DIV_64  6
/** @brief MCO prescaler division factor: 128 */
#define MCO_PRE_DIV_128 7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_ */
