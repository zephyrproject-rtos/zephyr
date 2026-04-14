/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Linumiz
 * Author: Sri Surya <srisurya@linumiz.com>
 */

#ifndef _SOC_TIVA_C_PINCTRL_SOC_H_
#define _SOC_TIVA_C_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/tiva-c-pinctrl.h>

typedef struct pinctrl_soc_pin {
	uint32_t pinmux;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)					\
	{.pinmux = DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux)},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)					\
	{										\
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)		\
	}

#endif /* _SOC_TIVA_C_PINCTRL_SOC_H_ */
