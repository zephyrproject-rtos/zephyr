/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F0_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus gatting clocks */
#define STM32_CLOCK_BUS_AHB1    0x014
#define STM32_CLOCK_BUS_APB2    0x018
#define STM32_CLOCK_BUS_APB1    0x01c

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1

/** Domain clocks */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI14		(STM32_SRC_HSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSI14 + 1)
/** Bus clock */
#define STM32_SRC_PCLK		(STM32_SRC_HSI48 + 1)
/** PLL clock */
#define STM32_SRC_PLLCLK	(STM32_SRC_PCLK + 1)

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

/** @brief RCC_CFGRx register offset */
#define CFGR3_REG		0x30

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x20

/** @brief Device domain clocks selection helpers */
/** CFGR3 devices */
#define USART1_SEL(val)		STM32_CLOCK(val, 3, 0, CFGR3_REG)
#define I2C1_SEL(val)		STM32_CLOCK(val, 1, 4, CFGR3_REG)
#define CEC_SEL(val)		STM32_CLOCK(val, 1, 6, CFGR3_REG)
#define USB_SEL(val)		STM32_CLOCK(val, 1, 7, CFGR3_REG)
#define USART2_SEL(val)		STM32_CLOCK(val, 3, 16, CFGR3_REG)
#define USART3_SEL(val)		STM32_CLOCK(val, 3, 18, CFGR3_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_CLOCK(val, 3, 8, BDCR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F0_CLOCK_H_ */
