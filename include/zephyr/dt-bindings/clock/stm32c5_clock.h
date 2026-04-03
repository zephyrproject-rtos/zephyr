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
/** @cond INTERNAL_HIDDEN */

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0522, Figure 24 Clock tree for STM32C5 Series */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSIS		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSIDIV3	(STM32_SRC_HSIS + 1)
#define STM32_SRC_HSIK		(STM32_SRC_HSIDIV3 + 1)
#define STM32_SRC_PSIS		(STM32_SRC_HSIK + 1)
#define STM32_SRC_PSIDIV3	(STM32_SRC_PSIS + 1)
#define STM32_SRC_PSIK		(STM32_SRC_PSIDIV3 + 1)
/** Bus clock */
#define STM32_SRC_HCLK		(STM32_SRC_PSIK + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK3		(STM32_SRC_PCLK2 + 1)
/* Clock muxes */
#define STM32_SRC_CK48		(STM32_SRC_PCLK3 + 1)
/** External clock */
/* #define STM32_SRC_AUDIOCLK	TBD */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB4    0x094
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** @brief RCC_CCIPRx register offset (RM0522.pdf) */
#define CCIPR1_REG		0xD8
#define CCIPR2_REG		0xDC
#define CCIPR3_REG		0xE0

/** @brief RCC_BDCR register offset */
#define RTCCR_REG		0xF0

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C

/** @brief Device domain clocks selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR1_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR1_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR1_REG)
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR1_REG)
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR1_REG)
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR1_REG)
#define UART7_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR1_REG)
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR1_REG)
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR1_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR1_REG)
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR1_REG)
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR1_REG)
/** CCIPR2 devices */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR2_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR2_REG)
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR2_REG)
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR2_REG)
#define ADCDACPRE_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR2_REG)
#define DAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 15, CCIPR2_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR2_REG)
#define CK48_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR2_REG)
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR2_REG)
/** CCIPR3 devices */
#define XSPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR3_REG)
#define ETH1REFCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 8, 8, CCIPR3_REG)
#define ETH1PTPCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
#define ETH1CLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 13, CCIPR3_REG)
#define ETH1CLKDIV_SEL(val)	STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR3_REG)
#define ETH1PTPDIV_SEL(val)	STM32_DT_CLOCK_SELECT((val), 31, 28, CCIPR3_REG)
/** RTCCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, RTCCR_REG)

/** CFGR1 devices */
#define MCO1_PRE(val)		STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR1_REG)
#define MCO1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR1_REG)
#define MCO2_PRE(val)		STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR1_REG)
#define MCO2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR1_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1 1
#define MCO_PRE_DIV_2 2
#define MCO_PRE_DIV_3 3
#define MCO_PRE_DIV_4 4
#define MCO_PRE_DIV_5 5
#define MCO_PRE_DIV_6 6
#define MCO_PRE_DIV_7 7
#define MCO_PRE_DIV_8 8
#define MCO_PRE_DIV_9 9
#define MCO_PRE_DIV_10 10
#define MCO_PRE_DIV_11 11
#define MCO_PRE_DIV_12 12
#define MCO_PRE_DIV_13 13
#define MCO_PRE_DIV_14 14
#define MCO_PRE_DIV_15 15

/** @endcond */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C5_CLOCK_H_ */
