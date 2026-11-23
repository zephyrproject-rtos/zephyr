/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ROCKCHIP_RK3588_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ROCKCHIP_RK3588_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/rockchip-rk3588.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rk3588_pinctrl_bias {
	RK3588_PINCTRL_BIAS_NONE,
	RK3588_PINCTRL_BIAS_DISABLE,
	RK3588_PINCTRL_BIAS_PULL_DOWN,
	RK3588_PINCTRL_BIAS_PULL_UP,
} rk3588_pinctrl_bias_t;

#define RK3588_PIN_COUNT 160U

#define RK3588_PINCTRL_INPUT_ENABLE    BIT(0)
#define RK3588_PINCTRL_INPUT_DISABLE   BIT(1)
#define RK3588_PINCTRL_SCHMITT_ENABLE  BIT(2)
#define RK3588_PINCTRL_SCHMITT_DISABLE BIT(3)

struct rk3588_pinctrl_soc_pin {
	uint8_t pin;
	uint8_t function;
	rk3588_pinctrl_bias_t bias;
	uint8_t drive_strength_ohms;
	uint8_t flags;
};

typedef struct rk3588_pinctrl_soc_pin pinctrl_soc_pin_t;

#define RK3588_DT_PIN(pinmux) ((RK3588_PINMUX_BANK(pinmux) * 32U) + RK3588_PINMUX_PIN(pinmux))

#define RK3588_DT_BIAS(node_id)                                                                    \
	(DT_PROP(node_id, bias_pull_up)     ? RK3588_PINCTRL_BIAS_PULL_UP                          \
	 : DT_PROP(node_id, bias_pull_down) ? RK3588_PINCTRL_BIAS_PULL_DOWN                        \
	 : DT_PROP(node_id, bias_disable)   ? RK3588_PINCTRL_BIAS_DISABLE                          \
					    : RK3588_PINCTRL_BIAS_NONE)

#define RK3588_DT_FLAGS(node_id)                                                                   \
	((DT_PROP(node_id, input_enable) ? RK3588_PINCTRL_INPUT_ENABLE : 0U) |                     \
	 (DT_PROP(node_id, input_disable) ? RK3588_PINCTRL_INPUT_DISABLE : 0U) |                   \
	 (DT_PROP(node_id, input_schmitt_enable) ? RK3588_PINCTRL_SCHMITT_ENABLE : 0U) |           \
	 (DT_PROP(node_id, input_schmitt_disable) ? RK3588_PINCTRL_SCHMITT_DISABLE : 0U))

#define RK3588_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                          \
	{                                                                                          \
		.pin = (uint8_t)RK3588_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),                 \
		.function = (uint8_t)RK3588_PINMUX_FUNCTION(DT_PROP_BY_IDX(node_id, prop, idx)),   \
		.bias = (rk3588_pinctrl_bias_t)RK3588_DT_BIAS(node_id),                            \
		.drive_strength_ohms =                                                             \
			(uint8_t)DT_PROP_OR(node_id, rockchip_drive_strength_ohms, 0),             \
		.flags = (uint8_t)RK3588_DT_FLAGS(node_id),                                        \
	},

#define RK3588_PINCTRL_STATE_INIT(node_id, prop, idx, unused)                                      \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE_BY_IDX(node_id, prop, idx), DT_FOREACH_PROP_ELEM,        \
			       pinmux, RK3588_PINCTRL_STATE_PIN_INIT)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_VARGS(node_id, prop, RK3588_PINCTRL_STATE_INIT, 0)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ROCKCHIP_RK3588_PINCTRL_SOC_H_ */
