/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RP1_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RP1_COMMON_H_

/**
 * @file
 * @brief Common definitions for the RP1 pinctrl driver.
 */

#include <stdint.h>
#include <zephyr/dt-bindings/pinctrl/rp1-pinctrl.h>

/**
 * Extract the GPIO number from an encoded RP1 pinmux value.
 *
 * @param pinctrl A pin configuration value defined in devicetree
 * @return GPIO number
 *
 */
#define RP1_GET_PIN_NUM(pinctrl)  (((pinctrl) >> RP1_PIN_NUM_POS) & RP1_PIN_NUM_MASK)

/**
 * Extract the alternate function selector from an encoded RP1 pinmux value.
 *
 * @param pinctrl A pin configuration value defined in devicetree
 * @return GPIO alternate function number
 */
#define RP1_GET_ALT_FUNC(pinctrl) (((pinctrl) >> RP1_ALT_FUNC_POS) & RP1_ALT_FUNC_MASK)

/**
 * @cond INTERNAL_HIDDEN
 */

/* Initialize one RP1 pinctrl entry from a devicetree property item. */
#define RASPBERRYPI_RP1_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                 \
	{                                                                                          \
		.pin_num = RP1_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                    \
		.alt_func = RP1_GET_ALT_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                  \
		.f_m = DT_PROP(node_id, raspberrypi_f_m),                                          \
		.oe_override = DT_PROP(node_id, raspberrypi_oe_override),                          \
		.out_override = DT_PROP(node_id, raspberrypi_out_override),                        \
		.in_override = DT_PROP(node_id, raspberrypi_in_override),                          \
		.irqmask_edge_low = DT_PROP(node_id, raspberrypi_irqmask_edge_low),                \
		.irqmask_edge_high = DT_PROP(node_id, raspberrypi_irqmask_edge_high),              \
		.irqmask_level_low = DT_PROP(node_id, raspberrypi_irqmask_level_low),              \
		.irqmask_level_high = DT_PROP(node_id, raspberrypi_irqmask_level_high),            \
		.irqmask_f_edge_low = DT_PROP(node_id, raspberrypi_irqmask_f_edge_low),            \
		.irqmask_f_edge_high = DT_PROP(node_id, raspberrypi_irqmask_f_edge_high),          \
		.irqmask_db_level_low = DT_PROP(node_id, raspberrypi_irqmask_db_level_low),        \
		.irqmask_db_level_high = DT_PROP(node_id, raspberrypi_irqmask_db_level_high),      \
		.irq_override = DT_PROP(node_id, raspberrypi_irq_override),                        \
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),                                      \
		.schmitt_enable = DT_PROP(node_id, input_schmitt_enable),                          \
		.pulldown = DT_PROP(node_id, bias_pull_down),                                      \
		.pullup = DT_PROP(node_id, bias_pull_up),                                          \
		.drive_strength = DT_ENUM_IDX(node_id, drive_strength),                            \
		.input_enable = DT_PROP(node_id, input_enable),                                    \
		.output_disable = DT_PROP(node_id, output_disable),                                \
	}

/* Per-pin RP1 pinctrl settings expanded from devicetree properties. */
struct raspberrypi_rp1_pinctrl_pinconfig {
	uint32_t pin_num: 5;
	uint32_t alt_func: 5;
	uint32_t f_m: 7;
	uint32_t oe_override: 2;
	uint32_t out_override: 2;
	uint32_t in_override: 2;
	uint32_t irqmask_edge_low: 1;
	uint32_t irqmask_edge_high: 1;
	uint32_t irqmask_level_low: 1;
	uint32_t irqmask_level_high: 1;
	uint32_t irqmask_f_edge_low: 1;
	uint32_t irqmask_f_edge_high: 1;
	uint32_t irqmask_db_level_low: 1;
	uint32_t irqmask_db_level_high: 1;
	uint32_t irq_override: 2;
	uint32_t slew_rate: 1;
	uint32_t schmitt_enable: 1;
	uint32_t pulldown: 1;
	uint32_t pullup: 1;
	uint32_t drive_strength: 4;
	uint32_t input_enable: 1;
	uint32_t output_disable: 1;
};

/* Apply one RP1 pin configuration entry. */
int raspberrypi_rp1_pinctrl_configure_pin(const struct raspberrypi_rp1_pinctrl_pinconfig *pin);

/**
 * @endcond
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_RP1_COMMON_H_ */
