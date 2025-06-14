/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_REALTEK_RTS5817_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_REALTEK_RTS5817_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/rts5817_pinctrl.h>

typedef struct pinctrl_soc_pin {
	uint8_t pin: 8;
	uint8_t func: 3;
	uint8_t pulldown: 1;
	uint8_t pullup: 1;
	uint8_t power_source: 2;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pin = RTS_FP_PINMUX_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),                      \
		.func = RTS_FP_PINMUX_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                    \
		.pulldown = DT_PROP(node_id, bias_pull_down),                                      \
		.pullup = DT_PROP(node_id, bias_pull_up),                                          \
		.power_source = (DT_PROP_OR(node_id, power_source, IO_POWER_3V3)),                 \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_ARM_REALTEK_RTS5817_PINCTRL_SOC_H_ */
