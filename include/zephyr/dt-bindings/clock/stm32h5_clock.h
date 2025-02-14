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
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_CSI		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSI		(STM32_SRC_CSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSI + 1)
/** Bus clock */
#define STM32_SRC_HCLK		(STM32_SRC_HSI48 + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK3		(STM32_SRC_PCLK2 + 1)
/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_PCLK3 + 1)
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_R + 1)
#define STM32_SRC_PLL2_Q	(STM32_SRC_PLL2_P + 1)
#define STM32_SRC_PLL2_R	(STM32_SRC_PLL2_Q + 1)
#define STM32_SRC_PLL3_P	(STM32_SRC_PLL2_R + 1)
#define STM32_SRC_PLL3_Q	(STM32_SRC_PLL3_P + 1)
#define STM32_SRC_PLL3_R	(STM32_SRC_PLL3_Q + 1)
/** Clock muxes */
#define STM32_SRC_CKPER		(STM32_SRC_PLL3_R + 1)


/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB4    0x094
#define STM32_CLOCK_BUS_APB1    0x09c
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** @brief RCC_CCIPRx register offset (RM0456.pdf) */
#define CCIPR1_REG		0xD8
#define CCIPR2_REG		0xDC
#define CCIPR3_REG		0xE0
#define CCIPR4_REG		0xE4
#define CCIPR5_REG		0xE8

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0xF0

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C

/** @brief Device domain clocks selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR1_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 3, CCIPR1_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR1_REG)
#define USART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 9, CCIPR1_REG)
#define USART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR1_REG)
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 15, CCIPR1_REG)
#define USART7_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 18, CCIPR1_REG)
#define USART8_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 21, CCIPR1_REG)
#define USART9_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR1_REG)
#define USART10_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 27, CCIPR1_REG)
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 31, CCIPR1_REG)

/** CCIPR2 devices */
#define USART11_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR2_REG)
#define USART12_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR2_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR2_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR2_REG)
#define LPTIM3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR2_REG)
#define LPTIM4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR2_REG)
#define LPTIM5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR2_REG)
#define LPTIM6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 28, CCIPR2_REG)

/** CCIPR3 devices */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR3_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 3, CCIPR3_REG)
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, CCIPR3_REG)
#define SPI4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 9, CCIPR3_REG)
#define SPI5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR3_REG)
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 15, CCIPR2_REG)
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR3_REG)

/** CCIPR4 devices */
#define OCTOSPI1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR4_REG)
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR4_REG)
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR4_REG)
#define SDMMC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 6, CCIPR4_REG)
#define SDMMC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 7, CCIPR4_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, CCIPR4_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 18, CCIPR4_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR4_REG)
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 22, CCIPR4_REG)
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 24, CCIPR4_REG)

/** CCIPR5 devices */
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR5_REG)
#define DAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 3, CCIPR5_REG)
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR5_REG)
#define CEC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 6, CCIPR5_REG)
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR5_REG)
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR5_REG)
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 19, CCIPR5_REG)
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 30, CCIPR5_REG)

/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)

/** CFGR1 devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 22, CFGR1_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0xF, 18, CFGR1_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 25, CFGR1_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0xF, 29, CFGR1_REG)

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H5_CLOCK_H_ */
