/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1 0x030
#define STM32_CLOCK_BUS_AHB2 0x034
#define STM32_CLOCK_BUS_AHB3 0x038
#define STM32_CLOCK_BUS_APB1 0x040
#define STM32_CLOCK_BUS_APB2 0x044
#define STM32_CLOCK_BUS_APB3 0x0A8

#define STM32_PERIPH_BUS_MIN STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX STM32_CLOCK_BUS_APB3

/** Domain clocks */
/* RM0386, 0390, 0402, 0430 § Dedicated Clock configuration register (RCC_DCKCFGRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSE		(STM32_SRC_HSI + 1)
/** PLL clock outputs */
#define STM32_SRC_PLL_P		(STM32_SRC_HSE + 1)
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)
/** I2S sources */
#define STM32_SRC_PLLI2S_Q	(STM32_SRC_PLL_R + 1)
#define STM32_SRC_PLLI2S_R	(STM32_SRC_PLLI2S_Q + 1)
/* CLK48MHz sources */
#define STM32_SRC_CK48		(STM32_SRC_PLLI2S_R + 1)
/** Bus clock */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_CK48 + 1)
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)

/* PLLSAI clocks */
#define STM32_SRC_PLLSAI_P	(STM32_SRC_TIMPCLK2 + 1)
#define STM32_SRC_PLLSAI_Q	(STM32_SRC_PLLSAI_P + 1)
#define STM32_SRC_PLLSAI_DIVQ	(STM32_SRC_PLLSAI_Q + 1)
#define STM32_SRC_PLLSAI_R	(STM32_SRC_PLLSAI_DIVQ + 1)
#define STM32_SRC_PLLSAI_DIVR	(STM32_SRC_PLLSAI_R + 1)

/* I2S_CKIN not supported yet */
/* #define STM32_SRC_I2S_CKIN	TBD */

/** @brief RCC_CFGRx register offset */
#define CFGR_REG 0x08
/** @brief RCC_BDCR register offset */
#define BDCR_REG 0x70

/** @brief Device domain clocks selection helpers */
/** CFGR devices */
#define I2S_SEL(val)  STM32_DT_CLOCK_SELECT((val), 1, 23, CFGR_REG)
#define MCO1_SEL(val) STM32_DT_CLOCK_SELECT((val), 0x3, 21, CFGR_REG)
#define MCO1_PRE(val) STM32_DT_CLOCK_SELECT((val), 0x7, 24, CFGR_REG)
#define MCO2_SEL(val) STM32_DT_CLOCK_SELECT((val), 0x3, 30, CFGR_REG)
#define MCO2_PRE(val) STM32_DT_CLOCK_SELECT((val), 0x7, 27, CFGR_REG)
/** BDCR devices */
#define RTC_SEL(val)  STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1 0
#define MCO_PRE_DIV_2 4
#define MCO_PRE_DIV_3 5
#define MCO_PRE_DIV_4 6
#define MCO_PRE_DIV_5 7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_ */
