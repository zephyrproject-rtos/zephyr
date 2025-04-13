/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32N6_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32N6_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0486, Figures 37 and 45 on clock distribution description */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI		(STM32_SRC_HSE + 1)
#define STM32_SRC_MSI		(STM32_SRC_HSI + 1)
/** PLL outputs */
#define STM32_SRC_PLL1		(STM32_SRC_MSI + 1)
#define STM32_SRC_PLL2		(STM32_SRC_PLL1 + 1)
#define STM32_SRC_PLL3		(STM32_SRC_PLL2 + 1)
#define STM32_SRC_PLL4		(STM32_SRC_PLL3 + 1)
/** Clock muxes */
#define STM32_SRC_CKPER		(STM32_SRC_PLL4 + 1)
#define STM32_SRC_IC1		(STM32_SRC_CKPER + 1)
#define STM32_SRC_IC2		(STM32_SRC_IC1 + 1)
#define STM32_SRC_IC3		(STM32_SRC_IC2 + 1)
#define STM32_SRC_IC4		(STM32_SRC_IC3 + 1)
#define STM32_SRC_IC5		(STM32_SRC_IC4 + 1)
#define STM32_SRC_IC6		(STM32_SRC_IC5 + 1)
#define STM32_SRC_IC7		(STM32_SRC_IC6 + 1)
#define STM32_SRC_IC8		(STM32_SRC_IC7 + 1)
#define STM32_SRC_IC9		(STM32_SRC_IC8 + 1)
#define STM32_SRC_IC10		(STM32_SRC_IC9 + 1)
#define STM32_SRC_IC11		(STM32_SRC_IC10 + 1)
#define STM32_SRC_IC12		(STM32_SRC_IC11 + 1)
#define STM32_SRC_IC13		(STM32_SRC_IC12 + 1)
#define STM32_SRC_IC14		(STM32_SRC_IC13 + 1)
#define STM32_SRC_IC15		(STM32_SRC_IC14 + 1)
#define STM32_SRC_IC16		(STM32_SRC_IC15 + 1)
#define STM32_SRC_IC17		(STM32_SRC_IC16 + 1)
#define STM32_SRC_IC18		(STM32_SRC_IC17 + 1)
#define STM32_SRC_IC19		(STM32_SRC_IC18 + 1)
#define STM32_SRC_IC20		(STM32_SRC_IC19 + 1)
#define STM32_SRC_HSI_DIV	(STM32_SRC_IC20 + 1)
#define STM32_SRC_TIMG		(STM32_SRC_HSI_DIV + 1)
#define STM32_SRC_HCLK1		(STM32_SRC_TIMG + 1)
#define STM32_SRC_HCLK2		(STM32_SRC_HCLK1 + 1)
#define STM32_SRC_HCLK3		(STM32_SRC_HCLK2 + 1)
#define STM32_SRC_HCLK4		(STM32_SRC_HCLK3 + 1)
#define STM32_SRC_HCLK5		(STM32_SRC_HCLK4 + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK5 + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK4		(STM32_SRC_PCLK2 + 1)
#define STM32_SRC_PCLK5		(STM32_SRC_PCLK4 + 1)

/** Others: Not yet supported */
/* #define STM32_SRC_I2SCKIN	TBD */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1	0x250
#define STM32_CLOCK_BUS_AHB2	0x254
#define STM32_CLOCK_BUS_AHB3	0x258
#define STM32_CLOCK_BUS_AHB4	0x25C
#define STM32_CLOCK_BUS_AHB5	0x260
#define STM32_CLOCK_BUS_APB1	0x264
#define STM32_CLOCK_BUS_APB1_2	0x268
#define STM32_CLOCK_BUS_APB2	0x26C
#define STM32_CLOCK_BUS_APB3	0x270
#define STM32_CLOCK_BUS_APB4	0x274
#define STM32_CLOCK_BUS_APB4_2	0x278
#define STM32_CLOCK_BUS_APB5	0x27C

#define STM32_CLOCK_LP_BUS_SHIFT	0x40

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB5

/** @brief RCC_CCIPRx register offset (RM0486.pdf) */
#define CCIPR1_REG		0x144
#define CCIPR2_REG		0x148
#define CCIPR3_REG		0x14C
#define CCIPR4_REG		0x150
#define CCIPR5_REG		0x154
#define CCIPR6_REG		0x158
#define CCIPR7_REG		0x15C
#define CCIPR8_REG		0x160
#define CCIPR9_REG		0x164
#define CCIPR12_REG		0x170
#define CCIPR13_REG		0x174
#define CCIPR14_REG		0x178

/** @brief Device domain clocks selection helpers */
/** CCIPR1 devices */
#define ADF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR1_REG)
#define ADC12_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR1_REG)
#define DCMIPP_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR1_REG)
/** CCIPR2 devices */
#define ETH1PTP_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR2_REG)
#define ETH1CLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR2_REG)
#define ETH1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR2_REG)
#define ETH1REFCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 20, CCIPR2_REG)
#define ETH1GTXCLK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 24, CCIPR2_REG)
/** CCIPR3 devices */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR3_REG)
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR3_REG)
/** CCIPR4 devices */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR4_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR4_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR4_REG)
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR4_REG)
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR4_REG)
#define I3C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR4_REG)
#define LTDC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 24, CCIPR4_REG)
/** CCIPR5 devices */
#define MCO1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR5_REG)
#define MCO2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR5_REG)
#define MDF1SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR5_REG)
/** CCIPR6 devices */
#define XSPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR6_REG)
#define XSPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR6_REG)
#define XSPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR6_REG)
#define OTGPHY1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR6_REG)
#define OTGPHY1CKREF_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 16, CCIPR6_REG)
#define OTGPHY2_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR6_REG)
#define OTGPHY2CKREF_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 24, CCIPR6_REG)
/** CCIPR7 devices */
#define PER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR7_REG)
#define PSSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR7_REG)
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR7_REG)
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR7_REG)
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR7_REG)
/** CCIPR8 devices */
#define SDMMC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR8_REG)
#define SDMMC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR8_REG)
/** CCIPR9 devices */
#define SPDIFRX1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR9_REG)
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR9_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR9_REG)
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR9_REG)
#define SPI4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR9_REG)
#define SPI5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR9_REG)
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR9_REG)
/** CCIPR12 devices */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR12_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR12_REG)
#define LPTIM3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR12_REG)
#define LPTIM4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR12_REG)
#define LPTIM5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR12_REG)
/** CCIPR13 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR13_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR13_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR13_REG)
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR13_REG)
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 16, CCIPR13_REG)
#define USART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 20, CCIPR13_REG)
#define UART7_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 24, CCIPR13_REG)
#define UART8_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 28, CCIPR13_REG)
/** CCIPR14 devices */
#define UART9_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 0, CCIPR14_REG)
#define USART10_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR14_REG)
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR14_REG)

/** @brief RCC_ICxCFGR register offset (RM0486.pdf) */
#define ICxCFGR_REG(ic)		(0xC4 + ((ic) - 1) * 4)

/** @brief Divider ICx source selection */
#define ICx_PLLy_SEL(ic, pll)	STM32_DT_CLOCK_SELECT((pll) - 1, 3, 28, ICxCFGR_REG(ic))

/** @brief RCC_CFGR1 register offset (RM0486.pdf) */
#define CFGR1_REG		0x20

/** @brief CPU clock switch selection */
#define CPU_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, CFGR1_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32N6_CLOCK_H_ */
