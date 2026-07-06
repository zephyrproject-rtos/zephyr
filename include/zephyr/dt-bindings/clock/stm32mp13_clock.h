/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP13_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP13_CLOCK_H_

/** @cond INTERNAL_HIDDEN */

#include "stm32_common_clocks.h"

/** System clock */
/* defined in stm32_common_clocks.h */
/* Fixed clocks */
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_HSE + 1)

/* PLL outputs */
/** @brief Clock source identifier for PLL1_P */
#define STM32_SRC_PLL1_P	(STM32_SRC_HSI + 1)
/** @brief Clock source identifier for PLL2_P */
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_P + 1)
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
/** @brief Clock source identifier for PLL4_P */
#define STM32_SRC_PLL4_P	(STM32_SRC_PLL3_R + 1)
/** @brief Clock source identifier for PLL4_Q */
#define STM32_SRC_PLL4_Q	(STM32_SRC_PLL4_P + 1)
/** @brief Clock source identifier for PLL4_R */
#define STM32_SRC_PLL4_R	(STM32_SRC_PLL4_Q + 1)

/* Bus clocks */
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    0x700
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    0x708
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3    0x710
/** @brief APB3_S bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3_S  0x718
/** @brief APB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4    0x728
/** @brief APB4_NS bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4_NS	0x738
/** @brief APB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB5    0x740
/** @brief APB6 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB6    0x748
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    0x750
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    0x768
/** @brief AHB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB5    0x778
/** @brief AHB6 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB6    0x780

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_APB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_AHB6

/** @brief Device domain clocks selection helpers */
#define BDCR_REG                0x400
#define MCO1CFGR_REG		0x460
/** @brief RCC MCO2CFGR register offset */
#define MCO2CFGR_REG		0x464
/** @brief RCC I2C12CKSELR register offset */
#define I2C12CKSELR_REG		0x600
/** @brief RCC I2C345CKSELR register offset */
#define I2C345CKSELR_REG	0x604
/** @brief RCC SPI2S1CKSELR register offset */
#define SPI2S1CKSELR_REG	0x608
/** @brief RCC SPI2S23CKSELR register offset */
#define SPI2S23CKSELR_REG	0x60c
/** @brief RCC SPI45CKSELR register offset */
#define SPI45CKSELR_REG		0x610
/** @brief RCC UART12CKSELR register offset */
#define UART12CKSELR_REG	0x614
/** @brief RCC UART35CKSELR register offset */
#define UART35CKSELR_REG	0x618
/** @brief RCC UART4CKSELR register offset */
#define UART4CKSELR_REG		0x61c
/** @brief RCC UART6CKSELR register offset */
#define UART6CKSELR_REG		0x620
/** @brief RCC UART78CKSELR register offset */
#define UART78CKSELR_REG	0x624
/** @brief RCC LPTIM1CKSELR register offset */
#define LPTIM1CKSELR_REG	0x628
/** @brief RCC LPTIM23CKSELR register offset */
#define LPTIM23CKSELR_REG	0x62c
/** @brief RCC LPTIM45CKSELR register offset */
#define LPTIM45CKSELR_REG	0x630
/** @brief RCC SAI1CKSELR register offset */
#define SAI1CKSELR_REG		0x634
/** @brief RCC SAI2CKSELR register offset */
#define SAI2CKSELR_REG		0x638
/** @brief RCC FDCANCKSELR register offset */
#define FDCANCKSELR_REG		0x63c
/** @brief RCC SPDIFCKSELR register offset */
#define SPDIFCKSELR_REG		0x640
/** @brief RCC ADC12CKSELR register offset */
#define ADC12CKSELR_REG		0x644
/** @brief RCC SDMMC12CKSELR register offset */
#define SDMMC12CKSELR_REG	0x648
/** @brief RCC ETH12CKSELR register offset */
#define ETH12CKSELR_REG		0x64c
/** @brief RCC USBCKSELR register offset */
#define USBCKSELR_REG		0x650
/** @brief RCC QSPICKSELR register offset */
#define QSPICKSELR_REG		0x654
/** @brief RCC FMCCKSELR register offset */
#define FMCCKSELR_REG		0x658
/** @brief RCC RNG1CKSELR register offset */
#define RNG1CKSELR_REG		0x65c
/** @brief RCC STGENCKSELR register offset */
#define STGENCKSELR_REG		0x660
/** @brief RCC DCMIPPCKSELR register offset */
#define DCMIPPCKSELR_REG	0x664
/** @brief RCC SAESCKSELR register offset */
#define SAESCKSELR_REG		0x668

