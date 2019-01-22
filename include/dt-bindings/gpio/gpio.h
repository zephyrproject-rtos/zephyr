/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_

/** GPIO pin is active (has logical value '1') in low state. */
#define GPIO_ACTIVE_LOW         (1 << 0)
/** GPIO pin is active (has logical value '1') in high state. */
#define GPIO_ACTIVE_HIGH        (0 << 0)

/** Configures GPIO output in single-ended mode (open drain or open source). */
#define GPIO_SINGLE_ENDED       (1 << 1)
/** Configures GPIO output in push-pull mode */
#define GPIO_PUSH_PULL          (0 << 1)

/** Indicates single ended open drain mode (wired AND). */
#define GPIO_LINE_OPEN_DRAIN    (1 << 2)
/** Indicates single ended open source mode (wired OR). */
#define GPIO_LINE_OPEN_SOURCE   (0 << 2)

/** Configures GPIO output in open drain mode (wired AND).
 *
 * @note 'Open Drain' mode also known as 'Open Collector' is an output
 * configuration which behaves like a switch that is either connected to ground
 * or disconnected.
 */
#define GPIO_OPEN_DRAIN         (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)
/** Configures GPIO output in open source mode (wired OR).
 *
 * @note 'Open Source' is a term used by software engineers to describe output
 * mode opposite to 'Open Drain'. It has no corresponding hardware schematic
 * and is generally not used by hardware engineers.
 */
#define GPIO_OPEN_SOURCE        (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_SOURCE)

/** Enables pin as input. */
#define GPIO_INPUT_ENABLE       (1 << 3)
/** Disables pin as input. */
#define GPIO_INPUT_DISABLE      (0 << 3)

/** Enables pin as output (no change to the output state). */
#define GPIO_OUTPUT_ENABLE      (1 << 4)
/** Disables pin as output. */
#define GPIO_OUTPUT_DISABLE     (0 << 4)

/** Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    (1 << 5)

/** Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   (1 << 6)

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT_ENABLE | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT_ENABLE | GPIO_OUTPUT_INIT_HIGH)

/** Enables GPIO pin debounce. */
#define GPIO_INPUT_DEBOUNCE     (1 << 7)

/**
 * @name GPIO interrupt flags.
 * The `GPIO_INT_*` flags are used to specify how input GPIO pins will trigger
 * interrupts.
 * @{
 */
/** Enables GPIO pin interrupt.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_ENABLE         (1 << 8)

/** GPIO interrupt is level sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LEVEL          (0 << 9)

/** GPIO interrupt is edge sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_EDGE           (1 << 9)

/** Trigger detection when input state is (or transitions to) low.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LOW            (1 << 10)

/** Trigger detection on input state is (or transitions to) high.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_HIGH           (1 << 11)

/** Trigger GPIO pin interrupt on rising edge. */
#define GPIO_INT_EDGE_RISING    (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_HIGH)

/** Trigger GPIO pin interrupt on falling edge. */
#define GPIO_INT_EDGE_FALLING   (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_LOW)

/** Trigger GPIO pin interrupt on rising or falling edge. */
#define GPIO_INT_EDGE_BOTH      (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_LOW	 \
				 | GPIO_INT_HIGH)

/** Trigger GPIO pin interrupt on level low. */
#define GPIO_INT_LEVEL_LOW      (GPIO_INT_ENABLE  \
				 | GPIO_INT_LEVEL \
				 | GPIO_INT_LOW)

/** Trigger GPIO pin interrupt on level high. */
#define GPIO_INT_LEVEL_HIGH     (GPIO_INT_ENABLE  \
				 | GPIO_INT_LEVEL \
				 | GPIO_INT_HIGH)

/** @} */

/**
 * @name GPIO pull flags.
 * The `GPIO_PUD_*` flags are used to specify the pull-up or pull-down
 * electrical configuration of a GPIO pin.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_SHIFT		12
#define GPIO_PUD_MASK		(0x3 << GPIO_PUD_SHIFT)
/** @endcond */

/** Pin is neither pull-up nor pull-down. */
#define GPIO_PUD_NORMAL		(0 << GPIO_PUD_SHIFT)

/** Enable GPIO pin pull-up. */
#define GPIO_PUD_PULL_UP	(1 << GPIO_PUD_SHIFT)

/** Enable GPIO pin pull-down. */
#define GPIO_PUD_PULL_DOWN	(2 << GPIO_PUD_SHIFT)
/** @} */


/**
 * @name GPIO drive strength flags.
 * The `GPIO_DS_*` flags are used to specify the drive strength configuration of
 * a GPIO pin.
 *
 * The drive strength of individual pins can be configured
 * independently for when the pin output is low and high.
 *
 * The `GPIO_DS_*_LOW` enumerations define the drive strength of a pin
 * when output is low.

 * The `GPIO_DS_*_HIGH` enumerations define the drive strength of a pin
 * when output is high.
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
#define GPIO_DS_LOW_SHIFT 14
#define GPIO_DS_LOW_MASK (0x3 << GPIO_DS_LOW_SHIFT)
/** @endcond */

/** Use default drive strength when GPIO pin output is low. */
#define GPIO_DS_DFLT_LOW        (0 << GPIO_DS_LOW_SHIFT)

