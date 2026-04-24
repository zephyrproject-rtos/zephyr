/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC4X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC4X_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/aurix-pinctrl.h>

struct tc4x_pinctrl {
	uint32_t port: 6;
	uint32_t pin: 4;
	uint32_t alt: 4;
	uint32_t type: 4;
	uint32_t analog: 1;
	uint32_t output: 1;
	uint32_t open_drain: 1;
	uint32_t pull_up: 1;
	uint32_t pull_down: 1;
	uint32_t output_high: 1;
	uint32_t output_low: 1;
	uint32_t output_select: 1;
	uint32_t pad_driver: 3;
	uint32_t pad_level: 3;
};

typedef struct tc4x_pinctrl pinctrl_soc_pin_t;

#define __HSFAST_DRIVE_MODE(drive_strength, slew_rate, output_select)                              \
	(output_select == 0 ? __NORMAL_DRIVE_MODE(drive_strength, slew_rate) : drive_strength)
#define __NORMAL_DRIVE_MODE(drive_strength, slew_rate) ((drive_strength << 1) | slew_rate)
#define PINCTRL_PAD_DRIVER(pad_type, drive_strength, slew_rate, output_select)                     \
	(pad_type ? __HSFAST_DRIVE_MODE(drive_strength, slew_rate, output_select)                  \
		  : __NORMAL_DRIVE_MODE(drive_strength, slew_rate))

#define Z_PINCTRL_STATE_PIN_INIT(node, prop, idx)                                                  \
	{.port = AURIX_PINMUX_PORT(DT_PROP_BY_IDX(node, prop, idx)),                               \
	 .pin = AURIX_PINMUX_PIN(DT_PROP_BY_IDX(node, prop, idx)),                                 \
	 .alt = AURIX_PINMUX_ALT(DT_PROP_BY_IDX(node, prop, idx)),                                 \
	 .type = AURIX_PINMUX_TYPE(DT_PROP_BY_IDX(node, prop, idx)),                               \
	 .analog = DT_PROP(node, input_disable) && !DT_PROP(node, output_enable),                  \
	 .output = DT_PROP(node, output_enable),                                                   \
	 .open_drain = DT_PROP(node, drive_open_drain),                                            \
	 .pull_up = DT_PROP(node, bias_pull_up),                                                   \
	 .pull_down = DT_PROP(node, bias_pull_down),                                               \
	 .output_high = DT_PROP(node, output_high),                                                \
	 .output_low = DT_PROP(node, output_low),                                                  \
	 .output_select = DT_PROP(node, output_select),                                            \
	 .pad_driver = PINCTRL_PAD_DRIVER(                                                         \
			AURIX_PINMUX_PAD_TYPE(                                                     \
				DT_PROP_BY_IDX(node, prop, idx)),                                  \
			DT_ENUM_IDX(node, drive_strength),                                         \
			DT_ENUM_IDX(node, slew_rate),                                              \
			DT_PROP(node, output_select)),                                             \
	 .pad_level = DT_ENUM_IDX(node, input_level)},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#endif /* ZEPHYR_SOC_INFINEON_AURIX_TC4X_PINCTRL_SOC_H_ */
