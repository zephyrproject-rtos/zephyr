/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_CH32V003_PICO_RP2_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_CH32V003_PICO_RP2_PINCTRL_SOC_H_

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct ch32v003_pinctrl_soc_pin {
	uint32_t config: 24;
	bool bias_pull_up: 1;
	bool bias_pull_down: 1;
	bool drive_open_drain: 1;
	bool drive_push_pull: 1;
	uint8_t slew_rate: 2;
};

typedef struct ch32v003_pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.config = DT_PROP_BY_IDX(node_id, prop, idx),                                      \
		.bias_pull_up = DT_PROP(node_id, bias_pull_up),                                    \
		.bias_pull_down = DT_PROP(node_id, bias_pull_down),                                \
		.drive_open_drain = DT_PROP(node_id, drive_open_drain),                            \
		.drive_push_pull = DT_PROP(node_id, drive_push_pull),                              \
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),                                      \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#endif /* ZEPHYR_SOC_ARM_CH32V003_PICO_RP2_PINCTRL_SOC_H_ */