/** Use low/reduced drive strength when GPIO pin output is low. */
#define GPIO_DS_LO_LOW          (1 << GPIO_DS_LOW_SHIFT)

/** Use high/increased drive strength when GPIO pin output is low. */
#define GPIO_DS_HI_LOW          (2 << GPIO_DS_LOW_SHIFT)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_HIGH_SHIFT 16
#define GPIO_DS_HIGH_MASK (0x3 << GPIO_DS_HIGH_SHIFT)
/** @endcond */

/** Use default drive strength when GPIO pin output is high. */
#define GPIO_DS_DFLT_HIGH        (0 << GPIO_DS_HIGH_SHIFT)

/** Use low/reduced drive strength when GPIO pin output is high. */
#define GPIO_DS_LO_HIGH          (1 << GPIO_DS_HIGH_SHIFT)

/** Use high/increased drive strength when GPIO pin output is high. */
#define GPIO_DS_HI_HIGH          (2 << GPIO_DS_HIGH_SHIFT)
/** @} */

/** @name Deprecated Flags.
 *
 * These flags were defined at a time when the characteristic they
 * identified was assumed to be relevant only for interrupts.  The
 * features are also useful for non-interrupt inputs and in some cases
 * for outputs.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_SHIFT          3
#define GPIO_DIR_MASK		(0x3 << GPIO_DIR_SHIFT)
/** @endcond */

/** Legacy flag indicating pin is configured as input only.
 *
 * @deprecated Replace with `GPIO_INPUT_ENABLE`
 */
#define GPIO_DIR_IN             GPIO_INPUT_ENABLE

/** Legacy flag indicating pin is configured as output.
 *
 * @deprecated Replace with `GPIO_OUTPUT_ENABLE`
 */
#define GPIO_DIR_OUT            GPIO_OUTPUT_ENABLE

/** Legacy alias identifying high drive strength when GPIO output is low.
 *
 * @deprecated Replace with `GPIO_DS_HI_LOW`
 */
#define GPIO_DS_ALT_LOW         GPIO_DS_HI_LOW

/** Legacy flag indicating pin is disconnected when GPIO pin output is low.
 *
 * @deprecated Replace with `GPIO_OPEN_SOURCE`
 */
#define GPIO_DS_DISCONNECT_LOW (0x3 << GPIO_DS_LOW_SHIFT)

/** Legacy alias identifying high drive strength when GPIO output is high.
 *
 * @deprecated Replace with `GPIO_DS_HI_HIGH`
 */
#define GPIO_DS_ALT_HIGH        GPIO_DS_HI_HIGH

/** Legacy flag indicating pin is disconnected when GPIO pin output is high.
 *
 * @deprecated Replace with `GPIO_OPEN_DRAIN`
 */
#define GPIO_DS_DISCONNECT_HIGH (0x3 << GPIO_DS_HIGH_SHIFT)

/** Legacy flag indicating that interrupts should be enabled on the GPIO.
 *
 * @deprecated Replace with `GPIO_INT_ENABLE`
 */
#define GPIO_INT                GPIO_INT_ENABLE

/** Legacy flag setting indicating signal or interrupt active level.
 *
 * This flag was used both to indicate a signal's active level, and to
 * indicate the level associated with an interrupt on a signal.  As
 * active level is also relevant to output signals the two
 * interpretations have been separated.  The legacy value supports
 * testing for interrupt level as this is the most common use in
 * existing code.
 *
 * @deprecated Replace with `GPIO_ACTIVE_LOW` or `GPIO_INT_LOW`
 * depending on intent.
 */
#define GPIO_INT_ACTIVE_LOW     GPIO_INT_LOW

/** Legacy flag setting indicating signal or interrupt active level.
 *
 * This flag was used both to indicate a signal's active level, and to
 * indicate the level associated with an interrupt on a signal.  As
 * active level is also relevant to output signals the two
 * interpretations have been separated.  The legacy value supports
 * testing for interrupt level as this is the most common use in
 * existing code.
 *
 * @deprecated Replace with `GPIO_ACTIVE_HIGH` or `GPIO_INT_HIGH`
 * depending on intent.
 */
#define GPIO_INT_ACTIVE_HIGH    GPIO_INT_HIGH

/** Legacy flag indicating interrupt triggers on both rising and falling edge.
 *
 * @deprecated Replace with `GPIO_INT_EDGE_BOTH`.
 */
#define GPIO_INT_DOUBLE_EDGE    (1 << 18)

/** Legacy flag indicating a desire to debounce the signal.
 *
 * @deprecated Replace with `GPIO_INPUT_DEBOUNCE`
 */
#define GPIO_INT_DEBOUNCE       GPIO_INPUT_DEBOUNCE

/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_SHIFT		19
#define GPIO_POL_MASK		(1 << GPIO_POL_SHIFT)
/** @endcond */

/** Legacy flag indicating that GPIO pin polarity is normal.
 *
 * @deprecated Replace with `GPIO_ACTIVE_HIGH`.
 */
#define GPIO_POL_NORMAL		(0 << GPIO_POL_SHIFT)

/** Legacy flag indicating that GPIO pin polarity is inverted.
 *
 * @deprecated Replace with `GPIO_ACTIVE_LOW`.
 */
#define GPIO_POL_INV		(1 << GPIO_POL_SHIFT)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_ */
