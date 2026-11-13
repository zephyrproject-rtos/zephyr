/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_PUYA_PY32_PINCTRL_SOC_H_
#define ZEPHYR_SOC_PUYA_PY32_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/puya_py32_pinctrl.h>
#include <zephyr/types.h>

/* Pinmux uses the established port/line/function encoding. */
struct py32_pinctrl_pin {
	uint32_t pinmux;
	uint8_t pull;
	uint8_t speed;
	bool open_drain;
	bool output_low;
	bool output_high;
};

typedef struct py32_pinctrl_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pinmux = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux),                     \
		.pull = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_up) +                \
			2U * DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_down),          \
		.speed = DT_ENUM_IDX(DT_PROP_BY_IDX(node_id, prop, idx), slew_rate),               \
		.open_drain = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), drive_open_drain),       \
		.output_low = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), output_low),             \
		.output_high = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), output_high),           \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_PUYA_PY32_PINCTRL_SOC_H_ */
