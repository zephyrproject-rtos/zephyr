/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0477  */

/** System clock */
/* defined in stm32_common_clocks.h */

/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSI48 */
#define STM32_SRC_HSI48		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for HSI_KER */
#define STM32_SRC_HSI_KER	(STM32_SRC_HSI48 + 1) /* HSI + HSIKERON */
/** @brief Clock source identifier for CSI_KER */
#define STM32_SRC_CSI_KER	(STM32_SRC_HSI_KER + 1) /* CSI + CSIKERON */
/* PLL outputs */
/** @brief Clock source identifier for PLL1_P */
#define STM32_SRC_PLL1_P	(STM32_SRC_CSI_KER + 1)
/** @brief Clock source identifier for PLL1_Q */
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
/** @brief Clock source identifier for PLL1_R */
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)
/** @brief Clock source identifier for PLL1_S */
#define STM32_SRC_PLL1_S	(STM32_SRC_PLL1_R + 1)
/** @brief Clock source identifier for PLL2_P */
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_S + 1)
/** @brief Clock source identifier for PLL2_Q */
#define STM32_SRC_PLL2_Q	(STM32_SRC_PLL2_P + 1)
/** @brief Clock source identifier for PLL2_R */
#define STM32_SRC_PLL2_R	(STM32_SRC_PLL2_Q + 1)
/** @brief Clock source identifier for PLL2_S */
#define STM32_SRC_PLL2_S	(STM32_SRC_PLL2_R + 1)
/** @brief Clock source identifier for PLL2_T */
#define STM32_SRC_PLL2_T	(STM32_SRC_PLL2_S + 1)
/** @brief Clock source identifier for PLL3_P */
#define STM32_SRC_PLL3_P	(STM32_SRC_PLL2_T + 1)
/** @brief Clock source identifier for PLL3_Q */
#define STM32_SRC_PLL3_Q	(STM32_SRC_PLL3_P + 1)
/** @brief Clock source identifier for PLL3_R */
#define STM32_SRC_PLL3_R	(STM32_SRC_PLL3_Q + 1)
/** @brief Clock source identifier for PLL3_S */
#define STM32_SRC_PLL3_S	(STM32_SRC_PLL3_R + 1)

/* Clock muxes */
/** @brief Clock source identifier for CKPER */
#define STM32_SRC_CKPER		(STM32_SRC_PLL3_S + 1)
/** @brief Clock source identifier for HCLK1 */
#define STM32_SRC_HCLK1		(STM32_SRC_CKPER + 1)
/** @brief Clock source identifier for HCLK2 */
#define STM32_SRC_HCLK2		(STM32_SRC_HCLK1 + 1)
/** @brief Clock source identifier for HCLK3 */
#define STM32_SRC_HCLK3		(STM32_SRC_HCLK2 + 1)
/** @brief Clock source identifier for HCLK4 */
#define STM32_SRC_HCLK4		(STM32_SRC_HCLK3 + 1)
/** @brief Clock source identifier for HCLK5 */
#define STM32_SRC_HCLK5		(STM32_SRC_HCLK4 + 1)
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_HCLK5 + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
/** @brief Clock source identifier for PCLK1 */
#define STM32_SRC_PCLK1		(STM32_SRC_TIMPCLK2 + 1)
/** @brief Clock source identifier for PCLK2 */
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
/** @brief Clock source identifier for PCLK4 */
#define STM32_SRC_PCLK4		(STM32_SRC_PCLK2 + 1)
/** @brief Clock source identifier for PCLK5 */
#define STM32_SRC_PCLK5		(STM32_SRC_PCLK4 + 1)
/** Others: Not yet supported */

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x138
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x13C
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    0x158
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x140
/** @brief AHB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB5    0x134
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x148
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x14C
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x150
/** @brief APB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4    0x154
/** @brief APB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB5    0x144
/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB5
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_AHB3

/** @brief RCC_DxCCIP register offset (RM0477.pdf) */
#define D1CCIPR_REG		0x4C
/** @brief RCC D2CCIPR register offset */
#define D2CCIPR_REG		0x50
/** @brief RCC D3CCIPR register offset */
#define D3CCIPR_REG		0x54
/** @brief RCC D4CCIPR register offset */
#define D4CCIPR_REG		0x58

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x10

/** @brief Device domain clocks selection helpers (RM0477.pdf) */

/* TODO to be completed */

/* D1CCIPR devices */
/** @brief Kernel clock source selection for FMC */
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, D1CCIPR_REG)
/** @brief Kernel clock source selection for SDMMC */
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 2, D1CCIPR_REG)
/** @brief Kernel clock source selection for XSPI1 */
#define XSPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, D1CCIPR_REG)
/** @brief Kernel clock source selection for XSPI2 */
#define XSPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, D1CCIPR_REG)
/** @brief Kernel clock source selection for OTGFS */
#define OTGFS_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 14, D1CCIPR_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, D1CCIPR_REG)
/** @brief Kernel clock source selection for CKPER */
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, D1CCIPR_REG)

/* D2CCIPR devices */
/** @brief Kernel clock source selection for USART234578 */
#define USART234578_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, D2CCIPR_REG)
/** @brief Kernel clock source selection for SPI23 */
#define SPI23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, D2CCIPR_REG)
/** @brief Kernel clock source selection for I2C23 */
#define I2C23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, D2CCIPR_REG)
/** @brief Kernel clock source selection for I2C1_I3C1 */
#define I2C1_I3C1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 13, 12, D2CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, D2CCIPR_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, D2CCIPR_REG)

/* D3CCIPR devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, D3CCIPR_REG)
/** @brief Kernel clock source selection for SPI45 */
#define SPI45_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, D3CCIPR_REG)
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 8, D3CCIPR_REG)
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, D3CCIPR_REG)
/** @brief Kernel clock source selection for SAI2 */
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 22, 20, D3CCIPR_REG)

/* D4CCIPR devices */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, D4CCIPR_REG)
/** @brief Kernel clock source selection for SPI6 */
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, D4CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM23 */
#define LPTIM23_SEL(val)	STM32_DT_CLOCK_SELECT((val), 10, 8, D4CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM45 */
#define LPTIM45_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 12, D4CCIPR_REG)

/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/* CFGR devices */
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR_REG)
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR_REG)
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1 1
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2 2
/** @brief MCO prescaler division factor: 3 */
#define MCO_PRE_DIV_3 3
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4 4
/** @brief MCO prescaler division factor: 5 */
#define MCO_PRE_DIV_5 5
/** @brief MCO prescaler division factor: 6 */
#define MCO_PRE_DIV_6 6
/** @brief MCO prescaler division factor: 7 */
#define MCO_PRE_DIV_7 7
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8 8
/** @brief MCO prescaler division factor: 9 */
#define MCO_PRE_DIV_9 9
/** @brief MCO prescaler division factor: 10 */
#define MCO_PRE_DIV_10 10
/** @brief MCO prescaler division factor: 11 */
#define MCO_PRE_DIV_11 11
/** @brief MCO prescaler division factor: 12 */
#define MCO_PRE_DIV_12 12
/** @brief MCO prescaler division factor: 13 */
#define MCO_PRE_DIV_13 13
/** @brief MCO prescaler division factor: 14 */
#define MCO_PRE_DIV_14 14
/** @brief MCO prescaler division factor: 15 */
#define MCO_PRE_DIV_15 15

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_ */
