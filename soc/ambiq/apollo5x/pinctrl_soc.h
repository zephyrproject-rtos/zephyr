/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_AMBIQ_APOLLO5_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_AMBIQ_APOLLO5_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/ambiq-apollo5-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct apollo5_pinctrl_soc_pin {
	/** Pin number 0..223 */
	uint64_t pin_num: 8;
	/** Alternative function (UART, SPI, etc.) */
	uint64_t alt_func: 4;
	/** Enable the pin as an input */
	uint64_t input_enable: 1;
	/** Drive strength, relative to full-driver strength */
	uint64_t drive_strength: 3;
	/** Drive actively high or low */
	uint64_t push_pull: 1;
	/** Drive with open drain */
	uint64_t open_drain: 1;
	/** High impedance mode */
	uint64_t tristate: 1;
	/** Enable the internal pull up resistor */
	uint64_t bias_pull_up: 1;
	/** Enable the internal pull down resistor */
	uint64_t bias_pull_down: 1;
	/** pullup resistor value */
	uint64_t ambiq_pull_up_ohms: 3;
	/** nCE module select */
	uint64_t nce: 6;
	/** nCE module polarity */
	uint64_t nce_pol: 1;
	/** SDIF CD WP pad select */
	uint64_t sdif_cdwp: 3;
};

typedef struct apollo5_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                       \
	{                                                                                      \
		APOLLO5_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                           \
		APOLLO5_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                      \
		DT_PROP(node_id, input_enable),                                                    \
		DT_ENUM_IDX(node_id, drive_strength),                                              \
		DT_PROP(node_id, drive_push_pull),                                                 \
		DT_PROP(node_id, drive_open_drain),                                                \
		DT_PROP(node_id, bias_high_impedance),                                             \
		DT_PROP(node_id, bias_pull_up),                                                    \
		DT_PROP(node_id, bias_pull_down),                                                  \
		DT_ENUM_IDX(node_id, ambiq_pull_up_ohms),                                          \
		DT_PROP(node_id, ambiq_nce_src),                                                   \
		DT_PROP(node_id, ambiq_nce_pol),                                                   \
		DT_PROP(node_id, ambiq_sdif_cdwp),                                                 \
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                \
        {DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),                      \
                    DT_FOREACH_PROP_ELEM, pinmux,                               \
                    Z_PINCTRL_STATE_PIN_INIT)}

#define APOLLO5_GET_PIN_NUM(pinctrl)                                            \
        (((pinctrl) >> APOLLO5_PIN_NUM_POS) & APOLLO5_PIN_NUM_MASK)
#define APOLLO5_GET_PIN_ALT_FUNC(pinctrl)                                       \
        (((pinctrl) >> APOLLO5_ALT_FUNC_POS) & APOLLO5_ALT_FUNC_MASK)


#endif /* ZEPHYR_SOC_ARM_AMBIQ_APOLLO5_PINCTRL_SOC_H_ */
