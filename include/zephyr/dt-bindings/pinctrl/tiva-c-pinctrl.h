/**
 * @file
 * @brief Tiva C pinctrl encoding definitions.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Linumiz
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TIVA_C_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TIVA_C_PINCTRL_H_

/*
 * Tiva C pinmux encoding (32-bit):
 *   Bits [15:0]  : port details
 *   Bits [31:16] : pin details
 */

/** Bit shift for the GPIO port field. */
#define TIVA_C_PORT_SHIFT  0
/** Bit mask for the GPIO port field. */
#define TIVA_C_PORT_MASK   0x7
/** Bit shift for the GPIO pin field. */
#define TIVA_C_PIN_SHIFT   16
/** Bit mask for the GPIO pin field. */
#define TIVA_C_PIN_MASK    0x7
/** Bit shift for the mux function field. */
#define TIVA_C_MUX_SHIFT   19
/** Bit mask for the mux function field. */
#define TIVA_C_MUX_MASK    0xF
/** Bit shift for the pin type field. */
#define TIVA_C_TYPE_SHIFT  23
/** Bit mask for the pin type field. */
#define TIVA_C_TYPE_MASK   0x7

/** GPIO Port A identifier. */
#define TIVA_C_PORT_A 0
/** GPIO Port B identifier. */
#define TIVA_C_PORT_B 1
/** GPIO Port C identifier. */
#define TIVA_C_PORT_C 2
/** GPIO Port D identifier. */
#define TIVA_C_PORT_D 3
/** GPIO Port E identifier. */
#define TIVA_C_PORT_E 4
/** GPIO Port F identifier. */
#define TIVA_C_PORT_F 5

/** GPIO pin type. */
#define TIVA_C_TYPE_GPIO 0
/** UART pin type. */
#define TIVA_C_TYPE_UART 1
/** I2C SDA pin type. */
#define TIVA_C_TYPE_I2C 2
/** I2C SCL pin type. */
#define TIVA_C_TYPE_I2C_SCL 3
/** SSI pin type. */
#define TIVA_C_TYPE_SSI 4
/** CAN pin type. */
#define TIVA_C_TYPE_CAN 5
/** PWM pin type. */
#define TIVA_C_TYPE_PWM 6

/**
 * @brief Encode a 32-bit Tiva C pinmux value.
 *
 * @param port GPIO port identifier.
 * @param pin GPIO pin number.
 * @param mux Alternate function number.
 * @param type Peripheral type.
 *
 * @return Encoded pinmux value.
 */
#define TIVA_C_PINMUX(port, pin, mux, type) \
	((((port) & TIVA_C_PORT_MASK) << TIVA_C_PORT_SHIFT) | \
	 (((pin) & TIVA_C_PIN_MASK) << TIVA_C_PIN_SHIFT) | \
	 (((mux) & TIVA_C_MUX_MASK) << TIVA_C_MUX_SHIFT) | \
	 (((type) & TIVA_C_TYPE_MASK) << TIVA_C_TYPE_SHIFT))

/** Extract GPIO port from an encoded pinmux value. */
#define TIVA_C_PINMUX_PORT(pm) \
	(((pm) >> TIVA_C_PORT_SHIFT) & TIVA_C_PORT_MASK)
/** Extract GPIO pin from an encoded pinmux value. */
#define TIVA_C_PINMUX_PIN(pm) \
	(((pm) >> TIVA_C_PIN_SHIFT) & TIVA_C_PIN_MASK)
/** Extract mux function from an encoded pinmux value. */
#define TIVA_C_PINMUX_MUX(pm) \
	(((pm) >> TIVA_C_MUX_SHIFT) & TIVA_C_MUX_MASK)
/** Extract peripheral type from an encoded pinmux value. */
#define TIVA_C_PINMUX_TYPE(pm) \
	(((pm) >> TIVA_C_TYPE_SHIFT) & TIVA_C_TYPE_MASK)

/** Bit position of the pull-up flag. */
#define TIVA_C_PULL_UP_SHIFT 26
/** Bit position of the pull-down flag. */
#define TIVA_C_PULL_DOWN_SHIFT 27
/** Bit position of the open-drain flag. */
#define TIVA_C_OPEN_DRAIN_SHIFT 28

/** Return the encoded pull-up flag. */
#define TIVA_C_PINMUX_PULL_UP(pm) \
	(((pm) >> TIVA_C_PULL_UP_SHIFT) & 0x1U)
/** Return the encoded pull-down flag. */
#define TIVA_C_PINMUX_PULL_DOWN(pm) \
	(((pm) >> TIVA_C_PULL_DOWN_SHIFT) & 0x1U)
/** Return the encoded open-drain flag. */
#define TIVA_C_PINMUX_OPEN_DRAIN(pm) \
	(((pm) >> TIVA_C_OPEN_DRAIN_SHIFT) & 0x1U)

/** Bit shift for pin configuration port field. */
#define TIVA_C_PINCFG_PORT_SHIFT 16
/** Bit shift for pin configuration pin field. */
#define TIVA_C_PINCFG_PIN_SHIFT 10

#endif
