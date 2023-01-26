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
#include <zephyr/dt-bindings/pinctrl/gecko-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_SILABS_GECKO_COMMON_PINCTRL_SOC_H_ */
