/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC3X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC3X_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/aurix-pinctrl.h>

struct tc3x_pinctrl {
	uint32_t pin: 4;
	uint32_t port: 5;
	uint32_t alt: 3;
	uint32_t type: 4;
	uint32_t analog: 1;
	uint32_t output: 1;
	uint32_t pull_up: 1;
	uint32_t pull_down: 1;
	uint32_t open_drain: 1;
	uint32_t output_high: 1;
	uint32_t output_low: 1;
	uint32_t output_select: 1;
	uint32_t pad_driver_mode: 2;
	uint32_t pad_level_sel: 2;
};

typedef struct tc3x_pinctrl pinctrl_soc_pin_t;

#define RFAST_DRIVE_MODE(node, prop, idx)                                                          \
	COND_CODE_1(IS_EQ(DT_ENUM_IDX(node, drive_strength), 0), (DT_ENUM_IDX(node, slew_rate)),   \
		    (COND_CODE_1(IS_EQ(DT_ENUM_IDX(node, drive_strength), 1), (2), (3))))
#define FAST_DRIVE_MODE(node, prop, idx)                                                           \
	COND_CODE_1(IS_EQ(DT_ENUM_IDX(node, drive_strength), 0), (DT_ENUM_IDX(node, slew_rate)),   \
		    (COND_CODE_1(IS_EQ(DT_ENUM_IDX(node, drive_strength), 1), (2), (3))))

#define SLOW_DRIVE_MODE(node, prop, idx)                                                           \
	COND_CODE_1(IS_EQ(DT_ENUM_IDX(node, drive_strength), 1), (DT_ENUM_IDX(node, slew_rate)),   \
		    (3))

#define PAD_DRIVE_MODE(node, prop, idx)                                                            \
	AURIX_PINMUX_PAD_TYPE(DT_PROP_BY_IDX(node, prop, idx)) == AURIX_PAD_TYPE_RFAST             \
		? RFAST_DRIVE_MODE(node, prop, idx)                                                \
	: AURIX_PINMUX_PAD_TYPE(DT_PROP_BY_IDX(node, prop, idx)) == AURIX_PAD_TYPE_FAST            \
		? FAST_DRIVE_MODE(node, prop, idx)                                                 \
		: SLOW_DRIVE_MODE(node, prop, idx)

#define Z_PINCTRL_STATE_PIN_INIT(node, prop, idx)                                                  \
	{.port = AURIX_PINMUX_PORT(DT_PROP_BY_IDX(node, prop, idx)),                               \
	 .pin = AURIX_PINMUX_PIN(DT_PROP_BY_IDX(node, prop, idx)),                                 \
	 .alt = AURIX_PINMUX_ALT(DT_PROP_BY_IDX(node, prop, idx)),                                 \
	 .type = AURIX_PINMUX_TYPE(DT_PROP_BY_IDX(node, prop, idx)),                               \
	 .analog = DT_PROP(node, input_disable) && !DT_PROP(node, output_enable),                  \
	 .output = DT_PROP(node, output_enable),                                                   \
	 .pull_up = DT_PROP(node, bias_pull_up),                                                   \
	 .pull_down = DT_PROP(node, bias_pull_down),                                               \
	 .open_drain = DT_PROP(node, drive_open_drain),                                            \
	 .output_high = DT_PROP(node, output_high),                                                \
	 .output_low = DT_PROP(node, output_low),                                                  \
	 .output_select = DT_PROP(node, output_select),                                            \
	 .pad_driver_mode = PAD_DRIVE_MODE(node, prop, idx),                                       \
	 .pad_level_sel = DT_ENUM_IDX(node, input_level)},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#endif /* ZEPHYR_SOC_INFINEON_AURIX_TC3X_PINCTRL_SOC_H_ */
