/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DT bindings for STM32C5 clock system
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C5_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C5_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0522, Figure 24 Clock tree for STM32C5 Series */

/** System clock */
/* defined in stm32_common_clocks.h */
/* Fixed clocks */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSIS */
#define STM32_SRC_HSIS		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for HSIDIV3 */
#define STM32_SRC_HSIDIV3	(STM32_SRC_HSIS + 1)
/** @brief Clock source identifier for HSIK */
#define STM32_SRC_HSIK		(STM32_SRC_HSIDIV3 + 1)
/** @brief Clock source identifier for PSIS */
#define STM32_SRC_PSIS		(STM32_SRC_HSIK + 1)
/** @brief Clock source identifier for PSIDIV3 */
#define STM32_SRC_PSIDIV3	(STM32_SRC_PSIS + 1)
/** @brief Clock source identifier for PSIK */
#define STM32_SRC_PSIK		(STM32_SRC_PSIDIV3 + 1)
/* Bus clock */
/** @brief Clock source identifier for HCLK */
#define STM32_SRC_HCLK		(STM32_SRC_PSIK + 1)
/** @brief Clock source identifier for PCLK1 */
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK + 1)
/** @brief Clock source identifier for PCLK2 */
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
/** @brief Clock source identifier for PCLK3 */
#define STM32_SRC_PCLK3		(STM32_SRC_PCLK2 + 1)
/* Clock muxes */
/** @brief Clock source identifier for CK48 */
#define STM32_SRC_CK48		(STM32_SRC_PCLK3 + 1)
/** External clock */
/* #define STM32_SRC_AUDIOCLK	TBD */

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x088
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x08C
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x094
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x09C
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x0A0
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x0A4
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3    0x0A8

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** @brief RCC_CCIPRx register offset (RM0522.pdf) */
#define CCIPR1_REG		0xD8
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0xDC
/** @brief RCC CCIPR3 register offset */
#define CCIPR3_REG		0xE0

/** @brief RCC_BDCR register offset */
#define RTCCR_REG		0xF0

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C

/** @brief Device domain clocks selection helpers */
/* CCIPR1 devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR1_REG)
/** @brief Kernel clock source selection for USART2 */
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR1_REG)
/** @brief Kernel clock source selection for USART3 */
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR1_REG)
/** @brief Kernel clock source selection for UART4 */
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR1_REG)
/** @brief Kernel clock source selection for UART5 */
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR1_REG)
/** @brief Kernel clock source selection for USART6 */
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR1_REG)
/** @brief Kernel clock source selection for UART7 */
#define UART7_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR1_REG)
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI2 */
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI3 */
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR1_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR1_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR2_REG)
/** @brief Kernel clock source selection for I2C2 */
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR2_REG)
/** @brief Kernel clock source selection for I3C1 */
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR2_REG)
/** @brief Kernel clock source selection for ADCDAC */
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR2_REG)
/** @brief Kernel clock source selection for ADCDACPRE */
#define ADCDACPRE_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for DAC */
#define DAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 15, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR2_REG)
/** @brief Kernel clock source selection for CK48 */
#define CK48_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR2_REG)
/** @brief Kernel clock source selection for SYSTICK */
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR2_REG)
/* CCIPR3 devices */
/** @brief Kernel clock source selection for XSPI1 */
#define XSPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR3_REG)
/** @brief Kernel clock source selection for ETH1REFCLK */
#define ETH1REFCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 8, 8, CCIPR3_REG)
/** @brief Kernel clock source selection for ETH1PTPCLK */
#define ETH1PTPCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
/** @brief Kernel clock source selection for ETH1CLK */
#define ETH1CLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 13, CCIPR3_REG)
/** @brief Kernel clock source selection for ETH1CLKDIV */
#define ETH1CLKDIV_SEL(val)	STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR3_REG)
/** @brief Kernel clock source selection for ETH1PTPDIV */
#define ETH1PTPDIV_SEL(val)	STM32_DT_CLOCK_SELECT((val), 31, 28, CCIPR3_REG)
/* RTCCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, RTCCR_REG)

/* CFGR1 devices */
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)		STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR1_REG)
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR1_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val)		STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR1_REG)
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR1_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1		1
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2		2
/** @brief MCO prescaler division factor: 3 */
#define MCO_PRE_DIV_3		3
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4		4
/** @brief MCO prescaler division factor: 5 */
#define MCO_PRE_DIV_5		5
/** @brief MCO prescaler division factor: 6 */
#define MCO_PRE_DIV_6		6
/** @brief MCO prescaler division factor: 7 */
#define MCO_PRE_DIV_7		7
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8		8
/** @brief MCO prescaler division factor: 9 */
#define MCO_PRE_DIV_9		9
/** @brief MCO prescaler division factor: 10 */
#define MCO_PRE_DIV_10		10
/** @brief MCO prescaler division factor: 11 */
#define MCO_PRE_DIV_11		11
/** @brief MCO prescaler division factor: 12 */
#define MCO_PRE_DIV_12		12
/** @brief MCO prescaler division factor: 13 */
#define MCO_PRE_DIV_13		13
/** @brief MCO prescaler division factor: 14 */
#define MCO_PRE_DIV_14		14
/** @brief MCO prescaler division factor: 15 */
#define MCO_PRE_DIV_15		15

/* MCO1 clock output */
/** @brief MCO1 clock source selection value: SYSCLK */
#define MCO1_SEL_SYSCLK		0
/** @brief MCO1 clock source selection value: HSE */
#define MCO1_SEL_HSE		1
/** @brief MCO1 clock source selection value: LSE */
#define MCO1_SEL_LSE		2
/** @brief MCO1 clock source selection value: LSI */
#define MCO1_SEL_LSI		3
/** @brief MCO1 clock source selection value: PSIK */
#define MCO1_SEL_PSIK		4
/** @brief MCO1 clock source selection value: HSIK */
#define MCO1_SEL_HSIK		5
/** @brief MCO1 clock source selection value: PSIS */
#define MCO1_SEL_PSIS		6
/** @brief MCO1 clock source selection value: HSIS */
#define MCO1_SEL_HSIS		7

/* MCO2 clock output */
/** @brief MCO2 clock source selection value: SYSCLK */
#define MCO2_SEL_SYSCLK		0
/** @brief MCO2 clock source selection value: HSE */
#define MCO2_SEL_HSE		1
/** @brief MCO2 clock source selection value: LSE */
#define MCO2_SEL_LSE		2
/** @brief MCO2 clock source selection value: LSI */
#define MCO2_SEL_LSI		3
/** @brief MCO2 clock source selection value: PSIK */
#define MCO2_SEL_PSIK		4
/** @brief MCO2 clock source selection value: HSIK */
#define MCO2_SEL_HSIK		5
/** @brief MCO2 clock source selection value: PSIDIV3 */
#define MCO2_SEL_PSIDIV3	6
/** @brief MCO2 clock source selection value: HSIDIV3 */
#define MCO2_SEL_HSIDIV3	7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C5_CLOCK_H_ */
