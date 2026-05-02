/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_RENESAS_RZA2M_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RENESAS_RZA2M_PINCTRL_SOC_H_

#include <zephyr/types.h>

typedef struct pinctrl_soc_pin {
	uint8_t port;
	uint8_t pin;
	uint8_t func;
	uint8_t drive_strength;
} pinctrl_soc_pin_t;

#define RZA2M_PINS_PER_PORT 8

#define RZA2M_MUX_FUNC_MAX      (BIT(0) | BIT(1) | BIT(2))
#define RZA2M_FUNC_GPIO_INPUT   BIT(3) /* Pin as input */
#define RZA2M_FUNC_GPIO_OUTPUT  BIT(4) /* Pin as output */
#define RZA2M_FUNC_GPIO_INT_EN  BIT(5) /* Enable interrupt for gpio */
#define RZA2M_FUNC_GPIO_INT_DIS BIT(6) /* Disable interrupt for gpio */
#define RZA2M_FUNC_GPIO_HIZ     BIT(7) /* Hi-Z mode */
/*
 * Use 16 lower bits [15:0] for pin identifier
 * Use 16 higher bits [31:16] for pin mux function
 */
#define RZA2M_MUX_PIN_ID_MASK   GENMASK(15, 0)
#define RZA2M_MUX_FUNC_MASK     GENMASK(31, 16)
#define RZA2M_MUX_FUNC_OFFS     16
#define RZA2M_FUNC(prop)        ((prop & RZA2M_MUX_FUNC_MASK) >> RZA2M_MUX_FUNC_OFFS)
#define RZA2M_PINCODE(prop)     (prop & RZA2M_MUX_PIN_ID_MASK)
#define RZA2M_PORT(prop)        ((RZA2M_PINCODE(prop)) / RZA2M_PINS_PER_PORT)
#define RZA2M_PIN(prop)         ((RZA2M_PINCODE(prop)) % RZA2M_PINS_PER_PORT)

#define Z_PINCTRL_PINMUX_INIT(node_id, state_prop, idx)                                            \
	{                                                                                          \
		.port = RZA2M_PORT(DT_PROP_BY_IDX(node_id, state_prop, idx)),                      \
		.pin = RZA2M_PIN(DT_PROP_BY_IDX(node_id, state_prop, idx)),                        \
		.func = RZA2M_FUNC(DT_PROP_BY_IDX(node_id, state_prop, idx)),                      \
		.drive_strength = DT_PROP_OR(node_id, drive_strength, 0),                          \
	},

#define Z_PINCTRL_STATE_PIN_CHILD_INIT(node_id)                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pinmux),                                             \
		    (DT_FOREACH_PROP_ELEM(node_id, pinmux, Z_PINCTRL_PINMUX_INIT)),                \
		    ())

#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	DT_FOREACH_CHILD(DT_PHANDLE_BY_IDX(node_id, state_prop, idx),                              \
			 Z_PINCTRL_STATE_PIN_CHILD_INIT)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, Z_PINCTRL_STATE_PIN_INIT, ())};

#endif /* ZEPHYR_SOC_RENESAS_RZA2M_PINCTRL_SOC_H_ */
