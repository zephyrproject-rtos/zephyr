/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_H_

#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h>

/* Adapted from Linux: include/dt-bindings/pinctrl/stm32-pinfunc.h */

/**
 * @brief Pin modes
 */

#define STM32_AF0     0x0
#define STM32_AF1     0x1
#define STM32_AF2     0x2
#define STM32_AF3     0x3
#define STM32_AF4     0x4
#define STM32_AF5     0x5
#define STM32_AF6     0x6
#define STM32_AF7     0x7
#define STM32_AF8     0x8
#define STM32_AF9     0x9
#define STM32_AF10    0xa
#define STM32_AF11    0xb
#define STM32_AF12    0xc
#define STM32_AF13    0xd
#define STM32_AF14    0xe
#define STM32_AF15    0xf
#define STM32_ANALOG  0x10
#define STM32_GPIO    0x11

/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is inspired from Linux equivalent st,stm32f429-pinctrl binding
 */

#define STM32_MODE_SHIFT 0U
#define STM32_MODE_MASK  0x1FU
#define STM32_LINE_SHIFT 5U
#define STM32_LINE_MASK  0xFU
#define STM32_PORT_SHIFT 9U
#define STM32_PORT_MASK  0xFU

/**
 * @brief Pin configuration configuration bit field.
 *
 * Fields:
 *
 * - mode [ 0 : 4 ]
 * - line [ 5 : 8 ]
 * - port [ 9 : 12 ]
 *
 * @param port Port ('A'..'K')
 * @param line Pin (0..15)
 * @param mode Mode (ANALOG, GPIO_IN, ALTERNATE).
 */
#define STM32_PINMUX(port, line, mode)					       \
		(((((port) - 'A') & STM32_PORT_MASK) << STM32_PORT_SHIFT) |    \
		(((line) & STM32_LINE_MASK) << STM32_LINE_SHIFT) |	       \
		(((STM32_ ## mode) & STM32_MODE_MASK) << STM32_MODE_SHIFT))

/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    Alternate Functions [ 0 : 3 ]
 *    GPIO Mode           [ 4 : 5 ]
 *    GPIO Output type    [ 6 ]
 *    GPIO Speed          [ 7 : 8 ]
 *    GPIO PUPD config    [ 9 : 10 ]
 *    GPIO Output data     [ 11 ]
 *
 */

/* GPIO Mode */
#define STM32_MODER_INPUT_MODE		(0x0 << STM32_MODER_SHIFT)
#define STM32_MODER_OUTPUT_MODE		(0x1 << STM32_MODER_SHIFT)
#define STM32_MODER_ALT_MODE		(0x2 << STM32_MODER_SHIFT)
#define STM32_MODER_ANALOG_MODE		(0x3 << STM32_MODER_SHIFT)
#define STM32_MODER_MASK	 	0x3
#define STM32_MODER_SHIFT		4

/* GPIO Output type */
#define STM32_OTYPER_PUSH_PULL		(0x0 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_OPEN_DRAIN		(0x1 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_MASK		0x1
#define STM32_OTYPER_SHIFT		6

/* GPIO speed */
#define STM32_OSPEEDR_LOW_SPEED		(0x0 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MEDIUM_SPEED	(0x1 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_HIGH_SPEED	(0x2 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_VERY_HIGH_SPEED	(0x3 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MASK		0x3
#define STM32_OSPEEDR_SHIFT		7

/* GPIO High impedance/Pull-up/pull-down */
#define STM32_PUPDR_NO_PULL		(0x0 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_UP		(0x1 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_DOWN		(0x2 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_MASK		0x3
#define STM32_PUPDR_SHIFT		9

/* GPIO plain output value */
#define STM32_ODR_0			(0x0 << STM32_ODR_SHIFT)
#define STM32_ODR_1			(0x1 << STM32_ODR_SHIFT)
#define STM32_ODR_MASK			0x1
#define STM32_ODR_SHIFT			11

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_H_ */
