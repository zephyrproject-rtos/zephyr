/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Microchip XEC SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Type for MCHP XEC pin. */
typedef uint32_t pinctrl_soc_pin_t;

/* initialize pinmux member fields of pinctrl_pin_t */
#define Z_PINCTRL_MCHP_XEC_PINMUX_INIT(node_id) (uint32_t)(DT_PROP(node_id, pinmux))

#define Z_PINCTRL_STATE_PINCFG_INIT(node_id)							\
	((DT_PROP(node_id, bias_disable)                   << MCHP_XEC_NO_PUD_POS)		\
	 | (DT_PROP(node_id, bias_pull_down)               << MCHP_XEC_PD_POS)			\
	 | (DT_PROP(node_id, bias_pull_up)                 << MCHP_XEC_PU_POS)			\
	 | (DT_PROP(node_id, drive_push_pull)              << MCHP_XEC_PUSH_PULL_POS)		\
	 | (DT_PROP(node_id, drive_open_drain)             << MCHP_XEC_OPEN_DRAIN_POS)		\
	 | (DT_PROP(node_id, output_disable)               << MCHP_XEC_OUT_DIS_POS)		\
	 | (DT_PROP(node_id, output_enable)                << MCHP_XEC_OUT_EN_POS)		\
	 | (DT_PROP(node_id, output_high)                  << MCHP_XEC_OUT_HI_POS)		\
	 | (DT_PROP(node_id, output_low)                   << MCHP_XEC_OUT_LO_POS)		\
	 | (DT_PROP(node_id, low_power_enable)             << MCHP_XEC_PIN_LOW_POWER_POS)	\
	 | (DT_PROP(node_id, microchip_output_func_invert) << MCHP_XEC_FUNC_INV_POS)		\
	 | (DT_ENUM_IDX(node_id, slew_rate)                << MCHP_XEC_SLEW_RATE_POS)		\
	 | (DT_ENUM_IDX(node_id, drive_strength)           << MCHP_XEC_DRV_STR_POS))

/* initialize pin structure members */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)		\
	(Z_PINCTRL_MCHP_XEC_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))		\
	 | Z_PINCTRL_STATE_PINCFG_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))),

/* Use DT FOREACH macro to initialize each used pin */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_ */
