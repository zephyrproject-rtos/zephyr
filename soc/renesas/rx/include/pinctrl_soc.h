/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RX_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RENESAS_RX_COMMON_PINCTRL_SOC_H_

#include <stdbool.h>
#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rx.h>

#define PORT_POS     (8)
#define PFS_BIT_ISEL (6)

#define PIN_MODE_GPIO       (0)
#define PIN_MODE_PERIPHERAL (1)

struct rx_pin_config {
	bool pin_mode;
	bool analog_enable;
	bool output_enable;
	bool output_high;
	bool bias_pull_up;
	bool drive_open_drain;
	uint8_t drive_strength;
	uint8_t psels;
};

/**
 * @brief Type to hold a renesas ra pin's pinctrl configuration.
 */
struct rx_pinctrl_soc_pin {
	/** Port number 0..5, A..E, H, J */
	uint32_t port_num: 5;
	/** Pin number 0..7 */
	uint32_t pin_num: 4;
	/** config pin */
	struct rx_pin_config cfg;
};

typedef struct rx_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.port_num = RX_GET_PORT_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                   \
		.pin_num = RX_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                     \
		.cfg =                                                                             \
			{                                                                          \
				.pin_mode = PIN_MODE_PERIPHERAL,                                   \
				.analog_enable = DT_PROP(node_id, renesas_analog_enable),          \
				.output_enable = DT_PROP(node_id, output_enable),                  \
				.output_high = DT_PROP(node_id, output_high),                      \
				.bias_pull_up = DT_PROP(node_id, bias_pull_up),                    \
				.drive_open_drain = DT_PROP(node_id, drive_open_drain),            \
				.drive_strength = DT_ENUM_IDX(node_id, drive_strength),            \
				.psels = RX_GET_PSEL(DT_PROP_BY_IDX(node_id, prop, idx)),          \
			},                                                                         \
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, psels,            \
				Z_PINCTRL_STATE_PIN_INIT)}

#define RX_GET_PORT_NUM(pinctrl) (((pinctrl) >> RX_PORT_NUM_POS) & RX_PORT_NUM_MASK)
#define RX_GET_PIN_NUM(pinctrl)  (((pinctrl) >> RX_PIN_NUM_POS) & RX_PIN_NUM_MASK)

#define RX_GET_PSEL(pinctrl) (((pinctrl) >> RX_PSEL_POS) & RX_PSEL_MASK)

#endif /* ZEPHYR_SOC_RENESAS_RX_COMMON_PINCTRL_SOC_H_ */
