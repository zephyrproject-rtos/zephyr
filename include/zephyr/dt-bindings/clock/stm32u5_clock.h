/*
 * Copyright (c) 2022 Linaro Limited
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0456, Figure 36 Clock tree for STM32U5 Series */

/** System clock */
/* defined in stm32_common_clocks.h */
/* Fixed clocks */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSI16 */
#define STM32_SRC_HSI16		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for HSI48 */
#define STM32_SRC_HSI48		(STM32_SRC_HSI16 + 1)
/** @brief Clock source identifier for MSIS */
#define STM32_SRC_MSIS		(STM32_SRC_HSI48 + 1)
/** @brief Clock source identifier for MSIK */
#define STM32_SRC_MSIK		(STM32_SRC_MSIS + 1)
/* Bus clock */
/** @brief Clock source identifier for HCLK */
#define STM32_SRC_HCLK		(STM32_SRC_MSIK + 1)
/** @brief Clock source identifier for PCLK1 */
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK + 1)
/** @brief Clock source identifier for PCLK2 */
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
/** @brief Clock source identifier for PCLK3 */
#define STM32_SRC_PCLK3		(STM32_SRC_PCLK2 + 1)
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK3 + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
/* PLL outputs */
/** @brief Clock source identifier for PLL1_P */
#define STM32_SRC_PLL1_P	(STM32_SRC_TIMPCLK2 + 1)
/** @brief Clock source identifier for PLL1_Q */
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
/** @brief Clock source identifier for PLL1_R */
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)
/** @brief Clock source identifier for PLL2_P */
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_R + 1)
/** @brief Clock source identifier for PLL2_Q */
#define STM32_SRC_PLL2_Q	(STM32_SRC_PLL2_P + 1)
/** @brief Clock source identifier for PLL2_R */
#define STM32_SRC_PLL2_R	(STM32_SRC_PLL2_Q + 1)
/** @brief Clock source identifier for PLL3_P */
#define STM32_SRC_PLL3_P	(STM32_SRC_PLL2_R + 1)
/** @brief Clock source identifier for PLL3_Q */
#define STM32_SRC_PLL3_Q	(STM32_SRC_PLL3_P + 1)
/** @brief Clock source identifier for PLL3_R */
#define STM32_SRC_PLL3_R	(STM32_SRC_PLL3_Q + 1)
/* DSI PHY clock */
/** @brief Clock source identifier for DSIPHY */
#define STM32_SRC_DSIPHY	(STM32_SRC_PLL3_R + 1)
/** Clock muxes */
/* #define STM32_SRC_ICLK	TBD */

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x088
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x08C
/** @brief AHB2_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2_2  0x090
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    0x094
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

/** @brief RCC_CCIPRx register offset (RM0456.pdf) */
#define CCIPR1_REG		0xE0
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0xE4
/** @brief RCC CCIPR3 register offset */
#define CCIPR3_REG		0xE8

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0xF0

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
/** @brief Kernel clock source selection for FDCAN1 */
#define FDCAN1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR1_REG)
/** @brief Kernel clock source selection for ICKLK */
#define ICKLK_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR1_REG)
/** @brief Kernel clock source selection for TIMIC */
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 29, CCIPR1_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for MDF1 */
#define MDF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR2_REG)
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 5, CCIPR2_REG)
/** @brief Kernel clock source selection for SAI2 */
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 8, CCIPR2_REG)
/** @brief Kernel clock source selection for SAE */
#define SAE_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 11, CCIPR2_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for SDMMC */
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 14, CCIPR2_REG)
/** @brief Kernel clock source selection for DSI */
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 15, CCIPR2_REG)
/** @brief Kernel clock source selection for USART6 */
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 16, 16, CCIPR2_REG)
/** @brief Kernel clock source selection for LTDC */
#define LTDC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 18, CCIPR2_REG)
/** @brief Kernel clock source selection for OCTOSPI */
#define OCTOSPI_SEL(val)	STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR2_REG)
/** @brief Kernel clock source selection for HSPI */
#define HSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR2_REG)
/** @brief Kernel clock source selection for I2C5 */
#define I2C5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR2_REG)
/** @brief Kernel clock source selection for I2C6 */
#define I2C6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR2_REG)
/** @brief Kernel clock source selection for OTGHS */
#define OTGHS_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR2_REG)
/* CCIPR3 devices */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI3 */
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 3, CCIPR3_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR3_REG)
/** @brief Kernel clock source selection for LPTIM34 */
#define LPTIM34_SEL(val)	STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR3_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
/** @brief Kernel clock source selection for ADCDAC */
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR3_REG)
/** @brief Kernel clock source selection for DAC1 */
#define DAC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 15, CCIPR3_REG)
/** @brief Kernel clock source selection for ADF1 */
#define ADF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR3_REG)
/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/* CFGR1 devices */
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 27, 24, CFGR1_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CFGR1_REG)

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_ */
