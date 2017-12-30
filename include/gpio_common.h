/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2018 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GPIO_COMMON_H_
#define _GPIO_COMMON_H_

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

/** GPIO pin trigger to be synchronized to clock pulses. */
#define GPIO_INT_CLOCK_SYNC     (1 << 3)

/** Enable GPIO pin debounce. */
#define GPIO_INT_DEBOUNCE       (1 << 4)

/** Do Level trigger. */
#define GPIO_INT_LEVEL		(0 << 5)

/** Do Edge trigger. */
#define GPIO_INT_EDGE		(1 << 5)

/** Interrupt triggers on both rising and falling edge. */
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

/**
 * @brief GPIO Pin Defines
 * @defgroup gpio_interface_pin_defs GPIO Pin Defines
 * @ingroup gpio_interface
 *
 * @note Not to be used with the legacy GPIO device driver.
 * @{
 */

/**
 * @def GPIO_PORT_PIN
 * @brief Helper to create pin number from pin index.
 *
 * @param idx Index of pin within port [0..31]
 * @return Pin number
 */
#define GPIO_PORT_PIN(idx)  (1U << (idx))

/**
 * @def GPIO_PORT_PIN_IDX
 * @brief Helper to create pin index from pin number.
 *
 * Pin number shall be one of GPIO_PORT_PINx (only one bit set).
 *
 * @note For runtime calculation use gpio_port_pin_idx() instead.
 *
 * @param pin Pin number
 * @return Index of pin within port [0..31]
 */
#define GPIO_PORT_PIN_IDX(pin) ( \
	(((pin & GPIO_PORT_PIN(0))  >>  0) *  0) + \
	(((pin & GPIO_PORT_PIN(1))  >>  1) *  1) + \
	(((pin & GPIO_PORT_PIN(2))  >>  2) *  2) + \
	(((pin & GPIO_PORT_PIN(3))  >>  3) *  3) + \
	(((pin & GPIO_PORT_PIN(4))  >>  4) *  4) + \
	(((pin & GPIO_PORT_PIN(5))  >>  5) *  5) + \
	(((pin & GPIO_PORT_PIN(6))  >>  6) *  6) + \
	(((pin & GPIO_PORT_PIN(7))  >>  7) *  7) + \
	(((pin & GPIO_PORT_PIN(8))  >>  8) *  8) + \
	(((pin & GPIO_PORT_PIN(9))  >>  9) *  9) + \
	(((pin & GPIO_PORT_PIN(10)) >> 10) * 10) + \
	(((pin & GPIO_PORT_PIN(11)) >> 11) * 11) + \
	(((pin & GPIO_PORT_PIN(12)) >> 12) * 12) + \
	(((pin & GPIO_PORT_PIN(13)) >> 13) * 13) + \
	(((pin & GPIO_PORT_PIN(14)) >> 14) * 14) + \
	(((pin & GPIO_PORT_PIN(15)) >> 15) * 15) + \
	(((pin & GPIO_PORT_PIN(16)) >> 16) * 16) + \
	(((pin & GPIO_PORT_PIN(17)) >> 17) * 17) + \
	(((pin & GPIO_PORT_PIN(18)) >> 18) * 18) + \
	(((pin & GPIO_PORT_PIN(19)) >> 19) * 19) + \
	(((pin & GPIO_PORT_PIN(20)) >> 20) * 20) + \
	(((pin & GPIO_PORT_PIN(21)) >> 21) * 21) + \
	(((pin & GPIO_PORT_PIN(22)) >> 22) * 22) + \
	(((pin & GPIO_PORT_PIN(23)) >> 23) * 23) + \
	(((pin & GPIO_PORT_PIN(24)) >> 24) * 24) + \
	(((pin & GPIO_PORT_PIN(25)) >> 25) * 25) + \
	(((pin & GPIO_PORT_PIN(26)) >> 26) * 26) + \
	(((pin & GPIO_PORT_PIN(27)) >> 27) * 27) + \
	(((pin & GPIO_PORT_PIN(28)) >> 28) * 28) + \
	(((pin & GPIO_PORT_PIN(29)) >> 29) * 29) + \
	(((pin & GPIO_PORT_PIN(30)) >> 30) * 30) + \
	(((pin & GPIO_PORT_PIN(31)) >> 31) * 31))

/**
 * @def GPIO_PORT_PIN0
 * @brief Pin 0 of the GPIO port.
 */
#define GPIO_PORT_PIN0		GPIO_PORT_PIN(0)

/**
 * @def GPIO_PORT_PIN1
 * @brief Pin 1 of the GPIO port.
 */
#define GPIO_PORT_PIN1		GPIO_PORT_PIN(1)

/**
 * @def GPIO_PORT_PIN2
 * @brief Pin 2 of the GPIO port.
 */
#define GPIO_PORT_PIN2		GPIO_PORT_PIN(2)

/**
 * @def GPIO_PORT_PIN3
 * @brief Pin 3 of the GPIO port.
 */
#define GPIO_PORT_PIN3		GPIO_PORT_PIN(3)

/**
 * @def GPIO_PORT_PIN4
 * @brief Pin 4 of the GPIO port.
 */
#define GPIO_PORT_PIN4		GPIO_PORT_PIN(4)

