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
#define STM32_CLOCK_BUS_AHB1    0x030
#define STM32_CLOCK_BUS_AHB2    0x034
#define STM32_CLOCK_BUS_AHB3    0x038
#define STM32_CLOCK_BUS_APB1    0x040
#define STM32_CLOCK_BUS_APB2    0x044
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/** Domain clocks */
/* RM0386, 0390, 0402, 0430 § Dedicated Clock configuration register (RCC_DCKCFGRx) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks */
/* Low speed clocks defined in stm32_common_clocks.h */
/** PLL clock outputs */
#define STM32_SRC_PLL_P		(STM32_SRC_LSI + 1)
#define STM32_SRC_PLL_Q		(STM32_SRC_PLL_P + 1)
#define STM32_SRC_PLL_R		(STM32_SRC_PLL_Q + 1)
/** I2S sources */
#define STM32_SRC_PLLI2S_R	(STM32_SRC_PLL_R + 1)
/* I2S_CKIN not supported yet */
/* #define STM32_SRC_I2S_CKIN	TBD */


#define STM32_CLOCK_REG_MASK    0xFFU
#define STM32_CLOCK_REG_SHIFT   0U
#define STM32_CLOCK_SHIFT_MASK  0x1FU
#define STM32_CLOCK_SHIFT_SHIFT 8U
#define STM32_CLOCK_MASK_MASK   0x7U
#define STM32_CLOCK_MASK_SHIFT  13U
#define STM32_CLOCK_VAL_MASK    0x7U
#define STM32_CLOCK_VAL_SHIFT   16U

/**
 * @brief STM32 clock configuration bit field.
 *
 * - reg   (1/2/3)         [ 0 : 7 ]
 * - shift (0..31)         [ 8 : 12 ]
 * - mask  (0x1, 0x3, 0x7) [ 13 : 15 ]
 * - val   (0..7)          [ 16 : 18 ]
 *
 * @param reg RCC_CFGRx register offset
 * @param shift Position within RCC_CFGRx.
 * @param mask Mask for the RCC_CFGRx field.
 * @param val Clock value (0, 1, ... 7).
 */
#define STM32_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |		\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |		\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CFGR register offset */
#define CFGR_REG		0x08
/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief Device domain clocks selection helpers */
/** CFGR devices */
#define I2S_SEL(val)		STM32_CLOCK(val, 1, 23, CFGR_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_CLOCK(val, 3, 8, BDCR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F4_CLOCK_H_ */
