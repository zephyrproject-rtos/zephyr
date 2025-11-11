/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U3_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U3_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0487, Figure 36 Clock tree for STM32U3 Series */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI16		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSI16 + 1)
#define STM32_SRC_MSIS		(STM32_SRC_HSI48 + 1)
#define STM32_SRC_MSIK		(STM32_SRC_MSIS + 1)
/** Bus clock */
#define STM32_SRC_HCLK		(STM32_SRC_MSIK + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK3		(STM32_SRC_PCLK2 + 1)
/** Clock muxes */
/* #define STM32_SRC_ICLK	TBD */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB1_2  0x094
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB2_2  0x090
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** @brief RCC_CCIPRx register offset (RM0487.pdf) */
#define CCIPR1_REG		0x100
#define CCIPR2_REG		0x104
#define CCIPR3_REG		0x108

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x110

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x0C

/** @brief Device domain clocks selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 0, 0, CCIPR1_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 2, CCIPR1_REG)
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 4, CCIPR1_REG)
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CCIPR1_REG)
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 8, CCIPR1_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 10, CCIPR1_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 12, 12, CCIPR1_REG)
#define I3C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 14, CCIPR1_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 16, 16, CCIPR1_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR1_REG)
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 20, 20, CCIPR1_REG)
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR1_REG)
#define FDCAN1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 24, CCIPR1_REG)
#define ICLK_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR1_REG)
#define USB1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 28, 28, CCIPR1_REG)
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 29, CCIPR1_REG)
/** CCIPR2 devices */
#define ADF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR2_REG)
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 3, CCIPR2_REG)
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 5, CCIPR2_REG)
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 11, CCIPR2_REG)
#define ADCDAC_PRE(val)		STM32_DT_CLOCK_SELECT((val), 15, 12, CCIPR2_REG)
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR2_REG)
#define DAC1SH_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 19, CCIPR2_REG)
#define OCTOSPI_SEL(val)	STM32_DT_CLOCK_SELECT((val), 20, 20, CCIPR2_REG)
/** CCIPR3 devices */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR3_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CCIPR3_REG)
#define LPTIM34_SEL(val)	STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR3_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/** CFGR1 devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 27, 24, CFGR1_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CFGR1_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1   0
#define MCO_PRE_DIV_2   1
#define MCO_PRE_DIV_4   2
#define MCO_PRE_DIV_8   3
#define MCO_PRE_DIV_16  4
#define MCO_PRE_DIV_32  5
#define MCO_PRE_DIV_64  6
#define MCO_PRE_DIV_128 7

/* ADC/DAC prescaler division factor */
#define ADCDAC_PRE_DIV_1	0x0
#define ADCDAC_PRE_DIV_2	0x1
#define ADCDAC_PRE_DIV_4	0x8
#define ADCDAC_PRE_DIV_8	0x9
#define ADCDAC_PRE_DIV_16	0xA
#define ADCDAC_PRE_DIV_32	0xB
#define ADCDAC_PRE_DIV_64	0xC
#define ADCDAC_PRE_DIV_128	0xD
#define ADCDAC_PRE_DIV_256	0xE
#define ADCDAC_PRE_DIV_512	0xF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U3_CLOCK_H_ */
