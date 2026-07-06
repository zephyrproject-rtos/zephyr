/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* Bus clocks */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1 0x030
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2 0x034
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3 0x038
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1 0x040
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2 0x044
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3 0x0A8

/** @brief First peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MIN STM32_CLOCK_BUS_AHB1
/** @brief Last peripheral bus clock enable register offset */
#define STM32_PERIPH_BUS_MAX STM32_CLOCK_BUS_APB3

/** Domain clocks */
/* RM0386, 0390, 0402, 0430 § Dedicated Clock configuration register (RCC_DCKCFGRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks */
/* Low speed clocks defined in stm32_common_clocks.h */
/** @brief Clock source identifier for HSI */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
/** @brief Clock source identifier for HSE */
#define STM32_SRC_HSE		(STM32_SRC_HSI + 1)
/* PLL clock outputs */
/** @brief Clock source identifier for PLL_P */
#define STM32_SRC_PLL_P		(STM32_SRC_HSE + 1)
/** @brief Clock source identifier for PLL_Q */
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
/** @brief Clock source identifier for PLL_R */
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)
/** @brief Clock source identifier for PLL_POST_R */
#define STM32_SRC_PLL_POST_R	(STM32_SRC_PLL_R + 1)
/* PLLI2S clock outputs */
/** @brief Clock source identifier for PLLI2S_P */
#define STM32_SRC_PLLI2S_P	(STM32_SRC_PLL_POST_R + 1)
/** @brief Clock source identifier for PLLI2S_Q */
#define STM32_SRC_PLLI2S_Q	(STM32_SRC_PLLI2S_P + 1)
/** @brief Clock source identifier for PLLI2S_POST_Q */
#define STM32_SRC_PLLI2S_POST_Q	(STM32_SRC_PLLI2S_Q + 1)
/** @brief Clock source identifier for PLLI2S_R */
#define STM32_SRC_PLLI2S_R	(STM32_SRC_PLLI2S_POST_Q + 1)
/** @brief Clock source identifier for PLLI2S_POST_R */
#define STM32_SRC_PLLI2S_POST_R	(STM32_SRC_PLLI2S_R + 1)
/* PLLSAI clock outputs */
/** @brief Clock source identifier for PLLSAI_P */
#define STM32_SRC_PLLSAI_P	(STM32_SRC_PLLI2S_POST_R + 1)
/** @brief Clock source identifier for PLLSAI_Q */
#define STM32_SRC_PLLSAI_Q	(STM32_SRC_PLLSAI_P + 1)
/** @brief Clock source identifier for PLLSAI_POST_Q */
#define STM32_SRC_PLLSAI_POST_Q	(STM32_SRC_PLLSAI_Q + 1)
/** @brief Clock source identifier for PLLSAI_R */
#define STM32_SRC_PLLSAI_R	(STM32_SRC_PLLSAI_POST_Q + 1)
/** @brief Clock source identifier for PLLSAI_POST_R */
#define STM32_SRC_PLLSAI_POST_R	(STM32_SRC_PLLSAI_R + 1)
/* CLK48MHz sources */
/** @brief Clock source identifier for CK48 */
#define STM32_SRC_CK48		(STM32_SRC_PLLSAI_POST_R + 1)
/* Bus clock */
/** @brief Clock source identifier for TIMPCLK1 */
#define STM32_SRC_TIMPCLK1	(STM32_SRC_CK48 + 1)
/** @brief Clock source identifier for TIMPCLK2 */
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)


/* I2S_CKIN not supported yet */
/* #define STM32_SRC_I2S_CKIN	TBD */

/** @brief RCC_CFGRx register offset */
#define CFGR_REG 0x08
/** @brief RCC_BDCR register offset */
#define BDCR_REG 0x70

/** @brief Device domain clocks selection helpers */
/* CFGR devices */
/** @brief MCO1 clock source selection */
#define MCO1_SEL(val) STM32_DT_CLOCK_SELECT((val), 22, 21, CFGR_REG)
/** @brief Kernel clock source selection for I2S */
#define I2S_SEL(val)  STM32_DT_CLOCK_SELECT((val), 23, 23, CFGR_REG)
/** @brief MCO1 prescaler selection */
#define MCO1_PRE(val) STM32_DT_CLOCK_SELECT((val), 26, 24, CFGR_REG)
/** @brief MCO2 prescaler selection */
#define MCO2_PRE(val) STM32_DT_CLOCK_SELECT((val), 29, 27, CFGR_REG)
/** @brief MCO2 clock source selection */
#define MCO2_SEL(val) STM32_DT_CLOCK_SELECT((val), 31, 30, CFGR_REG)
/* BDCR devices */
/** @brief Kernel clock source selection for RTC */
#define RTC_SEL(val)  STM32_DT_CLOCK_SELECT((val), 9, 8, BDCR_REG)

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 1 */
#define MCO_PRE_DIV_1 0
/** @brief MCO prescaler division factor: 2 */
#define MCO_PRE_DIV_2 4
/** @brief MCO prescaler division factor: 3 */
#define MCO_PRE_DIV_3 5
/** @brief MCO prescaler division factor: 4 */
#define MCO_PRE_DIV_4 6
/** @brief MCO prescaler division factor: 5 */
#define MCO_PRE_DIV_5 7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_ */
