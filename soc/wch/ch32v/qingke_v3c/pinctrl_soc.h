/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PINCTRL_SOC_H__
#define __PINCTRL_SOC_H__

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct ch57x_pinctrl_soc_pin {
	uint32_t config: 16;
	bool input_disable: 1;
	bool bias_pull_up: 1;
	bool bias_pull_down: 1;
	bool drive_push_pull: 1;
	bool push_pull_high_capacity: 1;
	bool output_high: 1;
	bool output_low: 1;
};

typedef struct ch57x_pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.config = DT_PROP_BY_IDX(node_id, prop, idx),                                      \
		.input_disable = DT_PROP(node_id, input_disable),                                  \
		.bias_pull_up = DT_PROP(node_id, bias_pull_up),                                    \
		.bias_pull_down = DT_PROP(node_id, bias_pull_down),                                \
		.drive_push_pull = DT_PROP(node_id, drive_push_pull),                              \
		.push_pull_high_capacity = DT_ENUM_IDX(node_id, output_capacity),                  \
		.output_high = DT_PROP(node_id, output_high),                                      \
		.output_low = DT_PROP(node_id, output_low),                                        \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif
