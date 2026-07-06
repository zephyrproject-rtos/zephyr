/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_

#include "stm32_common_clocks.h"

/** Peripheral clock sources */

/* RM0493, Figure 34, clock tree */
/* RM0515, Figure 36, clock tree */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSI16 */
#define STM32_SRC_HSI16		(STM32_SRC_HSE + 1)
/* Bus clock */
/** @brief Clock source identifier for HCLK1 */
#define STM32_SRC_HCLK1		(STM32_SRC_HSI16 + 1)
/** @brief Clock source identifier for HCLK5 */
#define STM32_SRC_HCLK5		(STM32_SRC_HCLK1 + 1)
/** @brief Clock source identifier for PCLK1 */
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK5 + 1)
/** @brief Clock source identifier for PCLK2 */
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
/** @brief Clock source identifier for PCLK7 */
#define STM32_SRC_PCLK7		(STM32_SRC_PCLK2 + 1)
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK7 + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
/* PLL outputs */
/** @brief Clock source identifier for PLL1_P */
#define STM32_SRC_PLL1_P	(STM32_SRC_TIMPCLK2 + 1)
/** @brief Clock source identifier for PLL1_Q */
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
/** @brief Clock source identifier for PLL1_R */
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)

/** @brief Clock source identifier for CLOCK_MIN */
#define STM32_SRC_CLOCK_MIN	STM32_SRC_PLL1_P
/** @brief Clock source identifier for CLOCK_MAX */
#define STM32_SRC_CLOCK_MAX	STM32_SRC_SYSCLK

/* Bus clocks (Register address offsets) */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x088
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x08C
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x094
/** @brief AHB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB5    0x098
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x09C
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x0A0
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x0A4
/** @brief APB7 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB7    0x0A8

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB7

/** @brief RCC_CCIPRx register offset (RM0493.pdf) */
#define CCIPR1_REG		0xE0
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0xE4
/** @brief RCC CCIPR3 register offset */
#define CCIPR3_REG		0xE8
/** @brief RCC_BCDR1 register offset (RM0493.pdf) */
#define BCDR1_REG		0xF0

/** @brief Device clk sources selection helpers */
/* CCIPR1 devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR1_REG)
/** @brief Kernel clock source selection for USART2 */
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR1_REG)
/** @brief Kernel clock source selection for USART3 */
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR1_REG)
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR1_REG)
/** @brief Kernel clock source selection for I2C2 */
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR1_REG)
/** @brief Kernel clock source selection for I2C4 */
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI2 */
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR1_REG)
/** @brief Kernel clock source selection for LPTIM2 */
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR1_REG)
/** @brief Kernel clock source selection for SYSTICK */
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR1_REG)
/** @brief Kernel clock source selection for TIMIC */
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 31, CCIPR1_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 5, CCIPR2_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for OTGHS */
#define OTGHS_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, CCIPR2_REG)
/* CCIPR3 devices */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI3 */
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 3, CCIPR3_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR3_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR3_REG)
/* BCDR1 devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BCDR1_REG)
/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C
/* CFGR1 devices */
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 27, 24, CFGR1_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CFGR1_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1 0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2 1
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4 2
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8 3
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16 4

/* MCO clock output */
/** @brief MCO clock source selection value: SYSCLKPRE */
#define MCO_SEL_SYSCLKPRE 1
/** @brief MCO clock source selection value: HSI16 */
#define MCO_SEL_HSI16 3
/** @brief MCO clock source selection value: HSE32 */
#define MCO_SEL_HSE32 4
/** @brief MCO clock source selection value: PLL1RCLK */
#define MCO_SEL_PLL1RCLK 5
/** @brief MCO clock source selection value: LSI */
#define MCO_SEL_LSI 6
/** @brief MCO clock source selection value: LSE */
#define MCO_SEL_LSE 7
/** @brief MCO clock source selection value: PLL1PCLK */
#define MCO_SEL_PLL1PCLK 8
/** @brief MCO clock source selection value: PLL1QCLK */
#define MCO_SEL_PLL1QCLK 9
/** @brief MCO clock source selection value: HCLK5 */
#define MCO_SEL_HCLK5 10


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_ */
