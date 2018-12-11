/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_

/*
 * Assigned Bit Positions
 *
 * @note: This table is intended as a convenience to summarize the bit
 * assignments used in the fields defined below.  The GPIO_foo_SHIFT
 * macros are the canonical definition.
 *
 * Bit   Prefix         Variations
 * ===== ============== =======================================================
 * 0     GPIO_ACTIVE    high or low
 * 1     GPIO_SIGNALING single-ended or push-pull
 * 2     GPIO_LINE      source or drain
 * 3-4   GPIO_PUD       pull normal/up/down
 * 5-8   GPIO_IO        in, out, initialize, initial value
 * 9-13  GPIO_INT       enable, level/edge, low, high, double-edge
 * 14    GPIO_POL       polarity normal/invert
 * 15    GPIO_DEBOUNCE  disable/enable
 * 16-17 GPIO_DS_LOW    drive strength for output low: default/low/high/discon
 * 18-19 GPIO_DS_HIGH   drive strength for output high: default/low/high/discon
 */

/**
 * @name GPIO active level indicator.
 *
 * The `GPIO_ACTIVE_*` flags indicate the level at which a
 * corresponding gpio is considered active, e.g. whether an LED is lit
 * with a low or high output, or a button is pressed with a low or
 * high input.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_ACTIVE_SHIFT       0
#define GPIO_ACTIVE_MASK        (1 << GPIO_ACTIVE_SHIFT)
/** @endcond */

/** GPIO pin indicates active in high state. */
#define GPIO_ACTIVE_HIGH        (0 << GPIO_ACTIVE_SHIFT)

/** GPIO pin indicates active in low state. */
#define GPIO_ACTIVE_LOW         (1 << GPIO_ACTIVE_SHIFT)

/**
 * @name GPIO signaling method and line role.
 *
 * These flags are components used to distinguish `GPIO_OPEN_DRAIN`,
 * `GPIO_OPEN_SOURCE`, and `GPIO_PUSH_PULL`.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_OPEN_SHIFT         1
#define GPIO_SIGNALING_SHIFT    GPIO_OPEN_SHIFT
#define GPIO_SIGNALING_MASK     (1 << GPIO_SIGNALING_SHIFT)
#define GPIO_LINE_SHIFT         (GPIO_OPEN_SHIFT + 1)
#define GPIO_LINE_MASK          (1 << GPIO_LINE_SHIFT)
#define GPIO_OPEN_MASK          (GPIO_SIGNALING_MASK | GPIO_LINE_MASK)
/** @endcond */

/** GPIO uses differential signaling. */
#define GPIO_PUSH_PULL          (0 << GPIO_SIGNALING_SHIFT)

/** GPIO uses single-ended signaling. */
#define GPIO_SINGLE_ENDED       (1 << GPIO_SIGNALING_SHIFT)

/** GPIO is source for signal. */
#define GPIO_LINE_SOURCE        (0 << GPIO_LINE_SHIFT)

/** GPIO is drain for signal. */
#define GPIO_LINE_DRAIN         (1 << GPIO_LINE_SHIFT)

/** GPIO is an open-source/emitter. */
#define GPIO_OPEN_SOURCE        (GPIO_SINGLE_ENDED | GPIO_LINE_SOURCE)

/** GPIO is an open-drain/collector. */
#define GPIO_OPEN_DRAIN         (GPIO_SINGLE_ENDED | GPIO_LINE_DRAIN)

/** @} */

/**
 * @name GPIO pull flags
 * The `GPIO_PUD_*` flags are used with `gpio_pin_configure` or
 * `gpio_port_configure`, to specify the pull-up or pull-down
 * electrical configuration of a GPIO pin.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_SHIFT          3
#define GPIO_PUD_MASK           (3 << GPIO_PUD_SHIFT)
/** @endcond */

/** Pin is neither pull-up nor pull-down. */
#define GPIO_PUD_NORMAL         (0 << GPIO_PUD_SHIFT)

/** Enable GPIO pin pull-up. */
#define GPIO_PUD_PULL_UP        (1 << GPIO_PUD_SHIFT)

/** Enable GPIO pin pull-down. */
#define GPIO_PUD_PULL_DOWN      (2 << GPIO_PUD_SHIFT)

