/*
 * Copyright (c) 2023 Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus clocks */
#define STM32_CLOCK_BUS_IOP     0x034
#define STM32_CLOCK_BUS_AHB1    0x038
#define STM32_CLOCK_BUS_APB1    0x03c
#define STM32_CLOCK_BUS_APB1_2  0x040

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_IOP
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1_2

/** Domain clocks */
/* RM0490, §5.4.21/22 Clock configuration register (RCC_CCIPRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSE		(STM32_SRC_HSI + 1)
/** Peripheral bus clock */
#define STM32_SRC_PCLK		(STM32_SRC_HSE + 1)

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x54

/** @brief RCC_CSR1 register offset */
#define CSR1_REG		0x5C

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x08

/** @brief Device domain clocks selection helpers */
/** CCIPR devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR_REG)
#define I2C2_I2S1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 14, CCIPR_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 30, CCIPR_REG)
/** CSR1 devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CSR1_REG)

/** CFGR1 devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 24, CFGR1_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 28, CFGR1_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 16, CFGR1_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 20, CFGR1_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1   0
#define MCO_PRE_DIV_2   1
#define MCO_PRE_DIV_4   2
#define MCO_PRE_DIV_8   3
#define MCO_PRE_DIV_16  4
#define MCO_PRE_DIV_32  5
#define MCO_PRE_DIV_64  6
#define MCO_PRE_DIV_128 7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32C0_CLOCK_H_ */
