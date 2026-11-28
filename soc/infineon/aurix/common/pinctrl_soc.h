/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_INFINEON_AURIX_COMMON_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/infineon-aurix-pinctrl.h>

/** @cond INTERNAL_HIDDEN */

typedef struct pinctrl_soc_pin {
	uint32_t pinmux;
	uint32_t output: 1;
	uint32_t open_drain: 1;
	uint32_t pull_up: 1;
	uint32_t pull_down: 1;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_AURIX_PIN_INIT(node_id)                                                          \
	{                                                                                          \
		.pinmux = DT_PROP(node_id, pinmux),                                                \
		.output = DT_PROP(node_id, output_enable),                                         \
		.open_drain = DT_PROP(node_id, drive_open_drain),                                  \
		.pull_up = DT_PROP(node_id, bias_pull_up),                                         \
		.pull_down = DT_PROP(node_id, bias_pull_down),                                     \
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	Z_PINCTRL_AURIX_PIN_INIT(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#endif /* ZEPHYR_SOC_INFINEON_AURIX_COMMON_PINCTRL_SOC_H_ */
