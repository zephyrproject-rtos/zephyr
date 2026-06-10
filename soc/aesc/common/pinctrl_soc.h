/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AESC_PINCTRL_H_
#define ZEPHYR_SOC_AESC_PINCTRL_H_

#include <zephyr/types.h>

typedef struct {
	const struct device *dev;
	uint8_t pin;
	uint8_t mux;
} pinctrl_soc_pin_t;

#define AESC_DT_PIN(node_id)						      \
	{								      \
		.dev = DEVICE_DT_GET(DT_PARENT(node_id)),		      \
		.pin = DT_PROP_BY_IDX(node_id, pinmux, 0),		      \
		.mux = DT_PROP_BY_IDX(node_id, pinmux, 1)		      \
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			      \
	AESC_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			      \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif /* ZEPHYR_SOC_AESC_PINCTRL_H_ */
