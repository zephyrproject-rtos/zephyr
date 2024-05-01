/*
 * Copyright (c) 2020 Rapid Silicon.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * VIRGO SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_RISCV_VIRGO_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_VIRGO_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/rapidsi_virgo_pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for Rapidsi Virgo pin. */
typedef struct pinctrl_soc_pin {
	/** Pin number. */
	uint32_t pin_num;
	/* Pin configuration Contains the 
	 * Pin Mode as listed:
	 * 	PIN_FN_MAIN
	 * 	PIN_FN_FPGA
	 * 	PIN_FN_ALT
	 * 	PIN_FN_DEBUG
	 * Pin Pull Resistor Enable or Disabled
	 * Pin Pull Direction (Up or Down)
	 */
	/* 
	*/
	uint32_t pincfg;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pin_num field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_VIRGO_PINNUM_INIT(node_id) // here parse the group1 and group2 fields

/**
 * @brief Definitions used to initialize fields in #pinctrl_pin_t
 */
#define Z_PINCTRL_VIRGO_PINCFG_INIT(node_id) // here parse the group1 and group2 fields

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)		       \
	{ .pin_num = Z_PINCTRL_VIRGO_PINNUM_INIT(			       \
		DT_PROP_BY_IDX(node_id, state_prop, idx)),		       \
	  .pincfg = Z_PINCTRL_VIRGO_PINCFG_INIT(			       \
		DT_PROP_BY_IDX(node_id, state_prop, idx)) },

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_VIRGO_PINCTRL_SOC_H_ */