/** MCO1CFGR / MCO2CFGR devices */
#define MCO1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, MCO1CFGR_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, MCO1CFGR_REG)
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, MCO2CFGR_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val)		STM32_DT_CLOCK_SELECT((val), 7, 4, MCO2CFGR_REG)

/** @brief MCO output enable bit */
#define MCOX_ON	BIT(12)

/* MCO1 source */
/** @brief MCO1 clock source selection value: HSI */
#define MCO1_SEL_HSI	0
/** @brief MCO1 clock source selection value: HSE */
#define MCO1_SEL_HSE	1
/** @brief MCO1 clock source selection value: CSI */
#define MCO1_SEL_CSI	2
/** @brief MCO1 clock source selection value: LSI */
#define MCO1_SEL_LSI	3
/** @brief MCO1 clock source selection value: LSE */
#define MCO1_SEL_LSE	4

/* MCO2 source */
/** @brief MCO2 clock source selection value: MPU */
#define MCO2_SEL_MPU	0
/** @brief MCO2 clock source selection value: AXI */
#define MCO2_SEL_AXI	1
/** @brief MCO2 clock source selection value: MLAHB */
#define MCO2_SEL_MLAHB	2
/** @brief MCO2 clock source selection value: PLL4 */
#define MCO2_SEL_PLL4	3
/** @brief MCO2 clock source selection value: HSE */
#define MCO2_SEL_HSE	4
/** @brief MCO2 clock source selection value: HSI */
#define MCO2_SEL_HSI	5

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1	0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2	1
/** @brief MCO prescaler division factor: 3 */
#define MCO_PRE_DIV_3	2
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4	3
/** @brief MCO prescaler division factor: 5 */
#define MCO_PRE_DIV_5	4
/** @brief MCO prescaler division factor: 6 */
#define MCO_PRE_DIV_6	5
/** @brief MCO prescaler division factor: 7 */
#define MCO_PRE_DIV_7	6
/** @brief MCO prescaler division factor: 8 */
#define MCO_PRE_DIV_8	7
/** @brief MCO prescaler division factor: 9 */
#define MCO_PRE_DIV_9	8
/** @brief MCO prescaler division factor: 10 */
#define MCO_PRE_DIV_10	9
/** @brief MCO prescaler division factor: 11 */
#define MCO_PRE_DIV_11	10
/** @brief MCO prescaler division factor: 12 */
#define MCO_PRE_DIV_12	11
/** @brief MCO prescaler division factor: 13 */
#define MCO_PRE_DIV_13	12
/** @brief MCO prescaler division factor: 14 */
#define MCO_PRE_DIV_14	13
/** @brief MCO prescaler division factor: 15 */
#define MCO_PRE_DIV_15	14
/** @brief MCO prescaler division factor: 16 */
#define MCO_PRE_DIV_16	15

