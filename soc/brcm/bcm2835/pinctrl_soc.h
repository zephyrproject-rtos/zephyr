/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_BRCM_BCM2835_PINCTRL_SOC_H_
#define ZEPHYR_SOC_BRCM_BCM2835_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/brcm-bcm2835-pinctrl.h>
#include <zephyr/types.h>

struct bcm2835_pinctrl_soc_pin {
	uint32_t pin: 6;
	uint32_t function: 3;
	uint32_t bias_disable: 1;
	uint32_t bias_pull_up: 1;
	uint32_t bias_pull_down: 1;
};

typedef struct bcm2835_pinctrl_soc_pin pinctrl_soc_pin_t;

#define BCM2835_GET_PIN(pincfg) \
	(((pincfg) >> BCM2835_PIN_NUM_POS) & BCM2835_PIN_NUM_MASK)

#define BCM2835_GET_FUNC(pincfg) \
	(((pincfg) >> BCM2835_PIN_FUNC_POS) & BCM2835_PIN_FUNC_MASK)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) \
	{ \
		.pin = BCM2835_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)), \
		.function = BCM2835_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)), \
		.bias_disable = DT_PROP_OR(node_id, bias_disable, 0), \
		.bias_pull_up = DT_PROP_OR(node_id, bias_pull_up, 0), \
		.bias_pull_down = DT_PROP_OR(node_id, bias_pull_down, 0), \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux, \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_BRCM_BCM2835_PINCTRL_SOC_H_ */
