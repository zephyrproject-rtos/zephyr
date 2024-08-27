/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/* RM0477  */

/** System clock */
/* defined in stm32_common_clocks.h */

/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI48		(STM32_SRC_HSE + 1)
#define STM32_SRC_HSI_KER	(STM32_SRC_HSI48 + 1) /* HSI + HSIKERON */
#define STM32_SRC_CSI_KER	(STM32_SRC_HSI_KER + 1) /* CSI + CSIKERON */
/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_CSI_KER + 1)
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)
#define STM32_SRC_PLL1_S	(STM32_SRC_PLL1_R + 1)
#define STM32_SRC_PLL2_P	(STM32_SRC_PLL1_S + 1)
#define STM32_SRC_PLL2_Q	(STM32_SRC_PLL2_P + 1)
#define STM32_SRC_PLL2_R	(STM32_SRC_PLL2_Q + 1)
#define STM32_SRC_PLL2_S	(STM32_SRC_PLL2_R + 1)
#define STM32_SRC_PLL2_T	(STM32_SRC_PLL2_S + 1)
#define STM32_SRC_PLL3_P	(STM32_SRC_PLL2_T + 1)
#define STM32_SRC_PLL3_Q	(STM32_SRC_PLL3_P + 1)
#define STM32_SRC_PLL3_R	(STM32_SRC_PLL3_Q + 1)
#define STM32_SRC_PLL3_S	(STM32_SRC_PLL3_R + 1)

/** Clock muxes */
#define STM32_SRC_CKPER		(STM32_SRC_PLL3_S + 1)
/** Others: Not yet supported */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x138
#define STM32_CLOCK_BUS_AHB2    0x13C
#define STM32_CLOCK_BUS_AHB3    0x158
#define STM32_CLOCK_BUS_AHB4    0x140
#define STM32_CLOCK_BUS_AHB5    0x134
#define STM32_CLOCK_BUS_APB1    0x148
#define STM32_CLOCK_BUS_APB1_2  0x14C
#define STM32_CLOCK_BUS_APB2    0x150
#define STM32_CLOCK_BUS_APB4    0x154
#define STM32_CLOCK_BUS_APB5    0x144
#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB5
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_AHB3

#define STM32_CLOCK_REG_MASK    0xFFU
#define STM32_CLOCK_REG_SHIFT   0U
#define STM32_CLOCK_SHIFT_MASK  0x1FU
#define STM32_CLOCK_SHIFT_SHIFT 8U
#define STM32_CLOCK_MASK_MASK   0x7U
#define STM32_CLOCK_MASK_SHIFT  13U
#define STM32_CLOCK_VAL_MASK    0x7U
#define STM32_CLOCK_VAL_SHIFT   16U

/**
 * @brief STM32H7RS clock configuration bit field.
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
#define STM32_CLOCK(val, mask, shift, reg)					\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |	\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |	\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_DxCCIP register offset (RM0477.pdf) */
#define D1CCIPR_REG		0x4C
#define D2CCIPR_REG		0x50
#define D3CCIPR_REG		0x54
#define D4CCIPR_REG		0x58

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief RCC_CFGRx register offset */
#define CFGR_REG                0x10

/** @brief Device domain clocks selection helpers (RM0477.pdf) */

/* TODO to be completed */

/** D1CCIPR devices */
#define FMC_SEL(val)		STM32_CLOCK(val, 3, 0, D1CCIPR_REG)
#define SDMMC_SEL(val)		STM32_CLOCK(val, 1, 2, D1CCIPR_REG)
#define XSPI1_SEL(val)		STM32_CLOCK(val, 3, 4, D1CCIPR_REG)
#define XSPI2_SEL(val)		STM32_CLOCK(val, 3, 6, D1CCIPR_REG)
#define ADC_SEL(val)		STM32_CLOCK(val, 3, 24, D1CCIPR_REG)
#define CKPER_SEL(val)		STM32_CLOCK(val, 3, 28, D1CCIPR_REG)

/** D2CCIPR devices */
#define USART234578_SEL(val)	STM32_CLOCK(val, 7, 0, D2CCIPR_REG)
#define SPI23_SEL(val)		STM32_CLOCK(val, 7, 4, D2CCIPR_REG)
#define I2C23_SEL(val)		STM32_CLOCK(val, 3, 8, D2CCIPR_REG)
#define I2C1_SEL(val)		STM32_CLOCK(val, 3, 12, D2CCIPR_REG)
#define I3C1_SEL(val)		STM32_CLOCK(val, 3, 12, D2CCIPR_REG)
#define LPTIM1_SEL(val)		STM32_CLOCK(val, 7, 16, D2CCIPR_REG)
#define FDCAN_SEL(val)		STM32_CLOCK(val, 3, 22, D2CCIPR_REG)

/** D3CCIPR devices */
#define USART1_SEL(val)		STM32_CLOCK(val, 7, 0, D3CCIPR_REG)
#define SPI45_SEL(val)		STM32_CLOCK(val, 7, 4, D3CCIPR_REG)
#define SPI1_SEL(val)		STM32_CLOCK(val, 7, 8, D3CCIPR_REG)
#define SAI1_SEL(val)		STM32_CLOCK(val, 7, 16, D3CCIPR_REG)
#define SAI2_SEL(val)		STM32_CLOCK(val, 7, 20, D3CCIPR_REG)

/** D4CCIPR devices */
#define LPUART1_SEL(val)	STM32_CLOCK(val, 7, 0, D4CCIPR_REG)
#define SPI6_SEL(val)		STM32_CLOCK(val, 7, 4, D4CCIPR_REG)
#define LPTIM23_SEL(val)	STM32_CLOCK(val, 7, 8, D4CCIPR_REG)
#define LPTIM45_SEL(val)	STM32_CLOCK(val, 7, 12, D4CCIPR_REG)

/** BDCR devices */
#define RTC_SEL(val)		STM32_CLOCK(val, 3, 8, BDCR_REG)

/** CFGR devices */
#define MCO1_SEL(val)           STM32_MCO_CFGR(val, 0x7, 22, CFGR_REG)
#define MCO1_PRE(val)           STM32_MCO_CFGR(val, 0xF, 18, CFGR_REG)
#define MCO2_SEL(val)           STM32_MCO_CFGR(val, 0x7, 29, CFGR_REG)
#define MCO2_PRE(val)           STM32_MCO_CFGR(val, 0xF, 25, CFGR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32H7RS_CLOCK_H_ */
