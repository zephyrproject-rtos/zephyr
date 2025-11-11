/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Bouffalo Lab SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_BFLB_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_BFLB_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>

/* clang-format off */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief BFLB pincfg bit field.
 * @anchor BFLB_PINMUX
 *
 * Fields:
 *
 * - 24..31: pin
 * - 20..23: signal
 * - 18..19: mode
 * - 16..17: instance
 * -  8..15: function
 * -      7: reserved
 * -      6: GPIO Output Enable
 * -      5: Pull Down
 * -      4: Pull Up
 * -   2..3: Driver Strength
 * -      1: Schmitt trigger (SMT)
 * -      0: reserved
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)						\
	((DT_PROP_BY_IDX(node_id, prop, idx))							\
	 | (DT_PROP(node_id, bias_pull_up)              << BFLB_PINMUX_PULL_UP_POS)		\
	 | (DT_PROP(node_id, bias_pull_down)            << BFLB_PINMUX_PULL_DOWN_POS)		\
	 | (DT_PROP(node_id, output_enable)             << BFLB_PINMUX_OE_POS)			\
	 | (DT_PROP(node_id, input_schmitt_enable)      << BFLB_PINMUX_SMT_POS)			\
	 | (DT_ENUM_IDX(node_id, drive_strength)        << BFLB_PINMUX_DRIVER_STRENGTH_POS)	\
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM, pinmux,	               \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

/* clang-format on */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_BFLB_COMMON_H_ */