/** @} */

/**
 * @name GPIO input/output configuration.
 * The `GPIO_IO_*` flags are used with `gpio_pin_configure` or
 * `gpio_port_configure`, to specify whether a GPIO pin will be used
 * for input and/or output and the initial state for output.
 *
 * The `GPIO_DIR_*` values derived from these flags may be more
 * convenient.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_IO_SHIFT           5
#define GPIO_IO_MASK            (0x0F << GPIO_IO_SHIFT)
/** @endcond */

/**
 * Mask to isolate the `GPIO_IO_IN_ENABLED` and `GPIO_IO_OUT_ENABLED`
 * flags.
 */
#define GPIO_DIR_MASK           (0x03 << GPIO_IO_SHIFT)

/**
 * Mask to isolate the `GPIO_IO_INIT_LOW` and `GPIO_IO_INIT_HIGH`
 * flags.
 */
#define GPIO_IO_INIT_MASK       (1 << (GPIO_IO_SHIFT + 3))

/** GPIO pin input is connected (i.e. read should work). */
#define GPIO_IO_IN_ENABLED      (1 << GPIO_IO_SHIFT)

/** GPIO pin output is enabled (i.e. write should work). */
#define GPIO_IO_OUT_ENABLED     (1 << (GPIO_IO_SHIFT + 1))

/**
 * GPIO pin output is to be initialized simultaneous with
 * configuration.
 */
#define GPIO_IO_INIT_ENABLED    (1 << (GPIO_IO_SHIFT + 2))

/**
 * GPIO pin output is driven low at configuration.
 *
 * Flag is applied only if `GPIO_IO_INIT_ENABLED` is set.
 */
#define GPIO_IO_INIT_LOW        (0 << (GPIO_IO_SHIFT + 3))

/**
 * GPIO pin output is driven high at configuration.
 *
 * Flag is applied only if `GPIO_IO_INIT_ENABLED` is set.
 */
#define GPIO_IO_INIT_HIGH       (1 << (GPIO_IO_SHIFT + 3))

/** GPIO pin configured as input only. */
#define GPIO_DIR_IN             GPIO_IO_IN_ENABLED

/** GPIO pin configured as output with no change to output state. */
#define GPIO_DIR_OUT            GPIO_IO_OUT_ENABLED

/** GPIO pin configured as output low.
 *
 * Where configuration is not atomic the output value should be set
 * before the pin is configured to drive the output.
 */
#define GPIO_DIR_OUT_LOW        (GPIO_IO_OUT_ENABLED	\
				 | GPIO_IO_INIT_ENABLED	\
				 | GPIO_IO_INIT_LOW)

/** GPIO pin configured as output high.
 *
 * Where configuration is not atomic the output value should be set
 * before the pin is configured to drive the output.
 */
#define GPIO_DIR_OUT_HIGH       (GPIO_IO_OUT_ENABLED	\
				 | GPIO_IO_INIT_ENABLED	\
				 | GPIO_IO_INIT_HIGH)
/** @} */


/**
 * @name GPIO interrupt flags
 *
 * The `GPIO_INT_*` flags are used with `gpio_pin_configure` or
 * `gpio_port_configure`, to specify how input GPIO pins will trigger
 * interrupts.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_INT_SHIFT          9
#define GPIO_INT_MASK           (0x1F << GPIO_INT_SHIFT)
/** @endcond */

/** Mask to isolate the `GPIO_INT_LEVEL` and `GPIO_INT_EDGE` flags. */
#define GPIO_INT_TRIGGER_MASK   (0x01 << (GPIO_INT_SHIFT + 1))

/** Mask to isolate the `GPIO_INT_LOW` and `GPIO_INT_HIGH` flags. */
#define GPIO_INT_DIR_MASK       (0x03 << (GPIO_INT_SHIFT + 2))

/** GPIO pin causes interrupt.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_ENABLE (1 << GPIO_INT_SHIFT)

/** Trigger detection when signal is observed to be at a particular
 * level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.  Isolate
 * using `GPIO_INT_TRIGGER_MASK`.
 */
#define GPIO_INT_LEVEL  (0 << (GPIO_INT_SHIFT + 1))

