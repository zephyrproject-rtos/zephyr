/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Linumiz GmbH
 * Author: Sri Surya  <srisurya@linumiz.com>
 */

#ifndef ZEPHYR_SOC_ARM_AMBIQ_APOLLO2_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_AMBIQ_APOLLO2_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/ambiq-apollo2-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct apollo2_pinctrl_soc_pin {
	/** Pin number 0..49 */
	uint32_t pin_num: 6;
	/** Alternative function */
	uint32_t alt_func: 3;
	/** Enable the pin as an input */
	uint32_t input_enable: 1;
	/** Drive strength 2MA, 4MA, 8MA and 12MA*/
	uint32_t drive_strength: 4;
	/** Drive with open drain */
	uint32_t open_drain: 1;
	/** Enable the internal pull up resistor */
	uint32_t bias_pull_up: 1;
	/** pullup resistor value */
	uint32_t ambiq_pull_up_ohms: 15;
};

typedef struct apollo2_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)						\
	{											\
		APOLLO2_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),			\
		APOLLO2_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),			\
		DT_PROP(node_id, input_enable),							\
		DT_PROP(node_id, drive_strength),						\
		DT_PROP(node_id, drive_open_drain),						\
		DT_PROP(node_id, bias_pull_up),							\
		DT_PROP(node_id, ambiq_pull_up_ohms),						\
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#define APOLLO2_GET_PIN_NUM(pinctrl) (((pinctrl) >> APOLLO2_PIN_NUM_POS) & APOLLO2_PIN_NUM_MASK)
#define APOLLO2_GET_PIN_ALT_FUNC(pinctrl)                                                          \
	(((pinctrl) >> APOLLO2_ALT_FUNC_POS) & APOLLO2_ALT_FUNC_MASK)

#endif /* ZEPHYR_SOC_ARM_AMBIQ_APOLLO2_PINCTRL_SOC_H_ */
