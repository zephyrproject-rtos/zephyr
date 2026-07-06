/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_

#include "stm32_common_clocks.h"

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x048
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x04c
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    0x050
/** @brief APB0 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB0    0x054
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x058
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x05c
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x060
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3    0x064

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** Domain clocks */
/* RM0461, §6.4.29 Clock configuration register (RCC_CFGR3) */


/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for MSI */
#define STM32_SRC_MSI		(STM32_SRC_HSI + 1)
/* #define STM32_SRC_HSI48	TBD */
/* Bus clock */
/** @brief Clock source identifier for PCLK */
#define STM32_SRC_PCLK		(STM32_SRC_MSI + 1)
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
/* PLL clock outputs */
/** @brief Clock source identifier for PLL_P */
#define STM32_SRC_PLL_P		(STM32_SRC_TIMPCLK2 + 1)
/** @brief Clock source identifier for PLL_Q */
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
/** @brief Clock source identifier for PLL_R */
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x88

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x90

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG        0x08

/** @brief Device domain clocks selection helpers */
/* CCIPR devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR_REG)
/** @brief Kernel clock source selection for USART2 */
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR_REG)
/** @brief Kernel clock source selection for SPI2 */
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR_REG)
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR_REG)
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR_REG)
/** @brief Kernel clock source selection for I2C2 */
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM2 */
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM3 */
#define LPTIM3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, CCIPR_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR_REG)
/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)
/* CFGR1 devices */
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)       STM32_DT_CLOCK_SELECT(val, 27, 24, CFGR1_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)       STM32_DT_CLOCK_SELECT(val, 30, 28, CFGR1_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1  0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2  1
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4  2
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8  3
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16 4

/* MCO clock output */
/** @brief MCO clock source selection value: NOCLK */
#define MCO_SEL_NOCLK     0
/** @brief MCO clock source selection value: SYSCLKPRE */
#define MCO_SEL_SYSCLKPRE 1
/** @brief MCO clock source selection value: MSI */
#define MCO_SEL_MSI       2
/** @brief MCO clock source selection value: HSI16 */
#define MCO_SEL_HSI16     3
/** @brief MCO clock source selection value: HSE32 */
#define MCO_SEL_HSE32     4
/** @brief MCO clock source selection value: PLL1RCLK */
#define MCO_SEL_PLL1RCLK  5
/** @brief MCO clock source selection value: LSI */
#define MCO_SEL_LSI       6
/** @brief MCO clock source selection value: LSE */
#define MCO_SEL_LSE       8
/** @brief MCO clock source selection value: PLL1PCLK */
#define MCO_SEL_PLL1PCLK  13
/** @brief MCO clock source selection value: PLL1QCLK */
#define MCO_SEL_PLL1QCLK  14

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_ */
