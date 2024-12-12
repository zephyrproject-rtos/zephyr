/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_SOPHGO_PINCTRL_H__
#define __SOC_SOPHGO_PINCTRL_H__

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/sophgo-cvi-pinctrl-common.h>

struct pinctrl_soc_pin {
	uint32_t fmux_idx;
	uint32_t fmux_sel;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define CVI_PINMUX_GET(pinmux, field) ((pinmux >> CVI_PINMUX_##field##_S) & CVI_PINMUX_##field##_M)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.fmux_idx = CVI_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), FMUX_IDX),          \
		.fmux_sel = CVI_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), FMUX_SEL),          \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* __SOC_SOPHGO_PINCTRL_H__ */
