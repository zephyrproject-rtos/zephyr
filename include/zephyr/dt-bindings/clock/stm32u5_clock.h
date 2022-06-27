/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_

/** Peripheral clock sources */

/* RM0468, Table 56 Kernel clock distribution summary */

/** PLL outputs */
#define STM32_SRC_PLL1_P	0x001
#define STM32_SRC_PLL1_Q	0x002
#define STM32_SRC_PLL1_R	0x003
/** PLL2/3 not yet supported */
/* #define STM32_SRC_PLL2_P	0x004 */
/* #define STM32_SRC_PLL2_Q	0x005 */
/* #define STM32_SRC_PLL2_R	0x006 */
/* #define STM32_SRC_PLL3_P	0x007 */
/* #define STM32_SRC_PLL3_Q	0x008 */
/* #define STM32_SRC_PLL3_R	0x009 */
/** Fixed clocks  */
#define STM32_SRC_HSE		0x00A
#define STM32_SRC_LSE		0x00B
#define STM32_SRC_LSI		0x00C
#define STM32_SRC_HSI16		0x00D
/* #define STM32_SRC_HSI48	0x00E */
#define STM32_SRC_MSIS		0x00F
#define STM32_SRC_MSIK		0x010
/** Core clock */
#define STM32_SRC_SYSCLK	0x011
/** Clock muxes */
/* #define STM32_SRC_ICLK	0x012 */

#define STM32_SRC_CLOCK_MIN	STM32_SRC_PLL1_P
#define STM32_SRC_CLOCK_MAX	STM32_SRC_SYSCLK

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB2_2  0x090
#define STM32_CLOCK_BUS_AHB3    0x094
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB3

/**
 * @brief STM32U5 clock configuration bit field.
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

#define STM32_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |	\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |	\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CCIPRx register offset (RM0456.pdf) */
#define CCIPR1_REG		0xE0
#define CCIPR2_REG		0xE4
#define CCIPR3_REG		0xE8

/** @brief Device clk sources selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_CLOCK(val, 3, 0, CCIPR1_REG)
#define USART2_SEL(val)		STM32_CLOCK(val, 3, 2, CCIPR1_REG)
#define USART3_SEL(val)		STM32_CLOCK(val, 3, 4, CCIPR1_REG)
#define USART4_SEL(val)		STM32_CLOCK(val, 3, 6, CCIPR1_REG)
#define USART5_SEL(val)		STM32_CLOCK(val, 3, 8, CCIPR1_REG)
#define I2C1_SEL(val)		STM32_CLOCK(val, 3, 10, CCIPR1_REG)
#define I2C2_SEL(val)		STM32_CLOCK(val, 3, 12, CCIPR1_REG)
#define I2C4_SEL(val)		STM32_CLOCK(val, 3, 14, CCIPR1_REG)
#define SPI2_SEL(val)		STM32_CLOCK(val, 3, 16, CCIPR1_REG)
#define LPTIM2_SEL(val)		STM32_CLOCK(val, 3, 18, CCIPR1_REG)
#define SPI1_SEL(val)		STM32_CLOCK(val, 3, 20, CCIPR1_REG)
#define SYSTICK_SEL(val)	STM32_CLOCK(val, 3, 22, CCIPR1_REG)
#define FDCAN1_SEL(val)		STM32_CLOCK(val, 3, 24, CCIPR1_REG)
#define ICKLK_SEL(val)		STM32_CLOCK(val, 3, 26, CCIPR1_REG)
#define TIMIC_SEL(val)		STM32_CLOCK(val, 7, 29, CCIPR1_REG)
/** CCIPR2 devices */
#define MDF1_SEL(val)		STM32_CLOCK(val, 7, 0, CCIPR2_REG)
#define SAI1_SEL(val)		STM32_CLOCK(val, 7, 5, CCIPR2_REG)
#define SAI2_SEL(val)		STM32_CLOCK(val, 7, 8, CCIPR2_REG)
#define SAE_SEL(val)		STM32_CLOCK(val, 1, 11, CCIPR2_REG)
#define RNG_SEL(val)		STM32_CLOCK(val, 3, 12, CCIPR2_REG)
#define SDMMC_SEL(val)		STM32_CLOCK(val, 1, 14, CCIPR2_REG)
#define OCTOSPI_SEL(val)	STM32_CLOCK(val, 3, 20, CCIPR2_REG)
/** CCIPR3 devices */
#define LPUART1_SEL(val)	STM32_CLOCK(val, 7, 0, CCIPR3_REG)
#define SPI3_SEL(val)		STM32_CLOCK(val, 3, 3, CCIPR3_REG)
#define I2C3_SEL(val)		STM32_CLOCK(val, 3, 6, CCIPR3_REG)
#define LPTIM34_SEL(val)	STM32_CLOCK(val, 3, 8, CCIPR3_REG)
#define LPTIM1_SEL(val)		STM32_CLOCK(val, 3, 10, CCIPR3_REG)
#define ADCDAC_SEL(val)		STM32_CLOCK(val, 7, 12, CCIPR3_REG)
#define DAC1_SEL(val)		STM32_CLOCK(val, 1, 15, CCIPR3_REG)
#define ADF1_SEL(val)		STM32_CLOCK(val, 7, 16, CCIPR3_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32U5_CLOCK_H_ */
