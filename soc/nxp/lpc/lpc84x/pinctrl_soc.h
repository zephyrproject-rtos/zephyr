/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP LPC84x SoC specific pinctrl definitions
 */

#ifndef ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

/**
 * @brief SoC specific pinctrl pin type.
 */
typedef struct {
	/** Switch Matrix configuration (pinmux). */
	uint16_t swm_cfg;
	/** IOCON configuration (pull-up/down, open-drain, etc.). */
	uint16_t iocon_cfg;
} pinctrl_soc_pin_t;

/* SWM decoding macros */
#define LPC84X_SWM_PIN(cfg)      ((cfg) & 0xFF)
#define LPC84X_SWM_FUNC(cfg)     (((cfg) >> 8) & 0x7F)
#define LPC84X_SWM_IS_FIXED(cfg) (((cfg) & (1U << 15)) != 0)
#define LPC84X_SWM_NONE          0xFFFF

/* IOCON decoding macros */
#define LPC84X_IOCON_INDEX(cfg) ((cfg) & 0xFF)
#define LPC84X_IOCON_CFG(cfg)   (((cfg) >> 8) & 0xFF)

/**
 * @brief Build IOCON configuration bits for storage in pinctrl_soc_pin_t.
 * We store the IOCON register bits 3..10 directly, shifted right by 3.
 * This includes: Bias (MODE), Hysteresis (HYS), Invert (INV), and Open-drain (OD).
 */

#define Z_PINCTRL_IOCON_BIAS(node_id)                                                              \
	((DT_PROP(node_id, bias_pull_down) ? 1U : 0U) | (DT_PROP(node_id, bias_pull_up) ? 2U : 0U))

#define Z_PINCTRL_IOCON_PINCFG(node_id)                                                            \
	((IOCON_PIO_MODE(Z_PINCTRL_IOCON_BIAS(node_id)) |                                          \
	  IOCON_PIO_HYS(DT_PROP(node_id, nxp_hysteresis)) |                                        \
	  IOCON_PIO_INV(DT_PROP(node_id, nxp_invert)) |                                            \
	  IOCON_PIO_OD(DT_PROP(node_id, drive_open_drain))) >>                                     \
	 3)

/**
 * @brief Utility macro to initialize a pinmux + configuration entry.
 *
 * @param group Devicetree node identifier containing pin configurations.
 * @param pin_prop Property name containing pinmux/pinmux-index (usually 'pinmux').
 * @param idx Index of the pin configuration in @p pin_prop.
 */
#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	{                                                                                          \
		.swm_cfg = (DT_PROP_BY_IDX(group, pin_prop, idx) >> 8) & 0xFFFF,                   \
		.iocon_cfg = (DT_PROP_BY_IDX(group, pin_prop, idx) & 0xFF) |                       \
			     (Z_PINCTRL_IOCON_PINCFG(group) << 8),                                 \
	},

/**
 * @brief Utility macro to initialize all pins for a single pinctrl phandle.
 *
 * @param node_id Node identifier containing the pinctrl property.
 * @param prop Property name (e.g. pinctrl-0).
 * @param idx Index of the phandle in @p prop.
 */
#define Z_PINCTRL_STATE_PIN_INIT_PHANDLE(node_id, prop, idx)                                       \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE_BY_IDX(node_id, prop, idx), DT_FOREACH_PROP_ELEM,        \
			       pinmux, Z_PINCTRL_STATE_PIN_INIT)

/**
 * @brief Utility macro to initialize all state pins for a given pinctrl property.
 *
 * @param node_id Node identifier.
 * @param prop Property name (e.g. pinctrl-0).
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, Z_PINCTRL_STATE_PIN_INIT_PHANDLE, ())}

#endif /* ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_ */
