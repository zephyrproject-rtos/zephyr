/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: This pin controller configuration header supports WCH CH582/CH582M SoCs only.
 */

/**
 * @file
 * @brief Pinctrl definitions for WCH CH582/CH582M SoCs only
 *
 * Unlike the CH570/CH572 family, CH582/CH582M use two GPIO ports (PA and PB)
 * and select each peripheral function with a single enable bit in
 * R32_PIN_CONFIG/R32_PIN_CONFIG2 rather than a multi-bit remap field, so no
 * alternate function width/offset/value needs to be encoded here.
 *
 * The pinmux encoding uses a 6-bit value:
 * - Bits [0:4] - Pin number within the port (0-23)
 * - Bit  [5]   - GPIO port: 0 = port A, 1 = port B
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH582M_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH582M_PINCTRL_H_

/** Position of the pin number field in the pinmux encoded value */
#define CH582M_PINCTRL_PIN_SHIFT 0
/** Mask for the pin number field in the pinmux encoded value */
#define CH582M_PINCTRL_PIN_MASK  GENMASK(4, 0)

/** Position of the GPIO port field in the pinmux encoded value */
#define CH582M_PINCTRL_PORT_SHIFT 5
/** Mask for the GPIO port field in the pinmux encoded value */
#define CH582M_PINCTRL_PORT_MASK  GENMASK(5, 5)

/** GPIO port A */
#define CH582M_PINCTRL_PORT_A 0
/** GPIO port B */
#define CH582M_PINCTRL_PORT_B 1

/**
 * @brief Encode pinmux configuration
 *
 * @param port GPIO port (CH582M_PINCTRL_PORT_A or CH582M_PINCTRL_PORT_B)
 * @param pin Pin number within the port
 * @return Encoded pinmux value for use in devicetree
 */
#define CH582M_PINMUX_DEFINE(port, pin)                                                            \
	(((pin) << CH582M_PINCTRL_PIN_SHIFT) | ((port) << CH582M_PINCTRL_PORT_SHIFT))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_CH582M_PINCTRL_H_ */
