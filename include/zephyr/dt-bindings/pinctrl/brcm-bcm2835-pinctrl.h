/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BRCM_BCM2835_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BRCM_BCM2835_PINCTRL_H_

/**
 * @file
 * @brief BCM2835 pinctrl devicetree constants.
 */

/** Pin number field position in a BCM2835 pinmux value. */
#define BCM2835_PIN_NUM_POS	0U
/** Pin number field mask in a BCM2835 pinmux value. */
#define BCM2835_PIN_NUM_MASK	0xffU
/** Pin function field position in a BCM2835 pinmux value. */
#define BCM2835_PIN_FUNC_POS	8U
/** Pin function field mask in a BCM2835 pinmux value. */
#define BCM2835_PIN_FUNC_MASK	0x7U

/** GPIO input function. */
#define BCM2835_INPUT	0U
/** GPIO output function. */
#define BCM2835_OUTPUT	1U
/** Alternate function 5. */
#define BCM2835_ALT5	2U
/** Alternate function 4. */
#define BCM2835_ALT4	3U
/** Alternate function 0. */
#define BCM2835_ALT0	4U
/** Alternate function 1. */
#define BCM2835_ALT1	5U
/** Alternate function 2. */
#define BCM2835_ALT2	6U
/** Alternate function 3. */
#define BCM2835_ALT3	7U

/** Build a BCM2835 pinmux value from a pin number and function. */
#define BCM2835_PINMUX(pin, func) \
	((((pin) & BCM2835_PIN_NUM_MASK) << BCM2835_PIN_NUM_POS) | \
	 (((func) & BCM2835_PIN_FUNC_MASK) << BCM2835_PIN_FUNC_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BRCM_BCM2835_PINCTRL_H_ */
