/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x048
#define STM32_CLOCK_BUS_AHB2    0x04c
#define STM32_CLOCK_BUS_AHB3    0x050
#define STM32_CLOCK_BUS_APB1    0x058
#define STM32_CLOCK_BUS_APB1_2  0x05c
#define STM32_CLOCK_BUS_APB2    0x060
#define STM32_CLOCK_BUS_APB3    0x064

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** Domain clocks */
/* RM0461, §6.4.29 Clock configuration register (RCC_CFGR3) */


/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
#define STM32_SRC_MSI		(STM32_SRC_HSI + 1)
/* #define STM32_SRC_HSI48	TBD */
/** Bus clock */
#define STM32_SRC_PCLK		(STM32_SRC_MSI + 1)
/** PLL clock outputs */
#define STM32_SRC_PLL_P		(STM32_SRC_PCLK + 1)
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x88

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x90

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG        0x08

/** @brief Device domain clocks selection helpers */
/** CCIPR devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR_REG)
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 10, CCIPR_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 14, CCIPR_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, CCIPR_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 18, CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR_REG)
#define LPTIM3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 22, CCIPR_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 28, CCIPR_REG)
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 30, CCIPR_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)
/** CFGR1 devices */
#define MCO1_SEL(val)       STM32_DT_CLOCK_SELECT(val, 0xF, 24, CFGR1_REG)
#define MCO1_PRE(val)       STM32_DT_CLOCK_SELECT(val, 0x7, 28, CFGR1_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1  0
#define MCO_PRE_DIV_2  1
#define MCO_PRE_DIV_4  2
#define MCO_PRE_DIV_8  3
#define MCO_PRE_DIV_16 4

/* MCO clock output */
#define MCO_SEL_NOCLK     0
#define MCO_SEL_SYSCLKPRE 1
#define MCO_SEL_MSI       2
#define MCO_SEL_HSI16     3
#define MCO_SEL_HSE32     4
#define MCO_SEL_PLL1RCLK  5
#define MCO_SEL_LSI       6
#define MCO_SEL_LSE       8
#define MCO_SEL_PLL1PCLK  13
#define MCO_SEL_PLL1QCLK  14

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_ */