/** Trigger detection when a signal is observed to change level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.  Isolate
 * using `GPIO_INT_TRIGGER_MASK`.
 */
#define GPIO_INT_EDGE   (1 << (GPIO_INT_SHIFT + 1))

/** Trigger detection when input state is (or transitions to) low.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.  Isolate
 * using `GPIO_INT_DIR_MASK`.
 *
 * This bit may be combined `GPIO_INT_HIGH` to sense both states
 * (generally with `GPIO_INT_EDGE`).
 */
#define GPIO_INT_LOW    (1 << (GPIO_INT_SHIFT + 2))

/** Trigger detection on input state is (or transitions to) high.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.  Isolate
 * using `GPIO_INT_DIR_MASK`.
 *
 * This bit may be combined `GPIO_INT_LOW` to sense both states
 * (generally with `GPIO_INT_EDGE`).
 */
#define GPIO_INT_HIGH   (1 << (GPIO_INT_SHIFT + 3))

/** Interrupt triggers on both rising and falling edge.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 *
 * For compatibility with old code this should be combined with
 * `GPIO_INT_EDGE`.
 *
 * For compatibility with new code this should be combined with both
 * `GPIO_INT_LOW` and `GPIO_INT_HIGH`.  Updated driver code should
 * prefer to check the individual flags.
 *
 * @deprecated Replace tests with a check that flags masked by
 * `GPIO_INT_DIR_MASK` is equal to `GPIO_INT_LOW | GPIO_INT_HIGH`.
 */
#define GPIO_INT_DOUBLE_EDGE    (1 << (GPIO_INT_SHIFT + 4))

/** GPIO pin trigger on rising edge. */
#define GPIO_INT_EDGE_RISING    (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_HIGH)

/** GPIO pin trigger on rising edge. */
#define GPIO_INT_EDGE_FALLING   (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_LOW)

/** GPIO pin trigger on rising or falling edge. */
#define GPIO_INT_EDGE_BOTH      (GPIO_INT_ENABLE \
				 | GPIO_INT_EDGE \
				 | GPIO_INT_LOW	 \
				 | GPIO_INT_HIGH \
				 | GPIO_INT_DOUBLE_EDGE)

/** GPIO pin trigger on level low. */
#define GPIO_INT_LEVEL_LOW      (GPIO_INT_ENABLE  \
				 | GPIO_INT_LEVEL \
				 | GPIO_INT_LOW)

/** GPIO pin trigger on level high. */
#define GPIO_INT_LEVEL_HIGH     (GPIO_INT_ENABLE  \
				 | GPIO_INT_LEVEL \
				 | GPIO_INT_HIGH)

/** @} */

/**
 * @name GPIO polarity flags
 *
 * The `GPIO_POL_*` flags are used with `gpio_pin_configure` or
 * `gpio_port_configure`, to specify the polarity of a GPIO pin.
 *
 * Note that not all GPIO drivers may support configuring pin
 * polarity, and the effect of such configuration may also be
 * driver-specific.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_SHIFT          14
#define GPIO_POL_MASK           (1 << GPIO_POL_SHIFT)
/** @endcond */

/** GPIO pin polarity is normal. */
#define GPIO_POL_NORMAL         (0 << GPIO_POL_SHIFT)

/** GPIO pin polarity is inverted. */
#define GPIO_POL_INV            (1 << GPIO_POL_SHIFT)

/** @} */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DEBOUNCE_SHIFT     15
#define GPIO_DEBOUNCE_MASK      (1 << GPIO_DEBOUNCE_SHIFT)
/** @endcond */

/** Enable GPIO pin debounce. */
#define GPIO_DEBOUNCE           (1 << GPIO_DEBOUNCE_SHIFT)

