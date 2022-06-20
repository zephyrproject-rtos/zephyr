/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_

/** Peripheral clock sources */

/* RM0468, Table 56 Kernel clock dictribution summary */

/** PLL outputs */
#define STM32_SRC_PLL1_P	0x001
#define STM32_SRC_PLL1_Q	0x002
#define STM32_SRC_PLL1_R	0x003
/** PLL2 not yet supported */
/* #define STM32_SRC_PLL2_P	0x004 */
/* #define STM32_SRC_PLL2_Q	0x005 */
/* #define STM32_SRC_PLL2_R	0x006 */
#define STM32_SRC_PLL3_P	0x007
#define STM32_SRC_PLL3_Q	0x008
#define STM32_SRC_PLL3_R	0x009
/** Oscillators  */
#define STM32_SRC_HSE		0x00A
#define STM32_SRC_LSE		0x00B
#define STM32_SRC_LSI		0x00C
/** Oscillators not yet supported */
/* #define STM32_SRC_HSI48	0x00D */
#define STM32_SRC_HSI_KER	0x00E /* HSI + HSIKERON */
#define STM32_SRC_CSI_KER	0x00F /* CSI + CSIKERON */
/** Core clock */
#define STM32_SRC_SYSCLK	0x010
/** Others: Not yet supported */
/* #define STM32_SRC_I2SCKIN	0x011 */
/* #define STM32_SRC_SPDIFRX	0x012 */
/** Clock muxes */
#define STM32_SRC_CKPER		0x013

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB3    0x0D4
#define STM32_CLOCK_BUS_AHB1    0x0D8
#define STM32_CLOCK_BUS_AHB2    0x0DC
#define STM32_CLOCK_BUS_AHB4    0x0E0
#define STM32_CLOCK_BUS_APB3    0x0E4
#define STM32_CLOCK_BUS_APB1    0x0E8
#define STM32_CLOCK_BUS_APB1_2  0x0EC
#define STM32_CLOCK_BUS_APB2    0x0F0
#define STM32_CLOCK_BUS_APB4    0x0F4
/** Alias D1/2/3 domains clocks */ /* TBD: To remove ? */
#define STM32_SRC_PCLK1		STM32_CLOCK_BUS_APB1
#define STM32_SRC_PCLK2		STM32_CLOCK_BUS_APB2
#define STM32_SRC_HCLK3		STM32_CLOCK_BUS_AHB3
#define STM32_SRC_PCLK3		STM32_CLOCK_BUS_APB3
#define STM32_SRC_PCLK4		STM32_CLOCK_BUS_APB4

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB3
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB4

/**
 * @brief STM32H7 clock configuration bit field.
 *
 * - reg   (0/1)           [ 0 : 7 ]
 * - shift (0..31)         [ 8 : 12 ]
 * - mask  (0x1, 0x3, 0x7) [ 13 : 15 ]
 * - val   (0..3)          [ 16 : 18 ]
 *
 * @param reg RCC_DxCCIP register offset
 * @param shift Position within RCC_DxCCIP.
 * @param mask Mask for the RCC_DxCCIP field.
 * @param val Clock value (0, 1, 2 or 3).
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

/** @brief RCC_DxCCIP register offset (RM0399.pdf) */
#define D1CCIPR_REG		0x4C
#define D2CCIP1R_REG		0x50
#define D2CCIP2R_REG		0x54
#define D3CCIPR_REG		0x58

/** @brief Device clk sources selection helpers (RM0399.pdf) */
/** D1CCIPR devices */
#define FMC_SEL(val)		STM32_CLOCK(val, 3, 0, D1CCIPR_REG)
#define QSPI_SEL(val)		STM32_CLOCK(val, 3, 4, D1CCIPR_REG)
#define DSI_SEL(val)		STM32_CLOCK(val, 1, 8, D1CCIPR_REG)
#define SDMMC_SEL(val)		STM32_CLOCK(val, 1, 16, D1CCIPR_REG)
#define CKPER_SEL(val)		STM32_CLOCK(val, 3, 28, D1CCIPR_REG)
/* device clk sources selection helpers (RM0468.pdf) */
#define OSPI_SEL(val)		STM32_CLOCK(val, 3, 4, D1CCIPR_REG)
/** D2CCIP1R devices */
#define SAI1_SEL(val)		STM32_CLOCK(val, 7, 0, D2CCIP1R_REG)
#define SAI23_SEL(val)		STM32_CLOCK(val, 7, 6, D2CCIP1R_REG)
#define SPI123_SEL(val)		STM32_CLOCK(val, 7, 12, D2CCIP1R_REG)
#define SPI45_SEL(val)		STM32_CLOCK(val, 7, 16, D2CCIP1R_REG)
#define SPDIF_SEL(val)		STM32_CLOCK(val, 3, 20, D2CCIP1R_REG)
#define DFSDM1_SEL(val)		STM32_CLOCK(val, 1, 24, D2CCIP1R_REG)
#define FDCAN_SEL(val)		STM32_CLOCK(val, 3, 28, D2CCIP1R_REG)
#define SWP_SEL(val)		STM32_CLOCK(val, 1, 31, D2CCIP1R_REG)
/** D2CCIP2R devices */
#define USART2345678_SEL(val)	STM32_CLOCK(val, 7, 0, D2CCIP2R_REG)
#define USART16_SEL(val)	STM32_CLOCK(val, 7, 3, D2CCIP2R_REG)
#define RNG_SEL(val)		STM32_CLOCK(val, 3, 8, D2CCIP2R_REG)
#define I2C123_SEL(val)		STM32_CLOCK(val, 3, 12, D2CCIP2R_REG)
#define USB_SEL(val)		STM32_CLOCK(val, 3, 20, D2CCIP2R_REG)
#define CEC_SEL(val)		STM32_CLOCK(val, 3, 22, D2CCIP2R_REG)
#define LPTIM1_SEL(val)		STM32_CLOCK(val, 7, 28, D2CCIP2R_REG)
/** D3CCIPR devices */
#define LPUART1_SEL(val)	STM32_CLOCK(val, 7, 0, D3CCIPR_REG)
#define I2C4_SEL(val)		STM32_CLOCK(val, 3, 8, D3CCIPR_REG)
#define LPTIM2_SEL(val)		STM32_CLOCK(val, 7, 10, D3CCIPR_REG)
#define LPTIM345_SEL(val)	STM32_CLOCK(val, 7, 13, D3CCIPR_REG)
#define ADC_SEL(val)		STM32_CLOCK(val, 3, 16, D3CCIPR_REG)
#define SAI4A_SEL(val)		STM32_CLOCK(val, 7, 21, D3CCIPR_REG)
#define SAI4B_SEL(val)		STM32_CLOCK(val, 7, 24, D3CCIPR_REG)
#define SPI6_SEL(val)		STM32_CLOCK(val, 7, 28, D3CCIPR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7_CLOCK_H_ */
