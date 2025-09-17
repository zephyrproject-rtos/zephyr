/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus clocks */
#define STM32_CLOCK_BUS_IOP     0x034
#define STM32_CLOCK_BUS_AHB1    0x038
#define STM32_CLOCK_BUS_APB1    0x03c
#define STM32_CLOCK_BUS_APB1_2  0x040

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_IOP
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1_2

/** Domain clocks */
/* RM0444, §5.4.21/22 Clock configuration register (RCC_CCIPRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSI + 1)
#define STM32_SRC_MSI		(STM32_SRC_HSI48 + 1)
#define STM32_SRC_HSE		(STM32_SRC_MSI + 1)
/** Peripheral bus clock */
#define STM32_SRC_PCLK		(STM32_SRC_HSE + 1)
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PCLK + 1)
/** PLL clock outputs */
#define STM32_SRC_PLL_P		(STM32_SRC_TIMPCLK1 + 1)
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)

/** @brief RCC_CFGR register offset */
#define CFGR_REG 0x08

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x54
#define CCIPR2_REG		0x58

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x5C

/** @brief Device domain clocks selection helpers */
/** CFGR devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 15, 24, CFGR_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 15, 28, CFGR_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 15, 16, CFGR_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 15, 20, CFGR_REG)
/** CCIPR devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR_REG)
#define CEC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 6, CCIPR_REG)
#define LPUART2_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR_REG)
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 10, CCIPR_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR_REG)
#define I2C2_I2S1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 14, CCIPR_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 18, CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR_REG)
#define TIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 22, CCIPR_REG)
#define TIM15_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 24, CCIPR_REG)
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 26, CCIPR_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 30, CCIPR_REG)
/** CCIPR2 devices */
#define I2S1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR2_REG)
#define I2S2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR2_REG)
#define FDCAN_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, CCIPR2_REG)
#define USB_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR2_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1   0
#define MCO_PRE_DIV_2   1
#define MCO_PRE_DIV_4   2
#define MCO_PRE_DIV_8   3
#define MCO_PRE_DIV_16  4
#define MCO_PRE_DIV_32  5
#define MCO_PRE_DIV_64  6
#define MCO_PRE_DIV_128 7

/* MCO clock output */
#define MCO_SEL_SYSCLK  1
#define MCO_SEL_HSI16   3
#define MCO_SEL_HSE     4
#define MCO_SEL_PLLRCLK 5
#define MCO_SEL_LSI     6
#define MCO_SEL_LSE     7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_CLOCK_H_ */
