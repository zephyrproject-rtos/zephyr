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
 * @param reg RCC_CCIPRx register offset
 * @param shift Position within RCC_CCIPRx.
 * @param mask Mask for the RCC_CCIPRx field.
 * @param val Clock value (0, 1, ... 7).
 */
#define STM32_DOMAIN_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |		\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |		\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x88

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x90

/** @brief Device domain clocks selection helpers */
/** CCIPR devices */
#define USART1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 0, CCIPR_REG)
#define USART2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 2, CCIPR_REG)
#define SPI2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 8, CCIPR_REG)
#define LPUART1_SEL(val)	STM32_DOMAIN_CLOCK(val, 3, 10, CCIPR_REG)
#define I2C1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 12, CCIPR_REG)
#define I2C2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 14, CCIPR_REG)
#define I2C3_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 16, CCIPR_REG)
#define LPTIM1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 18, CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 20, CCIPR_REG)
#define LPTIM3_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 22, CCIPR_REG)
#define ADC_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 28, CCIPR_REG)
#define RNG_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 30, CCIPR_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 8, BDCR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WL_CLOCK_H_ */
