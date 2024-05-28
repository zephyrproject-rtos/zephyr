/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_

/** Define system & low-speed clocks */
#include "stm32_common_clocks.h"

/** Other fixed clocks.
 * - CLKSLOWMUX: used to query slow clock tree frequency
 * - CLK16MHZ: secondary clock for LPUART, SPI3/I2S and BLE
 * - CLK32MHZ: secondary clock for SPI3/I2S and BLE
 */
#define STM32_SRC_CLKSLOWMUX		(STM32_SRC_LSI + 1)
#define STM32_SRC_CLK16MHZ		(STM32_SRC_CLKSLOWMUX + 1)
#define STM32_SRC_CLK32MHZ		(STM32_SRC_CLK16MHZ + 1)

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB0	0x50
#define STM32_CLOCK_BUS_APB0	0x54
#define STM32_CLOCK_BUS_APB1	0x58
#define STM32_CLOCK_BUS_APB2	0x60

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB0
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB2

#define STM32_CLOCK_REG_MASK	(0xFFFFU)
#define STM32_CLOCK_REG_SHIFT	(0U)
#define STM32_CLOCK_SHIFT_MASK  (0x3FU)
#define STM32_CLOCK_SHIFT_SHIFT (16U)
#define STM32_CLOCK_MASK_MASK   (0x1FU)
#define STM32_CLOCK_MASK_SHIFT  (22U)
#define STM32_CLOCK_VAL_MASK    STM32_CLOCK_MASK_MASK
#define STM32_CLOCK_VAL_SHIFT   (27U)

/**
 * @brief STM32 clock configuration bit field
 *
 * @param reg	Offset to target configuration register in RCC
 * @param shift	Position of field within RCC register (= field LSB's index)
 * @param mask	Mask of field in RCC register
 * @param val	Field value
 *
 * @note 'reg' range:	0x0~0xFFFF	[ 00 : 15 ]
 * @note 'shift' range:	0~63		[ 16 : 21 ]
 * @note 'mask' range:	0x00~0x1F	[ 22 : 26 ]
 * @note 'val' range:	0x00~0x1F	[ 27 : 31 ]
 */
#define STM32_CLOCK(val, mask, shift, reg)				\
	((((reg) & STM32_CLOCK_REG_MASK) << STM32_CLOCK_REG_SHIFT) |		\
	 (((shift) & STM32_CLOCK_SHIFT_MASK) << STM32_CLOCK_SHIFT_SHIFT) |	\
	 (((mask) & STM32_CLOCK_MASK_MASK) << STM32_CLOCK_MASK_SHIFT) |		\
	 (((val) & STM32_CLOCK_VAL_MASK) << STM32_CLOCK_VAL_SHIFT))

/** @brief RCC_CFGR register offset */
#define CFGR_REG	0x08

/** @brief RCC_APB2ENR register offset */
#define APB2ENR_REG	0x60

/** @brief Device clk sources selection helpers */
#define LPUART1_SEL(val)	STM32_CLOCK(val, 1, 13, CFGR_REG)	/* WB05/WB09 only */
#define SPI2_I2S2_SEL(val)	STM32_CLOCK(val, 1, 22, CFGR_REG)	/* WB06/WB07 only */
/* `mask` is only 0x1 for WB06/WB07, but a single definition with mask=0x3 is acceptable */
#define SPI3_I2S3_SEL(val)	STM32_CLOCK(val, 3, 22, CFGR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WB0_CLOCK_H_ */
