/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H5_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H5_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0481/0492, Table 47 Kernel clock distribution summary */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for CSI */
#define STM32_SRC_CSI		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_CSI + 1)
/** @brief Clock source identifier for HSI48 */
#define STM32_SRC_HSI48		(STM32_SRC_HSI + 1)
/* Bus clock */
/** @brief Clock source identifier for HCLK */
#define STM32_SRC_HCLK		(STM32_SRC_HSI48 + 1)
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
/* Clock muxes */
/** @brief Clock source identifier for CKPER */
#define STM32_SRC_CKPER		(STM32_SRC_PLL3_R + 1)


/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x088
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x08C
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x094
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x09c
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
#define CCIPR1_REG		0xD8
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0xDC
/** @brief RCC CCIPR3 register offset */
#define CCIPR3_REG		0xE0
/** @brief RCC CCIPR4 register offset */
#define CCIPR4_REG		0xE4
/** @brief RCC CCIPR5 register offset */
#define CCIPR5_REG		0xE8

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0xF0

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C

/** @brief Device domain clocks selection helpers */
/* CCIPR1 devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR1_REG)
/** @brief Kernel clock source selection for USART2 */
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, CCIPR1_REG)
/** @brief Kernel clock source selection for USART3 */
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 6, CCIPR1_REG)
/** @brief Kernel clock source selection for USART4 */
#define USART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 9, CCIPR1_REG)
/** @brief Kernel clock source selection for USART5 */
#define USART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR1_REG)
/** @brief Kernel clock source selection for USART6 */
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 15, CCIPR1_REG)
/** @brief Kernel clock source selection for USART7 */
#define USART7_SEL(val)		STM32_DT_CLOCK_SELECT((val), 20, 18, CCIPR1_REG)
/** @brief Kernel clock source selection for USART8 */
#define USART8_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 21, CCIPR1_REG)
/** @brief Kernel clock source selection for USART9 */
#define USART9_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR1_REG)
/** @brief Kernel clock source selection for USART10 */
#define USART10_SEL(val)	STM32_DT_CLOCK_SELECT((val), 29, 27, CCIPR1_REG)
/** @brief Kernel clock source selection for TIMIC */
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 31, CCIPR1_REG)

/* CCIPR2 devices */
/** @brief Kernel clock source selection for USART11 */
#define USART11_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR2_REG)
/** @brief Kernel clock source selection for USART12 */
#define USART12_SEL(val)	STM32_DT_CLOCK_SELECT((val), 6, 4, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 8, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM2 */
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM3 */
#define LPTIM3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM4 */
#define LPTIM4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM5 */
#define LPTIM5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR2_REG)
/** @brief Kernel clock source selection for LPTIM6 */
#define LPTIM6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 30, 28, CCIPR2_REG)

/* CCIPR3 devices */
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI2 */
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI3 */
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 6, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI4 */
#define SPI4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 9, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI5 */
#define SPI5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR3_REG)
/** @brief Kernel clock source selection for SPI6 */
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 15, CCIPR3_REG)
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR3_REG)

/* CCIPR4 devices */
/** @brief Kernel clock source selection for OCTOSPI1 */
#define OCTOSPI1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR4_REG)
/** @brief Kernel clock source selection for SYSTICK */
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR4_REG)
/** @brief Kernel clock source selection for USB */
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR4_REG)
/** @brief Kernel clock source selection for SDMMC1 */
#define SDMMC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CCIPR4_REG)
/** @brief Kernel clock source selection for SDMMC2 */
#define SDMMC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 7, CCIPR4_REG)
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR4_REG)
/** @brief Kernel clock source selection for I2C2 */
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR4_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CCIPR4_REG)
/** @brief Kernel clock source selection for I2C4 */
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR4_REG)
/** @brief Kernel clock source selection for I3C1 */
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 24, CCIPR4_REG)

/* CCIPR5 devices */
/** @brief Kernel clock source selection for ADCDAC */
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR5_REG)
/** @brief Kernel clock source selection for DAC */
#define DAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 3, CCIPR5_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR5_REG)
/** @brief Kernel clock source selection for CEC */
#define CEC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR5_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR5_REG)
/** @brief Kernel clock source selection for OCTOSPI2 */
#define OCTOSPI2_SEL(val)	STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR5_REG)
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR5_REG)
/** @brief Kernel clock source selection for SAI2 */
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 19, CCIPR5_REG)
/** @brief Kernel clock source selection for CKPER */
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 30, CCIPR5_REG)

/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/* CFGR1 devices */
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR1_REG)
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR1_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR1_REG)
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR1_REG)

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H5_CLOCK_H_ */
