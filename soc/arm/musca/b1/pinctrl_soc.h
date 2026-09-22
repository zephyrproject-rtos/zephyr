/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/pinctrl/v2m-musca-b1-pinctrl.h>

/**
 * @brief Holds a pin's pinctrl configuration
 */
struct musca_pinctrl_soc_pin {
	/** Pin number 0..15 */
	uint32_t pin_num: 4;

	/** Alternative function */
	uint32_t alt_func: 1;
};

typedef struct musca_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		MUSCA_B1_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                          \
		MUSCA_B1_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                     \
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#define MUSCA_B1_GET_PIN_NUM(pinctrl) (((pinctrl) >> MUSCA_B1_EXP_NUM_POS) & MUSCA_B1_EXP_NUM_MASK)
#define MUSCA_B1_GET_PIN_ALT_FUNC(pinctrl)                                                         \
	(((pinctrl) >> MUSCA_B1_ALT_FUNC_POS) & MUSCA_B1_ALT_FUNC_MASK)
