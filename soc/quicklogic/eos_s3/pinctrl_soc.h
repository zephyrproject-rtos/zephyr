/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_QUICKLOGIC_EOS_S3_PINCTRL_H_
#define ZEPHYR_SOC_ARM_QUICKLOGIC_EOS_S3_PINCTRL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc_pin_t {
	uint32_t pin;
	uint32_t iof;
	uint32_t input_enable: 1;
	uint32_t output_enable: 1;
	uint32_t pull_up: 1;
	uint32_t pull_down: 1;
	uint32_t high_impedance: 1;
	uint32_t slew_rate: 2;
	uint8_t drive_strength;
	uint32_t schmitt_enable: 1;
	uint32_t control_selection: 2;
} pinctrl_soc_pin_t;

#define QUICKLOGIC_EOS_S3_DT_PIN(node_id)			\
	{							\
		.pin = DT_PROP_BY_IDX(node_id, pinmux, 0),	\
		.iof = DT_PROP_BY_IDX(node_id, pinmux, 1),	\
		.input_enable = DT_PROP(node_id, input_enable),	\
		.output_enable = DT_PROP(node_id, output_enable),	\
		.pull_up = DT_PROP(node_id, bias_pull_up),	\
		.pull_down = DT_PROP(node_id, bias_pull_down),	\
		.high_impedance = DT_PROP(node_id, bias_high_impedance),	\
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),	\
		.drive_strength = DT_PROP_OR(node_id, drive_strength, 4),	\
		.schmitt_enable = DT_PROP(node_id, input_schmitt_enable),	\
		.control_selection = DT_ENUM_IDX(node_id, quicklogic_control_selection),	\
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)		\
	QUICKLOGIC_EOS_S3_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)		\
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_QUICKLOGIC_EOS_S3_PINCTRL_H_ */
