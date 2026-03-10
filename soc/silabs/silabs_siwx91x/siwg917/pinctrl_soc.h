/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_SILABS_SIWG917_PINCTRL_SOC_H_
#define ZEPHYR_SOC_SILABS_SIWG917_PINCTRL_SOC_H_

#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/silabs/siwx91x-pinctrl.h>

typedef struct pinctrl_soc_pin_t {
	uint8_t port;
	uint8_t pin;
	uint8_t ulppin;
	uint8_t mode;
	uint8_t ulpmode;
	uint8_t pad;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node, prop, idx) {                                           \
	.port    = FIELD_GET(SIWX91X_PINCTRL_PORT_MASK,    DT_PROP_BY_IDX(node, prop, idx)),  \
	.pin     = FIELD_GET(SIWX91X_PINCTRL_PIN_MASK,     DT_PROP_BY_IDX(node, prop, idx)),  \
	.ulppin  = FIELD_GET(SIWX91X_PINCTRL_ULPPIN_MASK,  DT_PROP_BY_IDX(node, prop, idx)),  \
	.mode    = FIELD_GET(SIWX91X_PINCTRL_MODE_MASK,    DT_PROP_BY_IDX(node, prop, idx)),  \
	.ulpmode = FIELD_GET(SIWX91X_PINCTRL_ULPMODE_MASK, DT_PROP_BY_IDX(node, prop, idx)),  \
	.pad     = FIELD_GET(SIWX91X_PINCTRL_PAD_MASK,     DT_PROP_BY_IDX(node, prop, idx)),  \
},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) {                                            \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM,               \
			       pinmux, Z_PINCTRL_STATE_PIN_INIT)                              \
}

#endif
