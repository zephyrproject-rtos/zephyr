/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_

/* System clock */
/** @brief Clock source identifier for SYSCLK */
#define STM32_SRC_SYSCLK	0x001
/* Fixed clocks */
/** @brief Clock source identifier for LSE */
#define STM32_SRC_LSE		0x002
/** @brief Clock source identifier for LSI */
#define STM32_SRC_LSI		0x003

/* Dummy: Add a specifier when no selection is possible */
/** @brief Dummy specifier used when no kernel clock source selection is available */
#define NO_SEL			0xFF

/** @brief Bit shift position for the STM32 clock divider field */
#define STM32_CLOCK_DIV_SHIFT	12

/* Clock divider */
/** @brief Compute the STM32 clock divider configuration field value from a divisor */
#define STM32_CLOCK_DIV(div)	(((div) - 1) << STM32_CLOCK_DIV_SHIFT)

/* Helper macros to pack RCC clock source selection register info in the DT */
/** @brief DT clock select helper: reg mask */
#define STM32_DT_CLKSEL_REG_MASK	0xFFFFU
/** @brief DT clock select helper: reg shift */
#define STM32_DT_CLKSEL_REG_SHIFT	0U
/** @brief DT clock select helper: shift mask */
#define STM32_DT_CLKSEL_SHIFT_MASK	0x1FU
/** @brief DT clock select helper: shift shift */
#define STM32_DT_CLKSEL_SHIFT_SHIFT	16U
/** @brief DT clock select helper: width mask */
#define STM32_DT_CLKSEL_WIDTH_MASK	0x3U
/** @brief DT clock select helper: width shift */
#define STM32_DT_CLKSEL_WIDTH_SHIFT	21U
/** @brief DT clock select helper: val mask */
#define STM32_DT_CLKSEL_VAL_MASK	0xFFU
/** @brief DT clock select helper: val shift */
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
