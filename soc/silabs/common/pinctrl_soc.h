/*
 * Copyright (c) 2022 Silicon Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Silabs SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_SILABS_GECKO_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_SILABS_GECKO_COMMON_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#if CONFIG_PINCTRL_SILABS_DBUS
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/pinctrl/silabs-pinctrl-dbus.h>
#elif CONFIG_SOC_FAMILY_SILABS_S1
#include <zephyr/dt-bindings/pinctrl/gecko-pinctrl-s1.h>
#else
#include <zephyr/dt-bindings/pinctrl/gecko-pinctrl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#if CONFIG_PINCTRL_SILABS_DBUS

/** Type for Silabs pin using DBUS. */
typedef struct pinctrl_soc_pin {
	uint16_t base_offset;
	uint8_t port;
	uint8_t pin;
	uint8_t en_bit;
	uint8_t route_offset;
	uint8_t mode;
	uint8_t dout;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_SILABS_MODE_INIT(node)                                                           \
	(DT_PROP(node, drive_push_pull)     ? (4 + DT_PROP(node, silabs_alternate_port_control))   \
	 : DT_PROP(node, drive_open_source) ? (6 + DT_PROP(node, bias_pull_down))                  \
	 : DT_PROP(node, drive_open_drain)                                                         \
		 ? (8 + DT_PROP(node, silabs_input_filter) + 2 * DT_PROP(node, bias_pull_up) +     \
		    4 * DT_PROP(node, silabs_alternate_port_control))                              \
	 : DT_PROP(node, input_enable)                                                             \
		 ? ((DT_PROP(node, bias_pull_down) || DT_PROP(node, bias_pull_up))                 \
			    ? (2 + DT_PROP(node, silabs_input_filter))                             \
			    : 1)                                                                   \
		 : 0)

#define Z_PINCTRL_SILABS_DOUT_INIT(node)                                                           \
	(DT_PROP(node, drive_push_pull)    ? DT_PROP(node, output_high)                            \
	 : DT_PROP(node, drive_open_drain) ? 1                                                     \
	 : DT_PROP(node, input_enable)                                                             \
		 ? ((DT_PROP(node, bias_pull_down) || DT_PROP(node, bias_pull_up))                 \
			    ? DT_PROP(node, bias_pull_up)                                          \
			    : DT_PROP(node, silabs_input_filter))                                  \
	 : DT_PROP(node, input_disable) ? DT_PROP(node, bias_pull_up)                              \
					: 0)

#define Z_PINCTRL_STATE_PIN_INIT(node, prop, idx)                                                  \
	{.base_offset =                                                                            \
		 FIELD_GET(SILABS_PINCTRL_PERIPH_BASE_MASK, DT_PROP_BY_IDX(node, prop, idx)),      \
	 .port = FIELD_GET(SILABS_PINCTRL_GPIO_PORT_MASK, DT_PROP_BY_IDX(node, prop, idx)),        \
	 .pin = FIELD_GET(SILABS_PINCTRL_GPIO_PIN_MASK, DT_PROP_BY_IDX(node, prop, idx)),          \
	 .en_bit =                                                                                 \
		 (FIELD_GET(SILABS_PINCTRL_HAVE_EN_MASK, DT_PROP_BY_IDX(node, prop, idx))          \
			  ? FIELD_GET(SILABS_PINCTRL_EN_BIT_MASK, DT_PROP_BY_IDX(node, prop, idx)) \
			  : 0xFF),                                                                 \
	 .route_offset = FIELD_GET(SILABS_PINCTRL_ROUTE_MASK, DT_PROP_BY_IDX(node, prop, idx)),    \
	 .mode = Z_PINCTRL_SILABS_MODE_INIT(node),                                                 \
	 .dout = Z_PINCTRL_SILABS_DOUT_INIT(node)},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pins,             \
				Z_PINCTRL_STATE_PIN_INIT)}

#else

/** Type for gecko pin. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) (DT_PROP_BY_IDX(node_id, prop, idx)),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, psels,     \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

/**
 * @brief Utility macro to obtain pin function.
 *
 * @param pincfg Pin configuration bit field.
 */
#define GECKO_GET_FUN(pincfg) (((pincfg) >> GECKO_FUN_POS) & GECKO_FUN_MSK)

/**
 * @brief Utility macro to obtain port configuration.
 *
 * @param pincfg port configuration bit field.
 */
#define GECKO_GET_PORT(pincfg) (((pincfg) >> GECKO_PORT_POS) & GECKO_PORT_MSK)

/**
 * @brief Utility macro to obtain pin configuration.
 *
 * @param pincfg pin configuration bit field.
 */
#define GECKO_GET_PIN(pincfg) (((pincfg) >> GECKO_PIN_POS) & GECKO_PIN_MSK)

/**
 * @brief Utility macro to obtain location configuration.
 *
 * @param pincfg Loc configuration bit field.
 */
#define GECKO_GET_LOC(pincfg) (((pincfg) >> GECKO_LOC_POS) & GECKO_LOC_MSK)

/**
 * @brief Utility macro to obtain speed configuration.
 *
 * @param pincfg speed configuration bit field.
 */
#define GECKO_GET_SPEED(pincfg) (((pincfg) >> GECKO_SPEED_POS) & GECKO_SPEED_MSK)

#endif /* CONFIG_PINCTRL_SILABS_DBUS */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_SILABS_GECKO_COMMON_PINCTRL_SOC_H_ */
