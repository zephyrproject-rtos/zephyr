/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/pinctrl/arm-mps2-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct mps2_pinctrl_soc_pin {
	/** Pin number 0..52 */
	uint32_t pin_num : 6;
	/** Alternative function (UART, SPI, etc.) */
	uint32_t alt_func : 2;
	/** Enable the pin as an input */
	uint32_t input_enable : 1;
};

typedef struct mps2_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)				\
	{									\
		MPS2_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),		\
		MPS2_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		DT_PROP(node_id, input_enable),					\
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),			\
				DT_FOREACH_PROP_ELEM, pinmux,			\
				Z_PINCTRL_STATE_PIN_INIT)}

#define MPS2_GET_PIN_NUM(pinctrl) \
	(((pinctrl) >> MPS2_EXP_NUM_POS) & MPS2_EXP_NUM_MASK)
#define MPS2_GET_PIN_ALT_FUNC(pinctrl) \
	(((pinctrl) >> MPS2_ALT_FUNC_POS) & MPS2_ALT_FUNC_MASK)
