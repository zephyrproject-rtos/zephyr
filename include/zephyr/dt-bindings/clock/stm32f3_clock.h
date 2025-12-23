/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F3_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F3_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus gatting clocks */
#define STM32_CLOCK_BUS_AHB1    0x014
#define STM32_CLOCK_BUS_APB2    0x018
#define STM32_CLOCK_BUS_APB1    0x01c

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1

/** Domain clocks */
/* RM0316, §9.4.13 Clock configuration register (RCC_CFGR3) */

/** System clock */
/* Defined in stm32_common_clocks.h */

/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
/* #define STM32_SRC_HSI48	TDB */
/** Bus clock */
#define STM32_SRC_PCLK		(STM32_SRC_HSI + 1)
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK + 1)
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)
#define STM32_SRC_TIMPLLCLK	(STM32_SRC_TIMPCLK2 + 1)
/** PLL clock */
#define STM32_SRC_PLLCLK	(STM32_SRC_TIMPLLCLK + 1)

/** @brief RCC_CFGRx register offset */
#define CFGR_REG		0x04
#define CFGR2_REG		0x2C
#define CFGR3_REG		0x30

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x20

/** @brief Device domain clocks selection helpers) */
/** CFGR devices */
#define I2S_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 23, CFGR_REG)
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 26, 24, CFGR_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CFGR_REG)
/** CFGR2 devices */
#define ADC12_PRE(val)		STM32_DT_CLOCK_SELECT((val), 8, 4, CFGR2_REG)
#define ADC34_PRE(val)		STM32_DT_CLOCK_SELECT((val), 13, 9, CFGR2_REG)
/** CFGR3 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 0, CFGR3_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 4, 4, CFGR3_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 5, 5, CFGR3_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 6, 6, CFGR3_REG)
#define TIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 8, 8, CFGR3_REG)
#define TIM8_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 9, CFGR3_REG)
#define TIM15_SEL(val)		STM32_DT_CLOCK_SELECT((val), 10, 10, CFGR3_REG)
#define TIM16_SEL(val)		STM32_DT_CLOCK_SELECT((val), 11, 11, CFGR3_REG)
#define TIM17_SEL(val)		STM32_DT_CLOCK_SELECT((val), 13, 13, CFGR3_REG)
#define TIM20_SEL(val)		STM32_DT_CLOCK_SELECT((val), 15, 15, CFGR3_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 17, 16, CFGR3_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 19, 18, CFGR3_REG)
#define USART4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, CFGR3_REG)
#define USART5_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, CFGR3_REG)
#define TIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 24, 24, CFGR3_REG)
#define TIM3_4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 25, 25, CFGR3_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/* ADC prescaler division factor for all F3 except F37x */
#define ADC_PRE_DISABLED	0x0
#define ADC_PRE_DIV_1		0x10
#define ADC_PRE_DIV_2		0x11
#define ADC_PRE_DIV_4		0x12
#define ADC_PRE_DIV_6		0x13
#define ADC_PRE_DIV_8		0x14
#define ADC_PRE_DIV_10		0x15
#define ADC_PRE_DIV_12		0x16
#define ADC_PRE_DIV_16		0x17
#define ADC_PRE_DIV_32		0x18
#define ADC_PRE_DIV_64		0x19
#define ADC_PRE_DIV_128		0x1A
#define ADC_PRE_DIV_256		0x1B

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F3_CLOCK_H_ */
