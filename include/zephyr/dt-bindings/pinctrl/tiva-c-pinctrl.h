/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Linumiz
 * Author: Sri Surya <srisurya@linumiz.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TIVA_C_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TIVA_C_PINCTRL_H_
/*
 * Tiva C pinmux encoding (32-bit):
 *
 *   Bits [15:0]  (lower 16) — port details:
 *     [2:0]   port index  (0=A … 5=F)
 *
 *   Bits [31:16] (upper 16) — pin details:
 *     [18:16] pin number   (0-7)
 *     [22:19] mux function (alternate-function 0-15)
 *     [25:23] pin type     (peripheral type, see below)
 */
/**
 * @name Tiva C Pinctrl Shift and Mask Definitions
 * @{
 */
#define TIVA_C_PORT_SHIFT  0
#define TIVA_C_PORT_MASK   0x7
#define TIVA_C_PIN_SHIFT   16
#define TIVA_C_PIN_MASK    0x7
#define TIVA_C_MUX_SHIFT   19
#define TIVA_C_MUX_MASK    0xF
#define TIVA_C_TYPE_SHIFT  23
#define TIVA_C_TYPE_MASK   0x7
/** @} */

/**
 * @name Tiva C GPIO Port Definitions
 * @{
 */
#define TIVA_C_PORT_A  0
#define TIVA_C_PORT_B  1
#define TIVA_C_PORT_C  2
#define TIVA_C_PORT_D  3
#define TIVA_C_PORT_E  4
#define TIVA_C_PORT_F  5
/** @} */

/**
 * @name Tiva C Peripheral Type Muxes
 * @{
 */
#define TIVA_C_TYPE_GPIO     0
#define TIVA_C_TYPE_UART     1
#define TIVA_C_TYPE_I2C      2
#define TIVA_C_TYPE_I2C_SCL  3
#define TIVA_C_TYPE_SSI      4
#define TIVA_C_TYPE_CAN      5
#define TIVA_C_TYPE_PWM      6
/** @} */

/*
 * Build a 32-bit pinmux value from port, pin, mux function, and type.
 *
 * Example:  TIVA_C_PINMUX(TIVA_C_PORT_A, 0, 1, TIVA_C_TYPE_UART)
 */

/**
 * @name Tiva C Pinmux Extraction Macros
 * @{
 */
#define TIVA_C_PINMUX(port, pin, mux, type) \
	(((port) & TIVA_C_PORT_MASK) << TIVA_C_PORT_SHIFT | \
	 ((pin)  & TIVA_C_PIN_MASK)  << TIVA_C_PIN_SHIFT  | \
	 ((mux)  & TIVA_C_MUX_MASK)  << TIVA_C_MUX_SHIFT  | \
	 ((type) & TIVA_C_TYPE_MASK) << TIVA_C_TYPE_SHIFT)

#define TIVA_C_PINMUX_PORT(pm) (((pm) >> TIVA_C_PORT_SHIFT) & TIVA_C_PORT_MASK)
#define TIVA_C_PINMUX_PIN(pm)  (((pm) >> TIVA_C_PIN_SHIFT)  & TIVA_C_PIN_MASK)
#define TIVA_C_PINMUX_MUX(pm)  (((pm) >> TIVA_C_MUX_SHIFT)  & TIVA_C_MUX_MASK)
#define TIVA_C_PINMUX_TYPE(pm) (((pm) >> TIVA_C_TYPE_SHIFT) & TIVA_C_TYPE_MASK)
/** @} */

/**
 * @name Tiva C Pin Configuration Bitfield Shifts
 * @{
 */
#define TIVA_C_PINCFG_PORT_SHIFT  16
#define TIVA_C_PINCFG_PIN_SHIFT   10
/** @} */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TIVA_C_PINCTRL_H_ */
