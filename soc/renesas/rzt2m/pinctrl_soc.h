/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RZT2M_PINCTRL_H_
#define ZEPHYR_SOC_ARM_RENESAS_RZT2M_PINCTRL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc_pin_t {
	uint32_t port;
	uint32_t pin;
	uint32_t func;
	uint32_t input_enable: 1;
	uint32_t output_enable: 1;
	uint32_t pull_up: 1;
	uint32_t pull_down: 1;
	uint32_t high_impedance: 1;
	uint32_t slew_rate: 2;
	uint8_t drive_strength;
	uint32_t schmitt_enable: 1;
} pinctrl_soc_pin_t;

#define RZT2M_GET_PORT(pinctrl) ((pinctrl >> 16) & 0xff)
#define RZT2M_GET_PIN(pinctrl) ((pinctrl >> 8) & 0xff)
#define RZT2M_GET_FUNC(pinctrl) (pinctrl & 0xff)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			\
	{							\
		.port = RZT2M_GET_PORT(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		.pin = RZT2M_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		.func = RZT2M_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		.input_enable = DT_PROP(node_id, input_enable),	\
		.pull_up = DT_PROP(node_id, bias_pull_up),	\
		.pull_down = DT_PROP(node_id, bias_pull_down),	\
		.high_impedance = DT_PROP(node_id, bias_high_impedance),	\
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),	\
		.drive_strength = DT_ENUM_IDX(node_id, drive_strength),	\
		.schmitt_enable = DT_PROP(node_id, input_schmitt_enable),	\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),	\
				DT_FOREACH_PROP_ELEM, pinmux,   \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_RENESAS_RZT2M_PINCTRL_H_ */
