/*
 * Copyright (c) 2023 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_AMBIQ_APOLLO3_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_AMBIQ_APOLLO3_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/ambiq-apollo3-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct apollo3_pinctrl_soc_pin {
	/** Pin number 0..74 */
	uint32_t pin_num: 7;
	/** Alternative function (UART, SPI, etc.) */
	uint32_t alt_func: 3;
	/** Enable the pin as an input */
	uint32_t input_enable: 1;
	/** Drive strength, relative to full-driver strength */
	uint32_t drive_strength: 2;
	/** Drive actively high or low */
	uint32_t push_pull: 1;
	/** Drive with open drain */
	uint32_t open_drain: 1;
	/** High impedance mode */
	uint32_t tristate: 1;
	/** Enable the internal pull up resistor */
	uint32_t bias_pull_up: 1;
	/** Enable the internal pull down resistor */
	uint32_t bias_pull_down: 1;
	/** pullup resistor value */
	uint32_t ambiq_pull_up_ohms: 3;
	/** IOM nCE module select */
	uint32_t iom_nce: 2;
	/** IOM or MSPI */
	uint32_t iom_mspi: 1;
	/** IOM/MSPI instance number */
	uint32_t iom_num: 3;
};

typedef struct apollo3_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		APOLLO3_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                           \
		APOLLO3_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                      \
		DT_PROP(node_id, input_enable),                                                    \
		DT_ENUM_IDX(node_id, drive_strength),                                              \
		DT_PROP(node_id, drive_push_pull),                                                 \
		DT_PROP(node_id, drive_open_drain),                                                \
		DT_PROP(node_id, bias_high_impedance),                                             \
		DT_PROP(node_id, bias_pull_up),                                                    \
		DT_PROP(node_id, bias_pull_down),                                                  \
		DT_ENUM_IDX(node_id, ambiq_pull_up_ohms),                                          \
		DT_PROP(node_id, ambiq_iom_nce_module),                                            \
		DT_PROP(node_id, ambiq_iom_mspi),                                                  \
		DT_PROP(node_id, ambiq_iom_num),                                                   \
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

#define APOLLO3_GET_PIN_NUM(pinctrl) (((pinctrl) >> APOLLO3_PIN_NUM_POS) & APOLLO3_PIN_NUM_MASK)
#define APOLLO3_GET_PIN_ALT_FUNC(pinctrl)                                                          \
	(((pinctrl) >> APOLLO3_ALT_FUNC_POS) & APOLLO3_ALT_FUNC_MASK)

#endif /* ZEPHYR_SOC_ARM_AMBIQ_APOLLO3_PINCTRL_SOC_H_ */
