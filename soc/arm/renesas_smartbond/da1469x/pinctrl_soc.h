/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_SMARTBOND_DA1469X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_SMARTBOND_DA1469X_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/smartbond-pinctrl.h>

struct smartbond_pinctrl_soc_pin {
	uint32_t func : 6;
	uint32_t port : 1;
	uint32_t pin : 5;
	uint32_t bias_pull_up : 1;
	uint32_t bias_pull_down : 1;
};

typedef struct smartbond_pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			\
	{								\
		SMARTBOND_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		SMARTBOND_GET_PORT(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		SMARTBOND_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		DT_PROP(node_id, bias_pull_up),				\
		DT_PROP(node_id, bias_pull_down),			\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
				DT_FOREACH_PROP_ELEM, pinmux,		\
				Z_PINCTRL_STATE_PIN_INIT)}

#define SMARTBOND_GET_FUNC(pinmux) \
	(((pinmux) >> SMARTBOND_PINMUX_FUNC_POS) & SMARTBOND_PINMUX_FUNC_MASK)
#define SMARTBOND_GET_PORT(pinmux) \
	(((pinmux) >> SMARTBOND_PINMUX_PORT_POS) & SMARTBOND_PINMUX_PORT_MASK)
#define SMARTBOND_GET_PIN(pinmux) \
	(((pinmux) >> SMARTBOND_PINMUX_PIN_POS) & SMARTBOND_PINMUX_PIN_MASK)

#endif /* ZEPHYR_SOC_ARM_RENESAS_SMARTBOND_DA1469X_PINCTRL_SOC_H_ */
