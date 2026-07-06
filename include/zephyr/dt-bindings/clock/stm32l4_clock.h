/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_

#include "stm32_common_clocks.h"

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x048
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x04c
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    0x050
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x058
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x05c
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x060

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2

/** Domain clocks */
/* RM0351/RM0432, § Clock configuration register (RCC_CCIPRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/* External High Speed oscillator */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/* Internal High Speed 16MHz oscillator */
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_HSE + 1)
/* Internal High Speed 48MHz oscillator */
/** @brief Clock source identifier for HSI48 */
#define STM32_SRC_HSI48		(STM32_SRC_HSI + 1)
/* Internal Multi Speed oscillator */
/** @brief Clock source identifier for MSI */
#define STM32_SRC_MSI		(STM32_SRC_HSI48 + 1)
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
/* PLLSAI1 clocks */
/* PLLSAI1 P output, can be selected for SAI peripherals */
/** @brief Clock source identifier for PLLSAI1_P */
#define STM32_SRC_PLLSAI1_P	(STM32_SRC_PLL_R + 1)
/** PLLSAI1 Q output, 48MHz clock which can be selected for USB / RNG */
#define STM32_SRC_PLLSAI1_Q	(STM32_SRC_PLLSAI1_P + 1)
/* PLLSAI1 R output, can be selected for ADC peripherals */
/** @brief Clock source identifier for PLLSAI1_R */
#define STM32_SRC_PLLSAI1_R	(STM32_SRC_PLLSAI1_Q + 1)
/* PLLSAI2 clocks */
/* PLLSAI2 P output, can be selected for SAI peripherals */
/** @brief Clock source identifier for PLLSAI2_P */
#define STM32_SRC_PLLSAI2_P	(STM32_SRC_PLLSAI1_R + 1)
/* PLLSAI2 Q output, can be selected for DSI peripheral */
/** @brief Clock source identifier for PLLSAI2_Q */
#define STM32_SRC_PLLSAI2_Q	(STM32_SRC_PLLSAI2_P + 1)
/* PLLSAI2 R output, can be selected for ADC or LTDC peripheral */
/** @brief Clock source identifier for PLLSAI2_R */
#define STM32_SRC_PLLSAI2_R	(STM32_SRC_PLLSAI2_Q + 1)
/* PLLSAI2 R output with addition divider, used for LTDC peripheral */
/** @brief Clock source identifier for PLLSAI2_POST_R */
#define STM32_SRC_PLLSAI2_POST_R	(STM32_SRC_PLLSAI2_R + 1)

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x88
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0x9C

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x90

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x08

/** @brief Device domain clocks selection helpers */
/* CCIPR devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR_REG)
/** @brief Kernel clock source selection for USART2 */
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR_REG)
/** @brief Kernel clock source selection for USART3 */
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR_REG)
/** @brief Kernel clock source selection for UART4 */
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR_REG)
/** @brief Kernel clock source selection for UART5 */
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR_REG)
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
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR_REG)
/** @brief Kernel clock source selection for SAI2 */
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR_REG)
/** @brief Kernel clock source selection for CLK48 */
#define CLK48_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, CCIPR_REG)
/** @brief Kernel clock source selection for SWPMI1 */
#define SWPMI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 30, 30, CCIPR_REG)
/** @brief Kernel clock source selection for DFSDM1 */
#define DFSDM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 31, CCIPR_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for I2C4 */
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR2_REG)
/** @brief Kernel clock source selection for DFSDM */
#define DFSDM_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 2, CCIPR2_REG)
/** @brief Kernel clock source selection for ADFSDM */
#define ADFSDM_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 3, CCIPR2_REG)
/** @brief Kernel clock source selection for DSI */
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 12, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for SDMMC */
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 14, CCIPR2_REG)
/** @brief Kernel clock source selection for OSPI */
#define OSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR2_REG)
/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)
/* CFGR devices */
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 27, 24, CFGR_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CFGR_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1	0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2	1
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4	2
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8	3
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16	4

/* MCO clock output */
/** @brief MCO clock source selection value: SYSCLK */
#define MCO_SEL_SYSCLK	1
/** @brief MCO clock source selection value: MSI */
#define MCO_SEL_MSI	2
/** @brief MCO clock source selection value: HSI16 */
#define MCO_SEL_HSI16	3
/** @brief MCO clock source selection value: HSE */
#define MCO_SEL_HSE	4
/** @brief MCO clock source selection value: PLLCLK */
#define MCO_SEL_PLLCLK	5
/** @brief MCO clock source selection value: LSI */
#define MCO_SEL_LSI	6
/** @brief MCO clock source selection value: LSE */
#define MCO_SEL_LSE	7
/** @brief MCO clock source selection value: HSI48 */
#define MCO_SEL_HSI48	8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_ */
