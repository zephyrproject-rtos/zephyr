/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x048
#define STM32_CLOCK_BUS_AHB2    0x04c
#define STM32_CLOCK_BUS_AHB3    0x050
#define STM32_CLOCK_BUS_APB1    0x058
#define STM32_CLOCK_BUS_APB1_2  0x05c
#define STM32_CLOCK_BUS_APB2    0x060

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2

/** Peripheral clock sources */
/* RM0351/RM0432/RM0438, § Clock configuration register (RCC_CCIPRx) */

/** Fixed clocks  */
#define STM32_SRC_HSI		0x001
/* #define STM32_SRC_HSI48	0x002 */
#define STM32_SRC_LSE		0x003
#define STM32_SRC_LSI		0x004
#define STM32_SRC_MSI		0x005
/** System clock */
#define STM32_SRC_SYSCLK	0x006
/** Bus clock */
#define STM32_SRC_PCLK		0x007
/** PLL clock outputs */
#define STM32_SRC_PLL_P		0x008
#define STM32_SRC_PLL_Q		0x009
#define STM32_SRC_PLL_R		0x00a
/* TODO: PLLSAI clocks */

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

#define STM32_CLOCK_REG_MASK    0xFFU
#define STM32_CLOCK_REG_SHIFT   0U
#define STM32_CLOCK_SHIFT_MASK  0x1FU
#define STM32_CLOCK_SHIFT_SHIFT 8U
#define STM32_CLOCK_MASK_MASK   0x7U
#define STM32_CLOCK_MASK_SHIFT  13U
#define STM32_CLOCK_VAL_MASK    0x7U
#define STM32_CLOCK_VAL_SHIFT   16U

#define STM32_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |		\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |		\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CCIPR register offset */
#define CCIPR_REG		0x88
#define CCIPR2_REG		0x9C

/** @brief Device clk sources selection helpers */
/** CCIPR devices */
#define USART1_SEL(val)		STM32_CLOCK(val, 3, 0, CCIPR_REG)
#define USART2_SEL(val)		STM32_CLOCK(val, 3, 2, CCIPR_REG)
#define USART3_SEL(val)		STM32_CLOCK(val, 3, 4, CCIPR_REG)
#define UART4_SEL(val)		STM32_CLOCK(val, 3, 6, CCIPR_REG)
#define UART5_SEL(val)		STM32_CLOCK(val, 3, 8, CCIPR_REG)
#define LPUART1_SEL(val)	STM32_CLOCK(val, 3, 10, CCIPR_REG)
#define I2C1_SEL(val)		STM32_CLOCK(val, 3, 12, CCIPR_REG)
#define I2C2_SEL(val)		STM32_CLOCK(val, 3, 14, CCIPR_REG)
#define I2C3_SEL(val)		STM32_CLOCK(val, 3, 16, CCIPR_REG)
#define LPTIM1_SEL(val)		STM32_CLOCK(val, 3, 18, CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_CLOCK(val, 3, 20, CCIPR_REG)
#define SAI1_SEL(val)		STM32_CLOCK(val, 3, 22, CCIPR_REG)
#define SAI2_SEL(val)		STM32_CLOCK(val, 3, 24, CCIPR_REG)
#define CLK48_SEL(val)		STM32_CLOCK(val, 3, 26, CCIPR_REG)
#define ADC_SEL(val)		STM32_CLOCK(val, 3, 28, CCIPR_REG)
#define SWPMI1_SEL(val)		STM32_CLOCK(val, 1, 30, CCIPR_REG)
#define DFSDM1_SEL(val)		STM32_CLOCK(val, 1, 31, CCIPR_REG)
/** CCIPR2 devices */
#define I2C4_SEL(val)		STM32_CLOCK(val, 3, 0, CCIPR2_REG)
#define DFSDM_SEL(val)		STM32_CLOCK(val, 1, 2, CCIPR2_REG)
#define ADFSDM_SEL(val)		STM32_CLOCK(val, 3, 3, CCIPR2_REG)
/* #define SAI1_SEL(val)		STM32_CLOCK(val, 7, 5, CCIPR2_REG) */
/* #define SAI2_SEL(val)		STM32_CLOCK(val, 7, 8, CCIPR2_REG) */
#define DSI_SEL(val)		STM32_CLOCK(val, 1, 12, CCIPR2_REG)
#define SDMMC_SEL(val)		STM32_CLOCK(val, 1, 14, CCIPR2_REG)
#define OSPI_SEL(val)		STM32_CLOCK(val, 3, 20, CCIPR2_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4_CLOCK_H_ */
