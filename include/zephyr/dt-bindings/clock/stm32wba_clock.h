/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_

#include "stm32_common_clocks.h"

/** Peripheral clock sources */

/* RM0493, Figure 30, clock tree */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI16		(STM32_SRC_HSE + 1)
/** Bus clock */
#define STM32_SRC_HCLK1		(STM32_SRC_HSI16 + 1)
#define STM32_SRC_HCLK5		(STM32_SRC_HCLK1 + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK5 + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK7		(STM32_SRC_PCLK2 + 1)
/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_PCLK7 + 1)
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)

#define STM32_SRC_CLOCK_MIN	STM32_SRC_PLL1_P
#define STM32_SRC_CLOCK_MAX	STM32_SRC_SYSCLK

/** Bus clocks (Register address offsets) */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB4    0x094
#define STM32_CLOCK_BUS_AHB5    0x098
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB7    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB7

/**
 * @brief STM32WBA clock configuration bit field.
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

#define STM32_CLOCK_REG_MASK    0xFFU
#define STM32_CLOCK_REG_SHIFT   0U
#define STM32_CLOCK_SHIFT_MASK  0x1FU
#define STM32_CLOCK_SHIFT_SHIFT 8U
#define STM32_CLOCK_MASK_MASK   0x7U
#define STM32_CLOCK_MASK_SHIFT  13U
#define STM32_CLOCK_VAL_MASK    0x7U
#define STM32_CLOCK_VAL_SHIFT   16U

#define STM32_DOMAIN_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |	\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |	\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CCIPRx register offset (RM0493.pdf) */
#define CCIPR1_REG		0xE0
#define CCIPR2_REG		0xE4
#define CCIPR3_REG		0xE8
/** @brief RCC_BCDR1 register offset (RM0493.pdf) */
#define BCDR1_REG		0xF0

/** @brief Device clk sources selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 0, CCIPR1_REG)
#define USART2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 2, CCIPR1_REG)
#define I2C1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 10, CCIPR1_REG)
#define LPTIM2_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 18, CCIPR1_REG)
#define SPI1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 20, CCIPR1_REG)
#define SYSTICK_SEL(val)	STM32_DOMAIN_CLOCK(val, 3, 22, CCIPR1_REG)
#define TIMIC_SEL(val)		STM32_DOMAIN_CLOCK(val, 1, 31, CCIPR1_REG)
/** CCIPR2 devices */
#define RNG_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 12, CCIPR2_REG)
/** CCIPR3 devices */
#define LPUART1_SEL(val)	STM32_DOMAIN_CLOCK(val, 3, 0, CCIPR3_REG)
#define SPI3_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 3, CCIPR3_REG)
#define I2C3_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 6, CCIPR3_REG)
#define LPTIM1_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 10, CCIPR3_REG)
#define ADC_SEL(val)		STM32_DOMAIN_CLOCK(val, 7, 12, CCIPR3_REG)
/** BCDR1 devices */
#define RTC_SEL(val)		STM32_DOMAIN_CLOCK(val, 3, 8, BCDR1_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_ */
