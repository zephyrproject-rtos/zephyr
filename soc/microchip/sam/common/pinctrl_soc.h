/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MICROCHIP_SAM_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_MICROCHIP_SAM_COMMON_PINCTRL_SOC_H_

typedef struct pinctrl_soc_pin_t {
	uint32_t pin_mux;
	uint32_t bias_disable: 1;
	uint32_t bias_pull_down: 1;
	uint32_t bias_pull_up: 1;
	uint32_t drive_open_drain: 1;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) {                  \
		.pin_mux = DT_PROP_BY_IDX(node_id, prop, idx),          \
		.bias_disable = DT_PROP(node_id, bias_disable),         \
		.bias_pull_down = DT_PROP(node_id, bias_pull_down),     \
		.bias_pull_up = DT_PROP(node_id, bias_pull_up),         \
		.drive_open_drain = DT_PROP(node_id, drive_open_drain), \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) {           \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),    \
			       DT_FOREACH_PROP_ELEM, pinmux, \
			       Z_PINCTRL_STATE_PIN_INIT)     \
	}

#endif /* ZEPHYR_SOC_MICROCHIP_SAM_COMMON_PINCTRL_SOC_H_ */
