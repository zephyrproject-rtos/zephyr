/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pinmux specifiers for Puya PY32 devices.
 * @ingroup pinctrl_py32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PY32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PY32_PINCTRL_H_

/**
 * @defgroup pinctrl_py32 Puya PY32 pin control helpers
 * @ingroup devicetree-pinctrl
 * @{
 */

/** GPIO port A index. */
#define PY32_PORTA 0U
/** GPIO port B index. */
#define PY32_PORTB 1U
/** GPIO port F index. */
#define PY32_PORTF 5U

/** Alternate function 0. */
#define PY32_AF0    0x0U
/** Alternate function 1. */
#define PY32_AF1    0x1U
/** Alternate function 2. */
#define PY32_AF2    0x2U
/** Alternate function 3. */
#define PY32_AF3    0x3U
/** Alternate function 4. */
#define PY32_AF4    0x4U
/** Alternate function 5. */
#define PY32_AF5    0x5U
/** Alternate function 6. */
#define PY32_AF6    0x6U
/** Alternate function 7. */
#define PY32_AF7    0x7U
/** Alternate function 8. */
#define PY32_AF8    0x8U
/** Alternate function 9. */
#define PY32_AF9    0x9U
/** Alternate function 10. */
#define PY32_AF10   0xaU
/** Alternate function 11. */
#define PY32_AF11   0xbU
/** Alternate function 12. */
#define PY32_AF12   0xcU
/** Alternate function 13. */
#define PY32_AF13   0xdU
/** Alternate function 14. */
#define PY32_AF14   0xeU
/** Alternate function 15. */
#define PY32_AF15   0xfU
/** Analog pin mode. */
#define PY32_ANALOG 0x10U
/** General-purpose input/output pin mode. */
#define PY32_GPIO   0x11U

/** @cond INTERNAL_HIDDEN */
#define PY32_MODE_SHIFT 0U
#define PY32_MODE_MASK  0x1FU
#define PY32_LINE_SHIFT 5U
#define PY32_LINE_MASK  0xFU
#define PY32_PORT_SHIFT 9U
#define PY32_PORT_MASK  0x1FU
/** @endcond */

/**
 * @brief Encode a GPIO port, pin number and function as a pinmux specifier.
 *
 * Fields:
 *
 * - mode [ 0 : 4 ]
 * - line [ 5 : 8 ]
 * - port [ 9 : 13 ]
 *
 * @param port Port letter ('A', 'B' or 'F').
 * @param line Pin (0..15)
 * @param mode Function (AF0 through AF15, ANALOG or GPIO).
 * @return Encoded pinmux specifier.
 */
#define PY32_PINMUX(port, line, mode)                                                              \
	(((((port) - 'A') & PY32_PORT_MASK) << PY32_PORT_SHIFT) |                                  \
	 (((line) & PY32_LINE_MASK) << PY32_LINE_SHIFT) |                                          \
	 (((PY32_##mode) & PY32_MODE_MASK) << PY32_MODE_SHIFT))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_PY32_PINCTRL_H_ */
