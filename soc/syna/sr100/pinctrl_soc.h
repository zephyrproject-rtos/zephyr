/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYNAPTICS_SRXXX_PINCTRL_SOC_H_
#define SYNAPTICS_SRXXX_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pin {
	uint32_t pinmux;
	uint32_t pincfg;
	uint32_t flags;
	uint32_t port;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define SRXXX_DRV_STRENGTH_SHIFT	0
#define SRXXX_HOLD_ENABLE_SHIFT		2
#define SRXXX_INPUT_ENABLE_SHIFT	3
#define SRXXX_PULL_ENABLE_SHIFT		4
#define SRXXX_PULL_UP_SHIFT		5
#define SRXXX_SLEW_RATE_SHIFT		6
#define SRXXX_SCHMITT_TRIG_SHIFT	7

#define SRXXX_DRV_STRENGTH_MASK		(0x3 << SRXXX_DRV_STRENGTH_SHIFT)
#define SRXXX_HOLD_ENABLE_MASK		(0x1 << SRXXX_HOLD_ENABLE_SHIFT)
#define SRXXX_INPUT_ENABLE_MASK		(0x1 << SRXXX_INPUT_ENABLE_SHIFT)
#define SRXXX_PULL_ENABLE_MASK		(0x3 << SRXXX_PULL_ENABLE_SHIFT)
#define SRXXX_SLEW_RATE_MASK		(0x1 << SRXXX_SLEW_RATE_SHIFT)
#define SRXXX_SCHMITT_TRIG_MASK		(0x1 << SRXXX_SCHMITT_TRIG_SHIFT)

#define SRXXX_DT_PINCFG_FLAGS(node)                                                  \
	((DT_NODE_HAS_PROP(node, slew_rate) << SRXXX_SLEW_RATE_SHIFT) |              \
	 (DT_NODE_HAS_PROP(node, bias_bus_hold) << SRXXX_HOLD_ENABLE_SHIFT) |        \
	 (DT_NODE_HAS_PROP(node, input_disable) << SRXXX_INPUT_ENABLE_SHIFT) |       \
	 (DT_PROP_OR(node, bias_enable, 0) << SRXXX_PULL_ENABLE_SHIFT) |             \
	 (DT_PROP_OR(node, bias_pull_up, 0) << SRXXX_PULL_ENABLE_SHIFT) |            \
	 (DT_PROP_OR(node, bias_pull_down, 0) << SRXXX_PULL_ENABLE_SHIFT) |          \
	 (DT_NODE_HAS_PROP(node, drive_strength) << SRXXX_DRV_STRENGTH_SHIFT) |      \
	 (DT_NODE_HAS_PROP(node, input_schmitt_enable) << SRXXX_SCHMITT_TRIG_SHIFT))

#define SRXXX_DT_PINCFG_INIT(node)                                                   \
	((DT_PROP_OR(node, slew_rate, 0) << SRXXX_SLEW_RATE_SHIFT) |                 \
	 (DT_PROP_OR(node, bias_bus_hold, 0) << SRXXX_HOLD_ENABLE_SHIFT) |           \
	 (!DT_PROP_OR(node, input_disable, 0) << SRXXX_INPUT_ENABLE_SHIFT) |         \
	 (DT_PROP_OR(node, bias_pull_up, 0) << SRXXX_PULL_UP_SHIFT) |                \
	 (DT_PROP_OR(node, bias_pull_up, 0) << SRXXX_PULL_ENABLE_SHIFT) |            \
	 (DT_PROP_OR(node, bias_pull_down, 0) << SRXXX_PULL_ENABLE_SHIFT) |          \
	 (DT_PROP_OR(node, drive_strength, 0) << SRXXX_DRV_STRENGTH_SHIFT) |         \
	 (DT_PROP_OR(node, input_schmitt_enable, 0) << SRXXX_SCHMITT_TRIG_SHIFT))

#define SRXXX_DT_PINMUX_INIT(node_id)	DT_PROP(node_id, pinmux)
#define SRXXX_DT_PORT_INIT(node_id)	DT_PROP_OR(node_id, port, 0)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                 \
	{                                                                            \
		.pinmux = SRXXX_DT_PINMUX_INIT(DT_PROP_BY_IDX(node_id, prop, idx)),  \
		.pincfg = SRXXX_DT_PINCFG_INIT(DT_PROP_BY_IDX(node_id, prop, idx)),  \
		.flags = SRXXX_DT_PINCFG_FLAGS(DT_PROP_BY_IDX(node_id, prop, idx)),  \
		.port = SRXXX_DT_PORT_INIT(DT_PROP_BY_IDX(node_id, prop, idx)),      \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                     \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#ifdef __cplusplus
}
#endif

#endif /* SYNAPTICS_SRXXX_PINCTRL_SOC_H_ */
