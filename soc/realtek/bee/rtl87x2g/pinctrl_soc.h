/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTL87X2G SoC Specific Pinctrl Data Structures
 *
 * This file defines the pinctrl state structure and initialization macros
 * used by the Zephyr pinctrl driver for the Realtek RTL87X2G series.
 */

#ifndef ZEPHYR_SOC_REALTEK_BEE_RTL87X2G_PINCTRL_SOC_H_
#define ZEPHYR_SOC_REALTEK_BEE_RTL87X2G_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/rtl87x2g-pinctrl.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Realtek RTL87X2G pin configuration structure.
 *
 * This structure holds the decoded configuration for a single pin,
 * using bitfields to optimize storage.
 */
struct pinctrl_soc_pin {
	/* Word 0 (32 bits) */
	uint32_t pin: 11;     /**< Pin number (bit[0:10]) */
	uint32_t pull_dis: 1; /**< Bias disable (bit[11]) */
	uint32_t pull_dir: 1; /**< Bias pull direction: 1 for Pull-up, 0 for Pull-down (bit[12]) */
	uint32_t drive: 1;    /**< Initial output level: 1 for High, 0 for Low (bit[13]) */
	uint32_t dir: 1;      /**< Output enable/Direction: 1 for Output, 0 for Input (bit[14]) */
	uint32_t pull_strength: 1; /**< Pull strength: 1 for Strong, 0 for Weak (bit[15]) */
	uint32_t fun: 16;          /**< Pinmux function index (bit[16:31]) */

	/* Word 1 (Partial) */
	uint32_t reserved_32: 1;   /**< Reserved (bit[32]) */
	uint32_t reserved_33: 1;   /**< Reserved (bit[33]) */
	uint32_t current_level: 2; /**< Drive current level (bit[34:36]) */
};

/**
 * @brief Typedef for the pinctrl soc pin structure.
 */
typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize a pinctrl_soc_pin object from Devicetree.
 *
 * @param node_id The Devicetree node identifier.
 * @param prop The property name (usually 'pinctrl-N').
 * @param idx The index in the property array.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pin = BEE_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),                            \
		.fun = BEE_GET_FUN(DT_PROP_BY_IDX(node_id, prop, idx)),                            \
		.pull_dis = DT_PROP_OR(node_id, bias_disable, 0),                                  \
		.pull_dir = DT_PROP_OR(node_id, bias_pull_up, 0),                                  \
		.drive = DT_PROP_OR(node_id, output_high, 0),                                      \
		.dir = DT_PROP_OR(node_id, output_enable, 0),                                      \
		.pull_strength = DT_PROP_OR(node_id, bias_pull_strong, 0),                         \
		.current_level = DT_PROP_OR(node_id, current_level, 0),                            \
	},

/**
 * @brief Utility macro to initialize a list of pinctrl_soc_pin objects.
 *
 * @param node_id The Devicetree node identifier.
 * @param prop The property name.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, psels,            \
				Z_PINCTRL_STATE_PIN_INIT)}

/**
 * @brief Extract the Function ID from the pinctrl specifier.
 */
#define BEE_GET_FUN(pincfg) (((pincfg) >> BEE_FUN_POS) & BEE_FUN_MSK)

/**
 * @brief Extract the Pin ID from the pinctrl specifier.
 */
#define BEE_GET_PIN(pincfg) (((pincfg) >> BEE_PIN_POS) & BEE_PIN_MSK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_REALTEK_BEE_RTL87X2G_PINCTRL_SOC_H_ */
