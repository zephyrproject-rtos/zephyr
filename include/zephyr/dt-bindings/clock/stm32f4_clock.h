/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_

/** Peripheral clock sources */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x030
#define STM32_CLOCK_BUS_AHB2    0x034
#define STM32_CLOCK_BUS_AHB3    0x038
#define STM32_CLOCK_BUS_APB1    0x040
#define STM32_CLOCK_BUS_APB2    0x044
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** Peripheral clock sources */
/* RM0386, 0390, 0402, 0430 § Dedicated Clock configuration register (RCC_DCKCFGRx) */

/** Fixed clocks  */
#define STM32_SRC_HSI		0x001
#define STM32_SRC_LSE		0x002
#define STM32_SRC_LSI		0x003
/* #define STM32_SRC_HSI48	0x004 */
/** System clock */
/* #define STM32_SRC_SYSCLK	0x005 */
/** Bus clock */
#define STM32_SRC_PCLK		0x006
/** PLL clock outputs */
#define STM32_SRC_PLL_P	0x007
#define STM32_SRC_PLL_Q	0x008
#define STM32_SRC_PLL_R	0x009

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_ */
