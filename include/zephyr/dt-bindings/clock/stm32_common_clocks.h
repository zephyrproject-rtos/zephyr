/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_

/** System clock */
#define STM32_SRC_SYSCLK	0x001
/** Fixed clocks  */
#define STM32_SRC_LSE		0x002
#define STM32_SRC_LSI		0x003

/** Dummy: Add a specifier when no selection is possible */
#define NO_SEL			0xFF

#define STM32_CLOCK_DIV_SHIFT	12

/** Clock divider */
#define STM32_CLOCK_DIV(div)	(((div) - 1) << STM32_CLOCK_DIV_SHIFT)

/** Helper macros to pack RCC clock source selection register info in the DT */
#define STM32_DT_CLKSEL_REG_MASK	0xFFFFU
#define STM32_DT_CLKSEL_REG_SHIFT	0U
#define STM32_DT_CLKSEL_SHIFT_MASK	0x1FU
#define STM32_DT_CLKSEL_SHIFT_SHIFT	16U
#define STM32_DT_CLKSEL_WIDTH_MASK	0x3U
#define STM32_DT_CLKSEL_WIDTH_SHIFT	21U
#define STM32_DT_CLKSEL_VAL_MASK	0xFFU
#define STM32_DT_CLKSEL_VAL_SHIFT	24U

/**
 * @brief Pack STM32 source clock selection RCC register bit fields for the DT
 *
 * @param val    Clock configuration field value
 * @param msb    Field MSB's index
 * @param lsb    Field LSB's index
 * @param reg    Offset to target clock configuration register in RCC
 *
 * @note Internally, the data are stored as follows
 * @note 'reg' range:    0x0~0xFFFF   [ 00 : 15 ]
 * @note 'shift' range:  0~31         [ 16 : 20 ]
 * @note 'width' range:  0~7          [ 21 : 23 ] Value encodes bit fields width minus 1
 * @note 'val' range:    0x00~0xFF    [ 24 : 31 ]
 */
#define STM32_DT_CLOCK_SELECT(val, msb, lsb, reg)						\
	((((reg) & STM32_DT_CLKSEL_REG_MASK) << STM32_DT_CLKSEL_REG_SHIFT) |			\
	 (((lsb) & STM32_DT_CLKSEL_SHIFT_MASK) << STM32_DT_CLKSEL_SHIFT_SHIFT) |		\
	 ((((msb) - (lsb)) & STM32_DT_CLKSEL_WIDTH_MASK) << STM32_DT_CLKSEL_WIDTH_SHIFT) |	\
	 (((val) & STM32_DT_CLKSEL_VAL_MASK) << STM32_DT_CLKSEL_VAL_SHIFT))

/**
 * Pack RCC clock register offset and bit in two 32-bit values
 * as expected for the Device Tree `clocks` property on STM32.
 *
 * @param bus STM32 bus name (expands to STM32_CLOCK_BUS_{bus})
 * @param bit Clock bit
 */
#define STM32_CLOCK(bus, bit) (STM32_CLOCK_BUS_##bus) (1 << bit)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_ */
