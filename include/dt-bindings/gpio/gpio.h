/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_


/**
 * @name GPIO direction flags
 * The `GPIO_DIR_*` flags are used with `gpio_pin_configure` or `gpio_port_configure`,
 * to specify whether a GPIO pin will be used for input or output.
 * @{
 */
/** GPIO pin to be input. */
#define GPIO_DIR_IN		(0 << 0)

/** GPIO pin to be output. */
#define GPIO_DIR_OUT		(1 << 0)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK		0x1
/** @endcond */
/** @} */


/**
 * @name GPIO interrupt flags
 * The `GPIO_INT_*` flags are used with `gpio_pin_configure` or `gpio_port_configure`,
 * to specify how input GPIO pins will trigger interrupts.
 * @{
 */
/** GPIO pin to trigger interrupt. */
#define GPIO_INT		(1 << 1)

/** GPIO pin trigger on level low or falling edge. */
#define GPIO_INT_ACTIVE_LOW	(0 << 2)

/** GPIO pin trigger on level high or rising edge. */
#define GPIO_INT_ACTIVE_HIGH	(1 << 2)

/** Enable GPIO pin debounce. */
#define GPIO_INT_DEBOUNCE       (1 << 4)

/** Do Level trigger. */
#define GPIO_INT_LEVEL		(0 << 5)

/** Do Edge trigger. */
#define GPIO_INT_EDGE		(1 << 5)

/** Interrupt triggers on both rising and falling edge.
 *  Must be combined with GPIO_INT_EDGE.
 */
#define GPIO_INT_DOUBLE_EDGE	(1 << 6)
/** @} */


/**
 * @name GPIO polarity flags
 * The `GPIO_POL_*` flags are used with `gpio_pin_configure` or `gpio_port_configure`,
 * to specify the polarity of a GPIO pin.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_POS		7
/** @endcond */

/** GPIO pin polarity is normal. */
#define GPIO_POL_NORMAL		(0 << GPIO_POL_POS)

/** GPIO pin polarity is inverted. */
#define GPIO_POL_INV		(1 << GPIO_POL_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_MASK		(1 << GPIO_POL_POS)
/** @endcond */
/** @} */


/**
 * @name GPIO pull flags
 * The `GPIO_PUD_*` flags are used with `gpio_pin_configure` or `gpio_port_configure`,
 * to specify the pull-up or pull-down electrical configuration of a GPIO pin.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_POS		8
/** @endcond */

/** Pin is neither pull-up nor pull-down. */
#define GPIO_PUD_NORMAL		(0 << GPIO_PUD_POS)

/** Enable GPIO pin pull-up. */
#define GPIO_PUD_PULL_UP	(1 << GPIO_PUD_POS)

/** Enable GPIO pin pull-down. */
#define GPIO_PUD_PULL_DOWN	(2 << GPIO_PUD_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_MASK		(3 << GPIO_PUD_POS)
/** @endcond */
/** @} */


/**
 * @name GPIO drive strength flags
 * The `GPIO_DS_*` flags are used with `gpio_pin_configure` or `gpio_port_configure`,
 * to specify the drive strength configuration of a GPIO pin.
 *
 * The drive strength of individual pins can be configured
 * independently for when the pin output is low and high.
 *
 * The `GPIO_DS_*_LOW` enumerations define the drive strength of a pin
 * when output is low.

 * The `GPIO_DS_*_HIGH` enumerations define the drive strength of a pin
 * when output is high.
 *
 * The `DISCONNECT` drive strength indicates that the pin is placed in a
 * high impedance state and not driven, this option is used to
 * configure hardware that supports a open collector drive mode.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
 *
 * On hardware that supports only one standard drive strength, both
 * `DFLT` and `ALT` have the same behavior.
 *
 * On hardware that does not support a disconnect mode, `DISCONNECT`
 * will behave the same as `DFLT`.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_LOW_POS 12
#define GPIO_DS_LOW_MASK (0x3 << GPIO_DS_LOW_POS)
/** @endcond */

/** Default drive strength standard when GPIO pin output is low.
 */
#define GPIO_DS_DFLT_LOW (0x0 << GPIO_DS_LOW_POS)

/** Alternative drive strength when GPIO pin output is low.
 * For hardware that does not support configurable drive strength
 * use the default drive strength.
 */
#define GPIO_DS_ALT_LOW (0x1 << GPIO_DS_LOW_POS)

/** Disconnect pin when GPIO pin output is low.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_LOW (0x3 << GPIO_DS_LOW_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_HIGH_POS 14
#define GPIO_DS_HIGH_MASK (0x3 << GPIO_DS_HIGH_POS)
/** @endcond */

/** Default drive strength when GPIO pin output is high.
 */
#define GPIO_DS_DFLT_HIGH (0x0 << GPIO_DS_HIGH_POS)

/** Alternative drive strength when GPIO pin output is high.
 * For hardware that does not support configurable drive strengths
 * use the default drive strength.
 */
#define GPIO_DS_ALT_HIGH (0x1 << GPIO_DS_HIGH_POS)

/** Disconnect pin when GPIO pin output is high.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_HIGH (0x3 << GPIO_DS_HIGH_POS)
/** @} */



#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_ */
