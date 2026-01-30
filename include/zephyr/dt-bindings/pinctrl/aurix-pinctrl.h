/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Device tree pinctrl encoding for the Infineon AURIX TC3x/TC4x families.
 *
 * Each devicetree pinmux cell on AURIX is a packed 32-bit word that
 * carries the GPIO port, pin index, alternate-function select, signal
 * type (input vs output) and pad-drive type. The AURIX_PIN_*, _PORT_*,
 * _ALT_*, _TYPE_* and _PAD_TYPE_* shift/mask pairs in this header are
 * the canonical layout; the AURIX_INPUT_PINMUX() and
 * AURIX_OUTPUT_PINMUX() helpers compose that word from named fields,
 * and the AURIX_PINMUX_<field>() helpers decode it for the pinctrl
 * driver.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_

/* Bit Masks */

/** Pin-index field bit position inside the pinmux word. */
#define AURIX_PIN_POS  0
/** Pin-index field bit mask (relative to AURIX_PIN_POS). */
#define AURIX_PIN_MASK 0xF

/** Port-index field bit position inside the pinmux word. */
#define AURIX_PORT_POS  4
/** Port-index field bit mask (relative to AURIX_PORT_POS). */
#define AURIX_PORT_MASK 0xFF

/** Alternate-function-select field bit position inside the pinmux word. */
#define AURIX_ALT_POS  12
/** Alternate-function-select field bit mask (relative to AURIX_ALT_POS). */
#define AURIX_ALT_MASK 0xF

/** Direction/type field bit position inside the pinmux word. */
#define AURIX_TYPE_POS  16
/** Direction/type field bit mask (relative to AURIX_TYPE_POS). */
#define AURIX_TYPE_MASK 0xF

/** Pad-drive-type field bit position inside the pinmux word. */
#define AURIX_PAD_TYPE_POS  20
/** Pad-drive-type field bit mask (relative to AURIX_PAD_TYPE_POS). */
#define AURIX_PAD_TYPE_MASK 0xF

/** Input alternate function select A. */
#define AURIX_INPUT_SELECT_A 0
/** Input alternate function select B. */
#define AURIX_INPUT_SELECT_B 1
/** Input alternate function select C. */
#define AURIX_INPUT_SELECT_C 2
/** Input alternate function select D. */
#define AURIX_INPUT_SELECT_D 3
/** Input alternate function select E. */
#define AURIX_INPUT_SELECT_E 4
/** Input alternate function select F. */
#define AURIX_INPUT_SELECT_F 5
/** Input alternate function select G. */
#define AURIX_INPUT_SELECT_G 6
/** Input alternate function select H. */
#define AURIX_INPUT_SELECT_H 7

/** Output alternate function select O0. */
#define AURIX_OUTPUT_SELECT_O0  0
/** Output alternate function select O1. */
#define AURIX_OUTPUT_SELECT_O1  1
/** Output alternate function select O2. */
#define AURIX_OUTPUT_SELECT_O2  2
/** Output alternate function select O3. */
#define AURIX_OUTPUT_SELECT_O3  3
/** Output alternate function select O4. */
#define AURIX_OUTPUT_SELECT_O4  4
/** Output alternate function select O5. */
#define AURIX_OUTPUT_SELECT_O5  5
/** Output alternate function select O6. */
#define AURIX_OUTPUT_SELECT_O6  6
/** Output alternate function select O7. */
#define AURIX_OUTPUT_SELECT_O7  7
/** Output alternate function select O8. */
#define AURIX_OUTPUT_SELECT_O8  8
/** Output alternate function select O9. */
#define AURIX_OUTPUT_SELECT_O9  9
/** Output alternate function select O10. */
#define AURIX_OUTPUT_SELECT_O10 10
/** Output alternate function select O11. */
#define AURIX_OUTPUT_SELECT_O11 11
/** Output alternate function select O12. */
#define AURIX_OUTPUT_SELECT_O12 12
/** Output alternate function select O13. */
#define AURIX_OUTPUT_SELECT_O13 13
/** Output alternate function select O14. */
#define AURIX_OUTPUT_SELECT_O14 14
/** Output alternate function select O15. */
#define AURIX_OUTPUT_SELECT_O15 15

/** Slow pad-drive type. */
#define AURIX_PAD_TYPE_SLOW   0
/** Fast pad-drive type. */
#define AURIX_PAD_TYPE_FAST   1
/** Reduced-fast pad-drive type. */
#define AURIX_PAD_TYPE_RFAST  2
/** High-speed-fast pad-drive type (alias of RFAST). */
#define AURIX_PAD_TYPE_HSFAST 2

/* Setters and getters */

/**
 * @brief Build a pinmux word for an input-direction pin.
 *
 * @param port     GPIO port index.
 * @param pin      Pin index inside the port.
 * @param alt_fun  Input alternate-function selector token (A..H), used
 *                 as a suffix to AURIX_INPUT_SELECT_.
 * @param type     Direction/type field value.
 * @param pad_type Pad-drive-type token (SLOW, FAST, RFAST, HSFAST), used
 *                 as a suffix to AURIX_PAD_TYPE_.
 */
#define AURIX_INPUT_PINMUX(port, pin, alt_fun, type, pad_type)                                     \
	(((port) << AURIX_PORT_POS) | ((pin) << AURIX_PIN_POS) |                                   \
	 ((AURIX_INPUT_SELECT_##alt_fun) << AURIX_ALT_POS) | ((type) << AURIX_TYPE_POS) |          \
	 ((AURIX_PAD_TYPE_##pad_type) << AURIX_PAD_TYPE_POS))
/**
 * @brief Build a pinmux word for an output-direction pin.
 *
 * @param port     GPIO port index.
 * @param pin      Pin index inside the port.
 * @param alt_fun  Output alternate-function selector token (O0..O15),
 *                 used as a suffix to AURIX_OUTPUT_SELECT_.
 * @param pad_type Pad-drive-type token (SLOW, FAST, RFAST, HSFAST), used
 *                 as a suffix to AURIX_PAD_TYPE_.
 */
#define AURIX_OUTPUT_PINMUX(port, pin, alt_fun, pad_type)                                          \
	(((port) << AURIX_PORT_POS) | ((pin) << AURIX_PIN_POS) |                                   \
	 ((AURIX_OUTPUT_SELECT_##alt_fun) << AURIX_ALT_POS) |                                      \
	 ((AURIX_PAD_TYPE_##pad_type) << AURIX_PAD_TYPE_POS))

/** Extract the port index from a pinmux word. */
#define AURIX_PINMUX_PORT(pinmux)     (((pinmux) >> AURIX_PORT_POS) & AURIX_PORT_MASK)
/** Extract the pin index from a pinmux word. */
#define AURIX_PINMUX_PIN(pinmux)      (((pinmux) >> AURIX_PIN_POS) & AURIX_PIN_MASK)
/** Extract the alternate-function selector from a pinmux word. */
#define AURIX_PINMUX_ALT(pinmux)      (((pinmux) >> AURIX_ALT_POS) & AURIX_ALT_MASK)
/** Extract the direction/type field from a pinmux word. */
#define AURIX_PINMUX_TYPE(pinmux)     (((pinmux) >> AURIX_TYPE_POS) & AURIX_TYPE_MASK)
/** Extract the pad-drive-type field from a pinmux word. */
#define AURIX_PINMUX_PAD_TYPE(pinmux) (((pinmux) >> AURIX_PAD_TYPE_POS) & AURIX_PAD_TYPE_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AURIX_PINCTRL_H_ */
