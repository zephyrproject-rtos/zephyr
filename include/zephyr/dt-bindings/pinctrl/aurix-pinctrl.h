/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_

/* Bit Masks */

#define AURIX_PIN_POS  0
#define AURIX_PIN_MASK 0xF

#define AURIX_PORT_POS  4
#define AURIX_PORT_MASK 0xFF

#define AURIX_ALT_POS  12
#define AURIX_ALT_MASK 0xF

#define AURIX_TYPE_POS  16
#define AURIX_TYPE_MASK 0xF

#define AURIX_PAD_TYPE_POS  20
#define AURIX_PAD_TYPE_MASK 0xF

#define AURIX_INPUT_SELECT_A 0
#define AURIX_INPUT_SELECT_B 1
#define AURIX_INPUT_SELECT_C 2
#define AURIX_INPUT_SELECT_D 3
#define AURIX_INPUT_SELECT_E 4
#define AURIX_INPUT_SELECT_F 5
#define AURIX_INPUT_SELECT_G 6
#define AURIX_INPUT_SELECT_H 7

#define AURIX_OUTPUT_SELECT_O0  0
#define AURIX_OUTPUT_SELECT_O1  1
#define AURIX_OUTPUT_SELECT_O2  2
#define AURIX_OUTPUT_SELECT_O3  3
#define AURIX_OUTPUT_SELECT_O4  4
#define AURIX_OUTPUT_SELECT_O5  5
#define AURIX_OUTPUT_SELECT_O6  6
#define AURIX_OUTPUT_SELECT_O7  7
#define AURIX_OUTPUT_SELECT_O8  8
#define AURIX_OUTPUT_SELECT_O9  9
#define AURIX_OUTPUT_SELECT_O10 10
#define AURIX_OUTPUT_SELECT_O11 11
#define AURIX_OUTPUT_SELECT_O12 12
#define AURIX_OUTPUT_SELECT_O13 13
#define AURIX_OUTPUT_SELECT_O14 14
#define AURIX_OUTPUT_SELECT_O15 15

#define AURIX_PAD_TYPE_SLOW   0
#define AURIX_PAD_TYPE_FAST   1
#define AURIX_PAD_TYPE_RFAST  2
#define AURIX_PAD_TYPE_HSFAST 2

/* Setters and getters */

#define AURIX_INPUT_PINMUX(port, pin, alt_fun, type, pad_type)                                     \
	(((port) << AURIX_PORT_POS) | ((pin) << AURIX_PIN_POS) |                                   \
	 ((AURIX_INPUT_SELECT_##alt_fun) << AURIX_ALT_POS) | ((type) << AURIX_TYPE_POS) |          \
	 ((AURIX_PAD_TYPE_##pad_type) << AURIX_PAD_TYPE_POS))
#define AURIX_OUTPUT_PINMUX(port, pin, alt_fun, pad_type)                                          \
	(((port) << AURIX_PORT_POS) | ((pin) << AURIX_PIN_POS) |                                   \
	 ((AURIX_OUTPUT_SELECT_##alt_fun) << AURIX_ALT_POS) |                                      \
	 ((AURIX_PAD_TYPE_##pad_type) << AURIX_PAD_TYPE_POS))

#define AURIX_PINMUX_PORT(pinmux)     (((pinmux) >> AURIX_PORT_POS) & AURIX_PORT_MASK)
#define AURIX_PINMUX_PIN(pinmux)      (((pinmux) >> AURIX_PIN_POS) & AURIX_PIN_MASK)
#define AURIX_PINMUX_ALT(pinmux)      (((pinmux) >> AURIX_ALT_POS) & AURIX_ALT_MASK)
#define AURIX_PINMUX_TYPE(pinmux)     (((pinmux) >> AURIX_TYPE_POS) & AURIX_TYPE_MASK)
#define AURIX_PINMUX_PAD_TYPE(pinmux) (((pinmux) >> AURIX_PAD_TYPE_POS) & AURIX_PAD_TYPE_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_ */
