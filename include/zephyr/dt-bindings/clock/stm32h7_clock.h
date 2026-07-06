/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0468, Table 56 Kernel clock dictribution summary */

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
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_CKPER + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
/** Others: Not yet supported */
/* #define STM32_SRC_I2SCKIN	TBD */
/* #define STM32_SRC_SPDIFRX	TBD */


/* Bus clocks */
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    0x0D4
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x0D8
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x0DC
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x0E0
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3    0x0E4
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x0E8
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  0x0EC
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x0F0
/** @brief APB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4    0x0F4
/** Alias D1/2/3 domains clocks */ /* TBD: To remove ? */
#define STM32_SRC_PCLK1		STM32_CLOCK_BUS_APB1
/** @brief Clock source identifier for PCLK2 */
#define STM32_SRC_PCLK2		STM32_CLOCK_BUS_APB2
/** @brief Clock source identifier for HCLK3 */
#define STM32_SRC_HCLK3		STM32_CLOCK_BUS_AHB3
/** @brief Clock source identifier for PCLK3 */
#define STM32_SRC_PCLK3		STM32_CLOCK_BUS_APB3
/** @brief Clock source identifier for PCLK4 */
#define STM32_SRC_PCLK4		STM32_CLOCK_BUS_APB4

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB3
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB4

/** @brief RCC_DxCCIP register offset (RM0399.pdf) */
#define D1CCIPR_REG		0x4C
/** @brief RCC D2CCIP1R register offset */
#define D2CCIP1R_REG		0x50
/** @brief RCC D2CCIP2R register offset */
#define D2CCIP2R_REG		0x54
/** @brief RCC D3CCIPR register offset */
#define D3CCIPR_REG		0x58

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x10

/** @brief Device domain clocks selection helpers (RM0399.pdf) */
/* D1CCIPR devices */
/** @brief Kernel clock source selection for FMC */
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, D1CCIPR_REG)
/** @brief Kernel clock source selection for QSPI */
#define QSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, D1CCIPR_REG)
/** @brief Kernel clock source selection for DSI */
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 8, D1CCIPR_REG)
/** @brief Kernel clock source selection for SDMMC */
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 16, 16, D1CCIPR_REG)
/** @brief Kernel clock source selection for CKPER */
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, D1CCIPR_REG)
/* Device domain clocks selection helpers (RM0468.pdf) */
/** @brief Kernel clock source selection for OSPI */
#define OSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, D1CCIPR_REG)
/* D2CCIP1R devices */
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, D2CCIP1R_REG)
/** @brief Kernel clock source selection for SAI23 */
#define SAI23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 6, D2CCIP1R_REG)
/** @brief Kernel clock source selection for SPI123 */
#define SPI123_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 12, D2CCIP1R_REG)
/** @brief Kernel clock source selection for SPI45 */
#define SPI45_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, D2CCIP1R_REG)
/** @brief Kernel clock source selection for SPDIF */
#define SPDIF_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, D2CCIP1R_REG)
/** @brief Kernel clock source selection for DFSDM1 */
#define DFSDM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 24, D2CCIP1R_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, D2CCIP1R_REG)
/** @brief Kernel clock source selection for SWP */
#define SWP_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 31, D2CCIP1R_REG)
/* D2CCIP2R devices */
/** @brief Kernel clock source selection for USART2345678 */
#define USART2345678_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, D2CCIP2R_REG)
/** @brief Kernel clock source selection for USART16 */
#define USART16_SEL(val)	STM32_DT_CLOCK_SELECT((val), 5, 3, D2CCIP2R_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, D2CCIP2R_REG)
/** @brief Kernel clock source selection for I2C123 */
#define I2C123_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, D2CCIP2R_REG)
/** @brief Kernel clock source selection for USB */
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, D2CCIP2R_REG)
/** @brief Kernel clock source selection for CEC */
#define CEC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, D2CCIP2R_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 30, 28, D2CCIP2R_REG)
/* D3CCIPR devices */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, D3CCIPR_REG)
/** @brief Kernel clock source selection for I2C4 */
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, D3CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM2 */
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 12, 10, D3CCIPR_REG)
/** @brief Kernel clock source selection for LPTIM345 */
#define LPTIM345_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 13, D3CCIPR_REG)
/** @brief Kernel clock source selection for ADC */
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, D3CCIPR_REG)
/** @brief Kernel clock source selection for SAI4A */
#define SAI4A_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 21, D3CCIPR_REG)
/** @brief Kernel clock source selection for SAI4B */
#define SAI4B_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 24, D3CCIPR_REG)
/** @brief Kernel clock source selection for SPI6 */
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 30, 28, D3CCIPR_REG)
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

/* MCO1 clock output */
/** @brief MCO1 clock source selection value: HSI */
#define MCO1_SEL_HSI		0
/** @brief MCO1 clock source selection value: LSE */
#define MCO1_SEL_LSE		1
/** @brief MCO1 clock source selection value: HSE */
#define MCO1_SEL_HSE		2
/** @brief MCO1 clock source selection value: PLL1QCLK */
#define MCO1_SEL_PLL1QCLK	3
/** @brief MCO1 clock source selection value: HSI48 */
#define MCO1_SEL_HSI48		4

/* MCO2 clock output */
/** @brief MCO2 clock source selection value: SYSCLK */
#define MCO2_SEL_SYSCLK		0
/** @brief MCO2 clock source selection value: PLL2PCLK */
#define MCO2_SEL_PLL2PCLK	1
/** @brief MCO2 clock source selection value: HSE */
#define MCO2_SEL_HSE		2
/** @brief MCO2 clock source selection value: PLL1PCLK */
#define MCO2_SEL_PLL1PCLK	3
/** @brief MCO2 clock source selection value: CSI */
#define MCO2_SEL_CSI		4
/** @brief MCO2 clock source selection value: LSI */
#define MCO2_SEL_LSI		5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_ */
