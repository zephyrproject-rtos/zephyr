/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F7_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F7_CLOCK_H_

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

/** PLL clock outputs */
#define STM32_SRC_PLL_P	0x001
#define STM32_SRC_PLL_Q	0x002
#define STM32_SRC_PLL_R	0x003
/** Fixed clocks */
#define STM32_SRC_LSE	0x004
#define STM32_SRC_LSI	0x005
#define STM32_SRC_HSI	0x008
/** System clock */
#define STM32_SRC_SYSCLK 0x006
/** Peripheral bus clock */
#define STM32_SRC_PCLK	0x007

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

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x70

/** @brief Device domain clocks selection helpers */
/** BDCR devices */
#define RTC_SEL(val)		STM32_CLOCK(val, 3, 8, BDCR_REG)

/** @brief RCC_DKCFGR register offset */
#define DCKCFGR1_REG		0x8C
#define DCKCFGR2_REG		0x90

/** @brief Dedicated clocks configuration register selection helpers */
/** DKCFGR2 devices */
#define USART1_SEL(val)		STM32_CLOCK(val, 3, 0, DCKCFGR2_REG)
#define USART2_SEL(val)		STM32_CLOCK(val, 3, 2, DCKCFGR2_REG)
#define USART3_SEL(val)		STM32_CLOCK(val, 3, 4, DCKCFGR2_REG)
#define USART4_SEL(val)		STM32_CLOCK(val, 1, 6, DCKCFGR2_REG)
#define USART5_SEL(val)		STM32_CLOCK(val, 3, 8, DCKCFGR2_REG)
#define USART6_SEL(val)		STM32_CLOCK(val, 3, 10, DCKCFGR2_REG)
#define USART7_SEL(val)		STM32_CLOCK(val, 3, 12, DCKCFGR2_REG)
#define USART8_SEL(val)		STM32_CLOCK(val, 3, 14, DCKCFGR2_REG)
#define I2C1_SEL(val)		STM32_CLOCK(val, 3, 16, DCKCFGR2_REG)
#define I2C2_SEL(val)		STM32_CLOCK(val, 3, 18, DCKCFGR2_REG)
#define I2C3_SEL(val)		STM32_CLOCK(val, 3, 20, DCKCFGR2_REG)
#define LPTIM1_SEL(val)		STM32_CLOCK(val, 3, 24, DCKCFGR2_REG)
#define CK48M_SEL(val)		STM32_CLOCK(val, 1, 27, DCKCFGR2_REG)
#define SDMMC1_SEL(val)		STM32_CLOCK(val, 1, 28, DCKCFGR2_REG)
#define SDMMC2_SEL(val)		STM32_CLOCK(val, 1, 29, DCKCFGR2_REG)
/** Dummy: Add a specificier when no selection is possible */
#define NO_SEL			0xFF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F7_CLOCK_H_ */
