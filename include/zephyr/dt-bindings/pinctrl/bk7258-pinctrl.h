/**
 * @file bk7258-pinctrl.h
 * @brief Beken BK7258 pin controller devicetree binding constants.
 *
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_

/** @brief GPIO pin number bitmask (bits 0-5) */
#define BK7258_GPIO_PIN_MASK 0x3fU

/** @brief Pin function select bitmask (bits 0-3 of function field) */
#define BK7258_FUNC_MASK     0x0fU

/** @brief Bit position of the function field in pinmux value */
#define BK7258_FUNC_POS      8

/**
 * @brief Encode a pinmux value from pin and function identifiers.
 * @param pin GPIO pin number (0-63).
 * @param func Pin function identifier.
 * @return Encoded pinmux value.
 */
#define BK7258_PINMUX(pin, func)                                                                   \
	(((pin) & BK7258_GPIO_PIN_MASK) | (((func) & BK7258_FUNC_MASK) << BK7258_FUNC_POS))

/** @brief Configure pin as output */
#define BK7258_FLAG_OUTPUT      0x00001000

/** @brief Configure pin as input */
#define BK7258_FLAG_INPUT       0x00002000

/** @brief Enable internal pull-up resistor */
#define BK7258_FLAG_PULL_UP     0x00004000

/** @brief Enable internal pull-down resistor */
#define BK7258_FLAG_PULL_DOWN   0x00008000

/** @brief Select alternate (secondary) function for pin */
#define BK7258_FLAG_SECOND_FUNC 0x00010000

/** @brief UART1 transmit function */
#define FUNC_UART1_TX 0

/** @brief UART1 receive function */
#define FUNC_UART1_RX 0

/** @brief GPIO pin 0 */
#define BK7258_PIN_0 0

/** @brief GPIO pin 1 */
#define BK7258_PIN_1 1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_ */
