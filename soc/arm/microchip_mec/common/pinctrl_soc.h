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
typedef struct {
	/** Pinmux settings (port, pin and function). */
	uint16_t pinmux;
	/** Pin configuration (bias, drive and slew rate). */
	uint16_t pincfg;
} pinctrl_soc_pin_t;

/* initialize pinmux member fields of pinctrl_pin_t */
#define Z_PINCTRL_MCHP_XEC_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

#define MCHP_XEC_BIAS_DIS_VAL(nid)	\
	(MCHP_XEC_NO_PULL * DT_PROP(nid, bias_disable))

#define MCHP_XEC_BIAS_PU_VAL(nid)	\
	(MCHP_XEC_PULL_UP * DT_PROP(nid, bias_pull_up))

#define MCHP_XEC_BIAS_PD_VAL(nid)	\
	(MCHP_XEC_PULL_DOWN * DT_PROP(nid, bias_pull_down))

#define MCHP_XEC_DRV_PP_VAL(nid)	\
	(MCHP_XEC_PUSH_PULL * DT_PROP(nid, drive_push_pull))

#define MCHP_XEC_DRV_OD_VAL(nid)	\
	(MCHP_XEC_OPEN_DRAIN * DT_PROP(nid, drive_open_drain))

#define MCHP_XEC_OVAL_DRV_LO(nid)	\
	(MCHP_XEC_OVAL_LOW * DT_PROP(nid, output_low))

#define MCHP_XEC_OVAL_DRV_HI(nid)	\
	(MCHP_XEC_OVAL_HIGH * DT_PROP(nid, output_high))

#define MCHP_XEC_LOW_POWER_EN(nid)	\
	(MCHP_XEC_PIN_LOW_POWER * DT_PROP(nid, low_power_enable))

#define MCHP_XEC_SLEW_VAL(nid)		\
	(DT_ENUM_IDX_OR(nid, slew_rate, MCHP_XEC_OSPEEDR_NO_CHG) << \
		MCHP_XEC_OSPEEDR_POS)

#define MCHP_XEC_DRVSTR_VAL(nid)	\
	(DT_ENUM_IDX_OR(nid, drive_strength, MCHP_XEC_ODRVSTR_NO_CHG) << \
		MCHP_XEC_ODRVSTR_POS)

/* initialize pincfg field in structure pinctrl_pin_t */
#define Z_PINCTRL_MCHP_XEC_PINCFG_INIT(node_id)				\
	((MCHP_XEC_BIAS_DIS_VAL(node_id) << MCHP_XEC_PUPDR_POS) |	\
	 (MCHP_XEC_BIAS_PU_VAL(node_id) << MCHP_XEC_PUPDR_POS) |	\
	 (MCHP_XEC_BIAS_PD_VAL(node_id) << MCHP_XEC_PUPDR_POS) |	\
	 (MCHP_XEC_DRV_PP_VAL(node_id) << MCHP_XEC_OTYPER_POS) |	\
	 (MCHP_XEC_DRV_OD_VAL(node_id) << MCHP_XEC_OTYPER_POS) |	\
	 (MCHP_XEC_OVAL_DRV_LO(node_id) << MCHP_XEC_OVAL_POS) |		\
	 (MCHP_XEC_OVAL_DRV_HI(node_id) << MCHP_XEC_OVAL_POS) |		\
	 (MCHP_XEC_LOW_POWER_EN(node_id) << MCHP_XEC_PIN_LOW_POWER_POS)	| \
	 MCHP_XEC_SLEW_VAL(node_id) |					\
	 MCHP_XEC_DRVSTR_VAL(node_id))

/* initialize pin structure members */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)		\
	{ .pinmux = Z_PINCTRL_MCHP_XEC_PINMUX_INIT(			\
		DT_PROP_BY_IDX(node_id, state_prop, idx)),		\
	  .pincfg = Z_PINCTRL_MCHP_XEC_PINCFG_INIT(			\
		DT_PROP_BY_IDX(node_id, state_prop, idx)), },

/* Use DT FOREACH macro to initialize each used pin */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_ */
