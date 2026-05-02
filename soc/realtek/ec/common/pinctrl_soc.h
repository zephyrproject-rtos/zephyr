/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

/**
 * @file
 * realtek SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_REALTEK_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_REALTEK_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/realtek-rts5912-pinctrl.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_REALTEK_RTS5912_PINMUX_INIT(node_id) (uint32_t)(DT_PROP(node_id, pinmux))

#define Z_PINCTRL_STATE_PINCFG_INIT(node_id)                                                       \
	((DT_PROP(node_id, bias_pull_down) << REALTEK_RTS5912_PD_POS) |                            \
	 (DT_PROP(node_id, bias_pull_up) << REALTEK_RTS5912_PU_POS) |                              \
	 (DT_PROP(node_id, drive_open_drain) << REALTEK_RTS5912_TYPE_POS) |                        \
	 (DT_PROP(node_id, output_high) << REALTEK_RTS5912_HIGH_LOW_POS) |                         \
	 (DT_PROP(node_id, output_enable) << REALTEK_RTS5912_INPUT_OUTPUT_POS) |                   \
	 (DT_PROP(node_id, input_schmitt_enable) << REALTEK_RTS5912_SCHMITTER_POS) |               \
	 (DT_PROP(node_id, input_enable) << REALTEK_RTS5912_INPUT_DETECTION_POS) |                 \
	 (DT_ENUM_IDX_OR(node_id, slew_rate, 0x0) << REALTEK_RTS5912_SLEW_RATE_POS) |              \
	 (DT_ENUM_IDX_OR(node_id, drive_strength, 0x0) << REALTEK_RTS5912_DRV_STR_POS))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	(Z_PINCTRL_REALTEK_RTS5912_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx)) |         \
	 Z_PINCTRL_STATE_PINCFG_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_REALTEK_COMMON_PINCTRL_SOC_H_ */
