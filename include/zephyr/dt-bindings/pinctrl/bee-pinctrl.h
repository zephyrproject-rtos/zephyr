/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEE_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEE_PINCTRL_H_

/**
 * @file
 * @brief Realtek BEE Pinctrl Devicetree Bindings
 */

/** Position of the function field. */
#define BEE_FUN_POS 16U
/** Mask for the function field. */
#define BEE_FUN_MSK 0xFFFFU
/** Position of the pin field. */
#define BEE_PIN_POS 0U
/** Mask for the pin field. */
#define BEE_PIN_MSK 0x7FFU

/**
 * @brief Utility macro to construct a pinctrl configuration field.
 *
 * This macro generates the encoded value required by the pinctrl driver
 * to configure a specific pin function.
 *
 * @param fun Pin function name (without the `BEE_` prefix).
 *            The macro automatically prepends `BEE_` to this argument.
 * @param pin Pad number/ID.
 */
#define BEE_PSEL(fun, pin)                                                                         \
	(((((pin) & BEE_PIN_MSK) << BEE_PIN_POS) | (((BEE_##fun) & BEE_FUN_MSK) << BEE_FUN_POS)))

/**
 * @brief Utility macro to configure a function without a physical pin connection.
 *
 * @param fun Pin function name (without the `BEE_` prefix).
 *            The macro automatically prepends `BEE_` to this argument.
 */
#define BEE_PSEL_DISCONNECTED(fun)                                                                 \
	(BEE_PIN_DISCONNECTED << BEE_PIN_POS | ((BEE_##fun & BEE_FUN_MSK) << BEE_FUN_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BEE_PINCTRL_H_ */
