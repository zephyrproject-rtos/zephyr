/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_DSPIC33A_PINCTRL_SOC_H_
#define ZEPHYR_SOC_DSPIC33A_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
#include <zephyr/dt-bindings/pinctrl/mchp-p33ak128mc106-pinctrl.h>
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
#include <zephyr/dt-bindings/pinctrl/mchp-p33ak512mps512-pinctrl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc_pin {
	uint32_t pinmux;
} pinctrl_soc_pin_t;

/* Extract pinmux from DT */
#define Z_PINCTRL_DSPIC33_PINMUX_INIT(node_id) (uint32_t)(DT_PROP(node_id, pinmux))

/* For dsPIC33, no extra config bits for now → only pinmux */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	{.pinmux = Z_PINCTRL_DSPIC33_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))},

/* Expand each pin in the list */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_DSPIC33A_PINCTRL_SOC_H_ */
