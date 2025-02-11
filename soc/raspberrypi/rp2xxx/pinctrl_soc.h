/*
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RPI_PICO_RP2_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RPI_PICO_RP2_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/rpi-pico-rp2040-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
struct rpi_pinctrl_soc_pin {
	/** Pin number 0..29 */
	uint32_t pin_num : 5;
	/** Alternative function (UART, SPI, etc.) */
	uint32_t alt_func : 4;
	/** Maximum current used by a pin, in mA */
	uint32_t drive_strength : 4;
	/** Slew rate, may be either false (slow) or true (fast) */
	uint32_t slew_rate : 1;
	/** Enable the internal pull up resistor */
	uint32_t pullup : 1;
	/** Enable the internal pull down resistor */
	uint32_t pulldown : 1;
	/** Enable the pin as an input */
	uint32_t input_enable : 1;
	/** Enable the internal schmitt trigger */
	uint32_t schmitt_enable : 1;
};

typedef struct rpi_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)				\
	{									\
		RP2_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),		\
		RP2_GET_PIN_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		DT_ENUM_IDX(node_id, drive_strength),				\
		DT_ENUM_IDX(node_id, slew_rate),				\
		DT_PROP(node_id, bias_pull_up),					\
		DT_PROP(node_id, bias_pull_down),				\
		DT_PROP(node_id, input_enable),					\
		DT_PROP(node_id, input_schmitt_enable),				\
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

#define RP2_GET_PIN_NUM(pinctrl) \
	(((pinctrl) >> RP2_PIN_NUM_POS) & RP2_PIN_NUM_MASK)
#define RP2_GET_PIN_ALT_FUNC(pinctrl) \
	(((pinctrl) >> RP2_ALT_FUNC_POS) & RP2_ALT_FUNC_MASK)

#endif /* ZEPHYR_SOC_ARM_RPI_PICO_RP2_PINCTRL_SOC_H_ */