/**
 * @def GPIO_PORT_PIN5
 * @brief Pin 5 of the GPIO port.
 */
#define GPIO_PORT_PIN5		GPIO_PORT_PIN(5)

/**
 * @def GPIO_PORT_PIN6
 * @brief Pin 6 of the GPIO port.
 */
#define GPIO_PORT_PIN6		GPIO_PORT_PIN(6)

/**
 * @def GPIO_PORT_PIN7
 * @brief Pin 7 of the GPIO port.
 */
#define GPIO_PORT_PIN7		GPIO_PORT_PIN(7)

/**
 * @def GPIO_PORT_PIN8
 * @brief Pin 8 of the GPIO port.
 */
#define GPIO_PORT_PIN8		GPIO_PORT_PIN(8)

/**
 * @def GPIO_PORT_PIN9
 * @brief Pin 9 of the GPIO port.
 */
#define GPIO_PORT_PIN9		GPIO_PORT_PIN(9)

/**
 * @def GPIO_PORT_PIN10
 * @brief Pin 10 of the GPIO port.
 */
#define GPIO_PORT_PIN10		GPIO_PORT_PIN(10)

/**
 * @def GPIO_PORT_PIN11
 * @brief Pin 11 of the GPIO port.
 */
#define GPIO_PORT_PIN11		GPIO_PORT_PIN(11)

/**
 * @def GPIO_PORT_PIN12
 * @brief Pin 12 of the GPIO port.
 */
#define GPIO_PORT_PIN12		GPIO_PORT_PIN(12)

/**
 * @def GPIO_PORT_PIN13
 * @brief Pin 13 of the GPIO port.
 */
#define GPIO_PORT_PIN13		GPIO_PORT_PIN(13)

/**
 * @def GPIO_PORT_PIN14
 * @brief Pin 14 of the GPIO port.
 */
#define GPIO_PORT_PIN14		GPIO_PORT_PIN(14)

/**
 * @def GPIO_PORT_PIN15
 * @brief Pin 15 of the GPIO port.
 */
#define GPIO_PORT_PIN15		GPIO_PORT_PIN(15)

/**
 * @def GPIO_PORT_PIN16
 * @brief Pin 16 of the GPIO port.
 */
#define GPIO_PORT_PIN16		GPIO_PORT_PIN(16)

/**
 * @def GPIO_PORT_PIN17
 * @brief Pin 17 of the GPIO port.
 */
#define GPIO_PORT_PIN17		GPIO_PORT_PIN(17)

/**
 * @def GPIO_PORT_PIN18
 * @brief Pin 18 of the GPIO port.
 */
#define GPIO_PORT_PIN18		GPIO_PORT_PIN(18)

/**
 * @def GPIO_PORT_PIN19
 * @brief Pin 19 of the GPIO port.
 */
#define GPIO_PORT_PIN19		GPIO_PORT_PIN(19)

/**
 * @def GPIO_PORT_PIN20
 * @brief Pin 20 of the GPIO port.
 */
#define GPIO_PORT_PIN20		GPIO_PORT_PIN(20)

/**
 * @def GPIO_PORT_PIN21
 * @brief Pin 21 of the GPIO port.
 */
#define GPIO_PORT_PIN21		GPIO_PORT_PIN(21)

/**
 * @def GPIO_PORT_PIN22
 * @brief Pin 22 of the GPIO port.
 */
#define GPIO_PORT_PIN22		GPIO_PORT_PIN(22)

/**
 * @def GPIO_PORT_PIN23
 * @brief Pin 23 of the GPIO port.
 */
#define GPIO_PORT_PIN23		GPIO_PORT_PIN(23)

/**
 * @def GPIO_PORT_PIN24
 * @brief Pin 24 of the GPIO port.
 */
#define GPIO_PORT_PIN24		GPIO_PORT_PIN(24)

/**
 * @def GPIO_PORT_PIN25
 * @brief Pin 25 of the GPIO port.
 */
#define GPIO_PORT_PIN25		GPIO_PORT_PIN(25)

/**
 * @def GPIO_PORT_PIN26
 * @brief Pin 26 of the GPIO port.
 */
#define GPIO_PORT_PIN26		GPIO_PORT_PIN(26)

/**
 * @def GPIO_PORT_PIN27
 * @brief Pin 27 of the GPIO port.
 */
#define GPIO_PORT_PIN27		GPIO_PORT_PIN(27)

/**
 * @def GPIO_PORT_PIN28
 * @brief Pin 28 of the GPIO port.
 */
#define GPIO_PORT_PIN28		GPIO_PORT_PIN(28)

/**
 * @def GPIO_PORT_PIN29
 * @brief Pin 29 of the GPIO port.
 */
#define GPIO_PORT_PIN29		GPIO_PORT_PIN(29)

/**
 * @def GPIO_PORT_PIN30
 * @brief Pin 30 of the GPIO port.
 */
#define GPIO_PORT_PIN30		GPIO_PORT_PIN(30)

/**
 * @def GPIO_PORT_PIN31
 * @brief Pin 31 of the GPIO port.
 */
#define GPIO_PORT_PIN31		GPIO_PORT_PIN(31)


/** @} gpio_interface_pin_defs */

#endif /* _GPIO_COMMON_H_ */