/** @brief Kernel clock source selection for I2C12 */
#define I2C12_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, I2C12CKSELR_REG)
/** @brief Kernel clock source selection for I2C3 */
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, I2C345CKSELR_REG)
/** @brief Kernel clock source selection for I2C4 */
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, I2C345CKSELR_REG)
/** @brief Kernel clock source selection for I2C5 */
#define I2C5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 6, I2C345CKSELR_REG)
/** @brief Kernel clock source selection for SPI1 */
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SPI2S1CKSELR_REG)
/** @brief Kernel clock source selection for SPI23 */
#define SPI23_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SPI2S23CKSELR_REG)
/** @brief Kernel clock source selection for SPI4 */
#define SPI4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SPI45CKSELR_REG)
/** @brief Kernel clock source selection for SPI5 */
#define SPI5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, SPI45CKSELR_REG)
/** @brief Kernel clock source selection for UART1 */
#define UART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, UART12CKSELR_REG)
/** @brief Kernel clock source selection for UART2 */
#define UART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, UART12CKSELR_REG)
/** @brief Kernel clock source selection for UART35 */
#define UART35_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, UART35CKSELR_REG)
/** @brief Kernel clock source selection for UART4 */
#define UART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, UART4CKSELR_REG)
/** @brief Kernel clock source selection for UART6 */
#define UART6_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, UART6CKSELR_REG)
/** @brief Kernel clock source selection for UART78 */
#define UART78_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, UART78CKSELR_REG)
/** @brief Kernel clock source selection for LPTIME1 */
#define LPTIME1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, LPTIM1CKSELR_REG)
/** @brief Kernel clock source selection for LPTIME2 */
#define LPTIME2_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, LPTIM23CKSELR_REG)
/** @brief Kernel clock source selection for LPTIME3 */
#define LPTIME3_SEL(val)	STM32_DT_CLOCK_SELECT((val), 5, 3, LPTIM23CKSELR_REG)
/** @brief Kernel clock source selection for LPTIME45 */
#define LPTIME45_SEL(val)	STM32_DT_CLOCK_SELECT((val), 2, 0, LPTIM45CKSELR_REG)
/** @brief Kernel clock source selection for SAI1 */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SAI1CKSELR_REG)
/** @brief Kernel clock source selection for SAI2 */
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SAI2CKSELR_REG)
/** @brief Kernel clock source selection for FDCAN */
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, FDCANCKSELR_REG)
/** @brief Kernel clock source selection for SPDIF */
#define SPDIF_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, SPDIFCKSELR_REG)
/** @brief Kernel clock source selection for ADC1 */
#define ADC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, ADC12CKSELR_REG)
/** @brief Kernel clock source selection for ADC2 */
#define ADC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, ADC12CKSELR_REG)
/** @brief Kernel clock source selection for SDMMC1 */
#define SDMMC1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 2, 0, SDMMC12CKSELR_REG)
/** @brief Kernel clock source selection for SDMMC2 */
#define SDMMC2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 3, SDMMC12CKSELR_REG)
/** @brief Kernel clock source selection for ETH1 */
#define ETH1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, ETH12CKSELR_REG)
/** @brief Kernel clock source selection for ETH2 */
#define ETH2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, ETH12CKSELR_REG)
/** @brief Kernel clock source selection for USBPHY */
#define USBPHY_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, USBCKSELR_REG)
/** @brief Kernel clock source selection for USBOTG */
#define USBOTG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 4, USBCKSELR_REG)
/** @brief Kernel clock source selection for QSPI */
#define QSPI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, QSPICKSELR_REG)
/** @brief Kernel clock source selection for FMC */
#define FMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, FMCCKSELR_REG)
/** @brief Kernel clock source selection for RNG1 */
#define RNG1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, RNG1CKSELR_REG)
/** @brief Kernel clock source selection for STGEN */
#define STGEN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, STGENCKSELR_REG)
/** @brief Kernel clock source selection for DCMIPP */
#define DCMIPP_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, DCMIPPCKSELR_REG)
/** @brief Kernel clock source selection for SAES */
#define SAES_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, SAESCKSELR_REG)
#define RTC_SEL(val)            STM32_DT_CLOCK_SELECT((val), 17, 16, BDCR_REG)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP13_CLOCK_H_ */
