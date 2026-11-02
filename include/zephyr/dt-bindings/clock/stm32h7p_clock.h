/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7P_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7P_CLOCK_H_

#include "stm32_common_clocks.h"

/** @cond INTERNAL_HIDDEN */

/** Domain clocks */

/* RM0533 Rev 1 Table 60 "Peripheral clock distribution summary" */

/** System clock */
/* defined in stm32_common_clocks.h */

/** Fixed clocks */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSI		(STM32_SRC_HSI48 + 1) /* hsi_ker_ck */
#define STM32_SRC_CSI		(STM32_SRC_HSI + 1) /* csi_ker_ck */
/* TODO: audio_ck support
 * #define STM32_SRC_AUDIOCLK	(STM32_SRC_CSI + 1)
 */

/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_CSI + 1)
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL1_S	(STM32_SRC_PLL1_Q + 1)
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_S + 1)
#define STM32_SRC_PLL2_Q	(STM32_SRC_PLL2_P + 1)
#define STM32_SRC_PLL2_R	(STM32_SRC_PLL2_Q + 1)
#define STM32_SRC_PLL2_S	(STM32_SRC_PLL2_R + 1)
#define STM32_SRC_PLL2_T	(STM32_SRC_PLL2_S + 1)
#define STM32_SRC_PLL3_P	(STM32_SRC_PLL2_T + 1)
#define STM32_SRC_PLL3_Q	(STM32_SRC_PLL3_P + 1)
#define STM32_SRC_PLL3_R	(STM32_SRC_PLL3_Q + 1)
#define STM32_SRC_PLL3_S	(STM32_SRC_PLL3_R + 1)

/** Clock muxes */
#define STM32_SRC_CKPER		(STM32_SRC_PLL3_S + 1) /* per_ck */
#define STM32_SRC_TIMCK		(STM32_SRC_CKPER + 1) /* tim_ck = rcc_tim_ker_ck = sys_cpu_ck */
#define STM32_SRC_ADCKERCK	(STM32_SRC_TIMCK + 1) /* rcc_adc_ker_ck */
#define STM32_SRC_HCLKN		(STM32_SRC_ADCKERCK + 1) /* rcc_hclk[5:1] = sys_bus_ck */
#define STM32_SRC_HCLK1		STM32_SRC_HCLKN
#define STM32_SRC_HCLK2		STM32_SRC_HCLKN
#define STM32_SRC_HCLK3		STM32_SRC_HCLKN
#define STM32_SRC_HCLK4		STM32_SRC_HCLKN
#define STM32_SRC_HCLK5		STM32_SRC_HCLKN
#define STM32_SRC_PCLK1		(STM32_SRC_HCLKN + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK4		(STM32_SRC_PCLK2 + 1)

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB5    0x134
#define STM32_CLOCK_BUS_AHB1    0x138
#define STM32_CLOCK_BUS_AHB2    0x13C
#define STM32_CLOCK_BUS_AHB4    0x140
#define STM32_CLOCK_BUS_APB1    0x148
#define STM32_CLOCK_BUS_APB1_2  0x14C
#define STM32_CLOCK_BUS_APB2    0x150
#define STM32_CLOCK_BUS_APB4    0x154
#define STM32_CLOCK_BUS_AHB3    0x158

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB5
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_AHB3

/** @brief RCC_CCIPRx register offset */
#define CCIPR1_REG		0x4C
#define CCIPR2_REG		0x50
#define CCIPR3_REG		0x54
#define CCIPR4_REG		0x58

/** @brief RCC_BDCR register offset */
#define BCDR_REG		0x70

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x10

/** @brief Device domain clocks selection helpers */
/** CCIPR1 devices */
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR1_REG)
#define SDMMC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 2, CCIPR1_REG)
#define XSPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR1_REG)
#define OTGFS_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 14, CCIPR1_REG)
#define ADCDIV(val)		STM32_DT_CLOCK_SELECT((val), 19, 16, CCIPR1_REG)
#define ADF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR1_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR1_REG)
#define CKPER_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 28, CCIPR1_REG)
/** CCIPR2 devices */
#define UART234578_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR2_REG)
#define SPI23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, CCIPR2_REG)
#define I2C123_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR2_REG)
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR2_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR2_REG)
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR2_REG)
/** CCIPR3 devices */
#define USART16910_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR3_REG)
#define SPI45_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, CCIPR3_REG)
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 8, CCIPR3_REG)
/** CCIPR4 devices */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR4_REG)
#define SPI6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 4, CCIPR4_REG)
#define LPTIM23_SEL(val)	STM32_DT_CLOCK_SELECT((val), 10, 8, CCIPR4_REG)
#define LPTIM45_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR4_REG)
#define DAC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR4_REG)
#define DAC1SH_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 19, CCIPR4_REG)
#define DAC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR4_REG)
#define DAC3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR4_REG)
#define DAC4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 30, 28, CCIPR4_REG)
/** BCDR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BCDR_REG)

/** CFGR devices */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR_REG)
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR_REG)

/* MCO prescaler: division factor */
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

/* MCO1 clock output */
#define MCO1_SEL_HSI		0
#define MCO1_SEL_LSE		1
#define MCO1_SEL_HSE		2
#define MCO1_SEL_PLL1QCLK	3
#define MCO1_SEL_HSI48		4

/* MCO2 clock output */
#define MCO2_SEL_SYSCLK		0
#define MCO2_SEL_PLL2PCLK	1
#define MCO2_SEL_HSE		2
#define MCO2_SEL_PLL1PCLK	3
#define MCO2_SEL_CSI		4
#define MCO2_SEL_LSI		5

/** INTERNAL_HIDDEN @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7P_CLOCK_H_ */
