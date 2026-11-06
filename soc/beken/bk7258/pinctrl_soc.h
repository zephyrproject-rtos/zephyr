/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_BEKEN_BK7258_PINCTRL_SOC_H_
#define _SOC_BEKEN_BK7258_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/beken-bk7258-pinctrl.h>
#include <zephyr/types.h>

/** @cond INTERNAL_HIDDEN */

struct bk7258_pinctrl_soc_pin {
	/** Pad number, 0 to 55 */
	uint32_t pin: 6;
	/** Index of the function the pad is switched to */
	uint32_t func: 4;
	/** Enable the pull-up resistor */
	uint32_t pull_up: 1;
	/** Enable the pull-down resistor */
	uint32_t pull_down: 1;
};

typedef struct bk7258_pinctrl_soc_pin pinctrl_soc_pin_t;

/* Each element of a pinctrl-N property is a phandle to one pad's node */
#define Z_PINCTRL_BK7258_PIN_INIT(pin_node)                                                        \
	{                                                                                          \
		.pin = (DT_PROP(pin_node, pinmux) >> BK7258_PIN_SHIFT) & BK7258_PIN_MASK,          \
		.func = (DT_PROP(pin_node, pinmux) >> BK7258_FUNC_SHIFT) & BK7258_FUNC_MASK,       \
		.pull_up = DT_PROP(pin_node, bias_pull_up),                                        \
		.pull_down = DT_PROP(pin_node, bias_pull_down),                                    \
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	Z_PINCTRL_BK7258_PIN_INIT(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#endif /* _SOC_BEKEN_BK7258_PINCTRL_SOC_H_ */
