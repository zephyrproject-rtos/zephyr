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
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSI_KER	(STM32_SRC_HSI48 + 1) /* HSI + HSIKERON */
#define STM32_SRC_CSI_KER	(STM32_SRC_HSI_KER + 1) /* CSI + CSIKERON */
/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_CSI_KER + 1)
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
/** Others: Not yet supported */
/* #define STM32_SRC_I2SCKIN	TBD */
/* #define STM32_SRC_SPDIFRX	TBD */


/** Bus clocks */
#define STM32_CLOCK_BUS_AHB3    0x0D4
#define STM32_CLOCK_BUS_AHB1    0x0D8
#define STM32_CLOCK_BUS_AHB2    0x0DC
#define STM32_CLOCK_BUS_AHB4    0x0E0
#define STM32_CLOCK_BUS_APB3    0x0E4
#define STM32_CLOCK_BUS_APB1    0x0E8
#define STM32_CLOCK_BUS_APB1_2  0x0EC
#define STM32_CLOCK_BUS_APB2    0x0F0
#define STM32_CLOCK_BUS_APB4    0x0F4
/** Alias D1/2/3 domains clocks */ /* TBD: To remove ? */
#define STM32_SRC_PCLK1		STM32_CLOCK_BUS_APB1
#define STM32_SRC_PCLK2		STM32_CLOCK_BUS_APB2
#define STM32_SRC_HCLK3		STM32_CLOCK_BUS_AHB3
#define STM32_SRC_PCLK3		STM32_CLOCK_BUS_APB3
#define STM32_SRC_PCLK4		STM32_CLOCK_BUS_APB4

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB3
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB4

/** @brief RCC_DxCCIP register offset (RM0399.pdf) */
#define D1CCIPR_REG		0x4C
#define D2CCIP1R_REG		0x50
#define D2CCIP2R_REG		0x54
#define D3CCIPR_REG		0x58

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x10

/** @brief Device domain clocks selection helpers (RM0399.pdf) */
/** D1CCIPR devices */
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, D1CCIPR_REG)
#define QSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, D1CCIPR_REG)
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 8, D1CCIPR_REG)
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 16, D1CCIPR_REG)
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 28, D1CCIPR_REG)
/* Device domain clocks selection helpers (RM0468.pdf) */
#define OSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, D1CCIPR_REG)
/** D2CCIP1R devices */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, D2CCIP1R_REG)
#define SAI23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 6, D2CCIP1R_REG)
#define SPI123_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, D2CCIP1R_REG)
#define SPI45_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, D2CCIP1R_REG)
#define SPDIF_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, D2CCIP1R_REG)
#define DFSDM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 24, D2CCIP1R_REG)
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 28, D2CCIP1R_REG)
#define SWP_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 31, D2CCIP1R_REG)
/** D2CCIP2R devices */
#define USART2345678_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 0, D2CCIP2R_REG)
#define USART16_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 3, D2CCIP2R_REG)
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, D2CCIP2R_REG)
#define I2C123_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, D2CCIP2R_REG)
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, D2CCIP2R_REG)
#define CEC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 22, D2CCIP2R_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 28, D2CCIP2R_REG)
/** D3CCIPR devices */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 0, D3CCIPR_REG)
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, D3CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 10, D3CCIPR_REG)
#define LPTIM345_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 13, D3CCIPR_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, D3CCIPR_REG)
#define SAI4A_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 21, D3CCIPR_REG)
#define SAI4B_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, D3CCIPR_REG)
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 28, D3CCIPR_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)
/** CFGR devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0xF, 22, CFGR_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 18, CFGR_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0xF, 29, CFGR_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 25, CFGR_REG)

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_ */