/**
 * @name GPIO drive strength flags
 *
 * The `GPIO_DS_*` flags are used with `gpio_pin_configure` or
 * `gpio_port_configure`, to specify the drive strength configuration
 * of a GPIO pin.
 *
 * The drive strength of individual pins can be configured
 * independently for when the pin output is low and high.
 *
 * The `GPIO_DS_*_LOW` bits define the drive strength of a pin
 * when output is low.
 *
 * The `GPIO_DS_*_HIGH` bits define the drive strength of a pin
 * when output is high.
 *
 * If the device does not distinguish drive strength by signal level,
 * the drive strength configuration from `GPIO_DS_*_LOW` shall be
 * used.
 *
 * The interface supports several drive strengths:
 * `LO` - The lowest drive strength supported by the HW
 * `HI` - The highest drive strength supported by the HW
 * `DISCONNECT` - The pin is placed in a high impedance state and not
 * driven (open drain mode)
 *
 * In addition `DFLT` represents the default drive strength for a
 * particular driver, and may be either `LO` or `HI`.
 *
 * On hardware that supports only one standard drive strength `LO` and
 * `HI` have the same behavior.
 *
 * On hardware that does not support a disconnect mode, `DISCONNECT`
 * will behave the same as `DFLT`.
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_LOW_SHIFT       16
#define GPIO_DS_LOW_MASK        (3 << GPIO_DS_LOW_SHIFT)
/** @endcond */

/** Use default drive strength when GPIO pin output is low. */
#define GPIO_DS_DFLT_LOW        (0 << GPIO_DS_LOW_SHIFT)

/** Use low/reduced drive strength when GPIO pin output is low. */
#define GPIO_DS_LO_LOW          (1 << GPIO_DS_LOW_SHIFT)

/** Use high/increased drive strength when GPIO pin output is low. */
#define GPIO_DS_HI_LOW          (2 << GPIO_DS_LOW_SHIFT)

/** Disconnect pin when GPIO pin output is low.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_LOW  (3 << GPIO_DS_LOW_SHIFT)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_HIGH_SHIFT      18
#define GPIO_DS_HIGH_MASK       (3 << GPIO_DS_HIGH_SHIFT)
/** @endcond */

/** Use default drive strength when GPIO pin output is low. */
#define GPIO_DS_DFLT_HIGH        (0 << GPIO_DS_HIGH_SHIFT)

/** Use low/reduced drive strength when GPIO pin output is low. */
#define GPIO_DS_LO_HIGH          (1 << GPIO_DS_HIGH_SHIFT)

/** Use high/increased drive strength when GPIO pin output is low. */
#define GPIO_DS_HI_HIGH          (2 << GPIO_DS_HIGH_SHIFT)

/** Disconnect pin when GPIO pin output is low.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_HIGH  (3 << GPIO_DS_HIGH_SHIFT)

/** @} */

/** @name Deprecated Flags
 *
 * These flags were defined at a time when the characteristic they
 * identified was assumed to be relevant only for interrupts.  The
 * features are also useful for non-interrupt inputs and in some cases
 * for outputs.
 *
 * @{
 */

/** Legacy flag setting indicating signal or interrupt active level.
 *
 * This flag was used both to indicate a signal's active level, and to
 * indicate the level associated with an interrupt on a signal.  As
 * active level is also relevant to output signals the two
 * intepretations have been separated.  The legacy value supports
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
 * intepretations have been separated.  The legacy value supports
 * testing for interrupt level as this is the most common use in
 * existing code.
 *
 * @deprecated Replace with `GPIO_ACTIVE_HIGH` or `GPIO_INT_HIGH`
 * depending on intent.
 */
#define GPIO_INT_ACTIVE_HIGH    GPIO_INT_HIGH

/** Legacy flag indicating a desire to debounce the signal.
 *
 * Although debouncing is useful for interrupts it is not specific to
 * interrupts.
 *
 * @deprecated Replace with `GPIO_DEBOUNCE`
 */
#define GPIO_INT_DEBOUNCE       GPIO_DEBOUNCE

/** Legacy alias indicating that interrupts should be enabled on the
 * GPIO.
 *
 * @deprecated Replace with `GPIO_INT_ENABLE`
 */
#define GPIO_INT                GPIO_INT_ENABLE

/** Legacy alias identifying high drive strength when GPIO output is
 * low.
 *
 * @deprecated Replace with `GPIO_DS_HI_LOW`
 */
#define GPIO_DS_ALT_LOW         GPIO_DS_HI_LOW

/** Legacy alias identifying high drive strength when GPIO output is
 * high.
 *
 * @deprecated Replace with `GPIO_DS_HI_HIGH`
 */
#define GPIO_DS_ALT_HIGH        GPIO_DS_HI_HIGH

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_ */
