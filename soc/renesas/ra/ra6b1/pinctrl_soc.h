/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/renesas/rafw-pinctrl.h>

struct ra6b1_pinctrl_soc_pin {
	uint32_t func: 6;
	uint32_t port: 2;
	uint32_t pin: 5;
	uint32_t bias_pull_up: 1;
	uint32_t bias_pull_down: 1;
	uint32_t output_enable: 1;
	uint32_t input_enable: 1;
	uint32_t isolation_enable : 1;
	uint32_t low_power_enable : 1;
};

typedef struct ra6b1_pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)						   \
	{											   \
		RA6B1_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),				   \
		RA6B1_GET_PORT(DT_PROP_BY_IDX(node_id, prop, idx)),				   \
		RA6B1_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),				   \
		DT_PROP(node_id, bias_pull_up),							   \
		DT_PROP(node_id, bias_pull_down),						   \
		DT_PROP(node_id, output_enable),						   \
		DT_PROP(node_id, input_enable),							   \
		DT_PROP(node_id, sleep_hardware_state),						   \
		DT_PROP(node_id, low_power_enable),						   \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)						   \
	{											   \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)				   \
	}

#define RA6B1_GET_FUNC(pinmux)									   \
	(((pinmux) >> RA6B1_PINMUX_FUNC_POS) & RA6B1_PINMUX_FUNC_MASK)
#define RA6B1_GET_PORT(pinmux)									   \
	(((pinmux) >> RA6B1_PINMUX_PORT_POS) & RA6B1_PINMUX_PORT_MASK)
#define RA6B1_GET_PIN(pinmux) (((pinmux) >> RA6B1_PINMUX_PIN_POS) & RA6B1_PINMUX_PIN_MASK)

#endif /* ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_PINCTRL_SOC_H_ */
