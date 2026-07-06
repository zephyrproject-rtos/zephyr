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
/** Clock muxes */
/* #define STM32_SRC_ICLK	TBD */

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0x088
/** @brief AHB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1_2  0x094
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x08C
/** @brief AHB2_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2_2  0x090
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

/** @brief RCC_CCIPRx register offset (RM0487.pdf) */
#define CCIPR1_REG		0x100
/** @brief RCC CCIPR2 register offset */
#define CCIPR2_REG		0x104
/** @brief RCC CCIPR3 register offset */
#define CCIPR3_REG		0x108

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x110

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x0C

/** @brief Device domain clocks selection helpers */
/* CCIPR1 devices */
/** @brief Kernel clock source selection for USART1 */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 0, 0, CCIPR1_REG)
/** @brief Kernel clock source selection for USART3 */
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 2, CCIPR1_REG)
/** @brief Kernel clock source selection for UART4 */
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 4, CCIPR1_REG)
/** @brief Kernel clock source selection for UART5 */
#define UART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CCIPR1_REG)
/** @brief Kernel clock source selection for I3C1 */
#define I3C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 8, CCIPR1_REG)
/** @brief Kernel clock source selection for I2C1 */
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 10, CCIPR1_REG)
/** @brief Kernel clock source selection for I2C2 */
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 12, 12, CCIPR1_REG)
/** @brief Kernel clock source selection for I3C2 */
#define I3C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 14, 14, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI2 */
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 16, 16, CCIPR1_REG)
/** @brief Kernel clock source selection for LPTIM2 */
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CCIPR1_REG)
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 20, 20, CCIPR1_REG)
/** @brief Kernel clock source selection for SYSTICK */
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 23, 22, CCIPR1_REG)
/** @brief Kernel clock source selection for FDCAN1 */
#define FDCAN1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 24, CCIPR1_REG)
/** @brief Kernel clock source selection for ICLK */
#define ICLK_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 26, CCIPR1_REG)
/** @brief Kernel clock source selection for USB1 */
#define USB1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 28, 28, CCIPR1_REG)
/** @brief Kernel clock source selection for TIMIC */
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 29, CCIPR1_REG)
/* CCIPR2 devices */
/** @brief Kernel clock source selection for ADF1 */
#define ADF1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR2_REG)
/** @brief Kernel clock source selection for SPI3 */
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 3, CCIPR2_REG)
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 5, CCIPR2_REG)
/** @brief Kernel clock source selection for RNG */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 11, CCIPR2_REG)
/** @brief Clock prescaler selection for ADCDAC */
#define ADCDAC_PRE(val)		STM32_DT_CLOCK_SELECT((val), 15, 12, CCIPR2_REG)
/** @brief Kernel clock source selection for ADCDAC */
#define ADCDAC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CCIPR2_REG)
/** @brief Kernel clock source selection for DAC1SH */
#define DAC1SH_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 19, CCIPR2_REG)
/** @brief Kernel clock source selection for OCTOSPI */
#define OCTOSPI_SEL(val)	STM32_DT_CLOCK_SELECT((val), 20, 20, CCIPR2_REG)
/* CCIPR3 devices */
/** @brief Kernel clock source selection for LPUART1 */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 0, CCIPR3_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CCIPR3_REG)
/** @brief Kernel clock source selection for LPTIM34 */
#define LPTIM34_SEL(val)	STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR3_REG)
/** @brief Kernel clock source selection for LPTIM1 */
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 10, CCIPR3_REG)
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
#define MCO_PRE_DIV_1   0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2   1
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4   2
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8   3
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16  4
/** @brief MCO prescaler division factor: 32 */
#define MCO_PRE_DIV_32  5
/** @brief MCO prescaler division factor: 64 */
#define MCO_PRE_DIV_64  6
/** @brief MCO prescaler division factor: 128 */
#define MCO_PRE_DIV_128 7

/* ADC/DAC prescaler division factor */
/** @brief ADC/DAC prescaler division factor: 1 */
#define ADCDAC_PRE_DIV_1	0x0
/** @brief ADC/DAC prescaler division factor: 2 */
#define ADCDAC_PRE_DIV_2	0x1
/** @brief ADC/DAC prescaler division factor: 4 */
#define ADCDAC_PRE_DIV_4	0x8
/** @brief ADC/DAC prescaler division factor: 8 */
#define ADCDAC_PRE_DIV_8	0x9
/** @brief ADC/DAC prescaler division factor: 16 */
#define ADCDAC_PRE_DIV_16	0xA
/** @brief ADC/DAC prescaler division factor: 32 */
#define ADCDAC_PRE_DIV_32	0xB
/** @brief ADC/DAC prescaler division factor: 64 */
#define ADCDAC_PRE_DIV_64	0xC
/** @brief ADC/DAC prescaler division factor: 128 */
#define ADCDAC_PRE_DIV_128	0xD
/** @brief ADC/DAC prescaler division factor: 256 */
#define ADCDAC_PRE_DIV_256	0xE
/** @brief ADC/DAC prescaler division factor: 512 */
#define ADCDAC_PRE_DIV_512	0xF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U3_CLOCK_H_ */
