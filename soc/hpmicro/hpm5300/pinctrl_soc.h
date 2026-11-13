/*
 * Copyright (c) 2022-2025 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_HPMICRO_HPM5300_PINCTRL_SOC_H_
#define SOC_HPMICRO_HPM5300_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/hpmicro-pinctrl-common.h>
#include <zephyr/types.h>

struct pinctrl_soc_pin {
	uint32_t pinmux;
	uint32_t pincfg;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, prop, mask, shift)                                     \
	((((uint32_t)DT_PROP(node_id, prop)) & (mask)) << (shift))

#define Z_PINCTRL_HPMICRO_CFG_ENUM(node_id, prop, mask, shift)                                     \
	((((uint32_t)DT_ENUM_IDX(node_id, prop)) & (mask)) << (shift))

#define Z_PINCTRL_HPMICRO_PINCFG_INIT(node_id)                                                     \
	(Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, drive_open_drain, HPMICRO_OPEN_DRAIN_MASK,            \
				    HPMICRO_OPEN_DRAIN_SHIFT) |                                    \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, bias_disable, HPMICRO_NO_PULL_MASK,                   \
				    HPMICRO_NO_PULL_SHIFT) |                                       \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, bias_pull_down, HPMICRO_PULL_DOWN_MASK,               \
				    HPMICRO_PULL_DOWN_SHIFT) |                                     \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, bias_pull_up, HPMICRO_PULL_UP_MASK,                   \
				    HPMICRO_PULL_UP_SHIFT) |                                       \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, input_enable, HPMICRO_FORCE_INPUT_MASK,               \
				    HPMICRO_FORCE_INPUT_SHIFT) |                                   \
	 Z_PINCTRL_HPMICRO_CFG_ENUM(node_id, drive_strength, HPMICRO_DRIVER_STRENGTH_MASK,         \
				    HPMICRO_DRIVER_STRENGTH_SHIFT) |                               \
	 Z_PINCTRL_HPMICRO_CFG_ENUM(node_id, hpmicro_slew_pd, HPMICRO_SLEW_PD_MASK,                \
				    HPMICRO_SLEW_PD_SHIFT) |                                       \
	 Z_PINCTRL_HPMICRO_CFG_ENUM(node_id, hpmicro_pull_resistance, HPMICRO_PULL_STRENGTH_MASK,  \
				    HPMICRO_PULL_STRENGTH_SHIFT) |                                 \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, input_schmitt_enable, HPMICRO_SCHMITT_ENABLE_MASK,    \
				    HPMICRO_SCHMITT_ENABLE_SHIFT) |                                \
	 Z_PINCTRL_HPMICRO_CFG_ENUM(node_id, slew_rate, HPMICRO_SLEW_RATE_MASK,                    \
				    HPMICRO_SLEW_RATE_SHIFT) |                                     \
	 Z_PINCTRL_HPMICRO_CFG_BOOL(node_id, hpmicro_keeper_disable, HPMICRO_KEEPER_DISABLE_MASK,  \
				    HPMICRO_KEEPER_DISABLE_SHIFT))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pinmux = DT_PROP_BY_IDX(node_id, prop, idx),                                      \
		.pincfg = Z_PINCTRL_HPMICRO_PINCFG_INIT(node_id),                                  \
	},

#define Z_PINCTRL_STATE_PIN_CHILD_INIT(node_id)                                                    \
	DT_FOREACH_PROP_ELEM(node_id, pinmux, Z_PINCTRL_STATE_PIN_INIT)

#define Z_PINCTRL_STATE_PIN_GROUP_INIT(node_id, state_prop, idx)                                   \
	DT_FOREACH_CHILD(DT_PHANDLE_BY_IDX(node_id, state_prop, idx),                              \
			 Z_PINCTRL_STATE_PIN_CHILD_INIT)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, Z_PINCTRL_STATE_PIN_GROUP_INIT, ())}

#endif /* SOC_HPMICRO_HPM5300_PINCTRL_SOC_H_ */
