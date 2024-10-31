/*
 * Copyright (c) 2024 Rapid Silicon
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
#include <zephyr/dt-bindings/pinctrl/rapidsi_virgo_pinctrl.h>
#include <zephyr/types.h>

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
	uint32_t pin_cfg;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop property name.
 * @param idx property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                       \
	{.pin_num = DT_PHA_BY_IDX(node_id, prop, idx, pin_num),                                \
	 .pin_cfg =                                                                            \
		 (((PIN_ENABLE << PIN_ENABLE_OFFSET) & PIN_ENABLE_MASK) |                          \
		 ((DT_PHA_BY_IDX(node_id, prop, idx, pin_mode) << PIN_MODE_OFFSET) &               \
		  (PIN_MODE_MASK)) |                                                               \
		 ((DT_PHA_BY_IDX(node_id, prop, idx, pin_pull_state) << PIN_PULL_STATE_OFFSET) &   \
		  (PIN_PULL_STATE_MASK)) |                                                         \
		 ((DT_PHA_BY_IDX(node_id, prop, idx, pin_pull_dir) << PIN_PULL_DIR_OFFSET) &       \
		  (PIN_PULL_DIR_MASK)))},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM, pinmaps,		       \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_VIRGO_PINCTRL_SOC_H_ */
