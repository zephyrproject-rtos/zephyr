/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RISCV_PRIVILEGE_HI3861_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_PRIVILEGE_HI3861_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/hi3861-pinctrl.h>

#define HI3861_PAD_CTRL_SE_S 3
#define HI3861_PAD_CTRL_DS_S 4
#define HI3861_PAD_CTRL_SR_S 7
#define HI3861_PAD_CTRL_PU_S 8
#define HI3861_PAD_CTRL_PD_S 9
#define HI3861_PAD_CTRL_IE_S 10

struct pinctrl_soc_pin {
	uint32_t pad;
	uint32_t signal;
	uint32_t pad_ctrl;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define PAD_DRIVE_STRENGTH(node_id, prop, idx)                                                     \
	(DT_ENUM_IDX_OR(node_id, drive_strength,                                                   \
			HI3861_GET_DS_INIT(DT_PROP_BY_IDX(node_id, prop, idx))) &                  \
	 HI3861_GET_DS_MASK(DT_PROP_BY_IDX(node_id, prop, idx)))

#define Z_PINCTRL_HI3861_PAD_CTRL_INIT(node_id, prop, idx)                                         \
	((DT_PROP(node_id, input_schmitt_enable) << HI3861_PAD_CTRL_SE_S) |                        \
	 (PAD_DRIVE_STRENGTH(node_id, prop, idx) << HI3861_PAD_CTRL_DS_S) |                        \
	 (DT_ENUM_IDX(node_id, slew_rate) << HI3861_PAD_CTRL_SR_S) |                               \
	 (DT_PROP(node_id, bias_pull_up) << HI3861_PAD_CTRL_PU_S) |                                \
	 (DT_PROP(node_id, bias_pull_down) << HI3861_PAD_CTRL_PD_S) |                              \
	 (DT_PROP(node_id, input_enable) << HI3861_PAD_CTRL_IE_S))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pad = HI3861_GET_PAD(DT_PROP_BY_IDX(node_id, prop, idx)),                         \
		.signal = HI3861_GET_SIGNAL(DT_PROP_BY_IDX(node_id, prop, idx)),                   \
		.pad_ctrl = Z_PINCTRL_HI3861_PAD_CTRL_INIT(node_id, prop, idx),                    \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#define HI3861_GET_PAD(pinmux)     ((pinmux >> HI3861_PINMUX_PAD_S) & HI3861_PINMUX_PAD_M)
#define HI3861_GET_SIGNAL(pinmux)  ((pinmux >> HI3861_PINMUX_SIGNAL_S) & HI3861_PINMUX_SIGNAL_M)
#define HI3861_GET_DS_INIT(pinmux) ((pinmux >> HI3861_PINMUX_DS_INIT_S) & HI3861_PINMUX_DS_INIT_M)
#define HI3861_GET_DS_MASK(pinmux) ((pinmux >> HI3861_PINMUX_DS_MASK_S) & HI3861_PINMUX_DS_MASK_M)

#endif /* ZEPHYR_SOC_RISCV_PRIVILEGE_HI3861_PINCTRL_SOC_H_ */
