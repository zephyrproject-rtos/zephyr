/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_KB1200_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_KB1200_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/ene-kb1200-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;

/* initialize pinmux member fields of pinctrl_pin_t */
#define Z_PINCTRL_ENE_KB1200_PINMUX_INIT(node_id) (uint32_t)(DT_PROP(node_id, pinmux))

#define Z_PINCTRL_STATE_PINCFG_INIT(node_id)                                                       \
	((DT_PROP(node_id, bias_disable) << ENE_KB1200_NO_PUD_POS) |                               \
	 (DT_PROP(node_id, bias_pull_down) << ENE_KB1200_PD_POS) |                                 \
	 (DT_PROP(node_id, bias_pull_up) << ENE_KB1200_PU_POS) |                                   \
	 (DT_PROP(node_id, drive_push_pull) << ENE_KB1200_PUSH_PULL_POS) |                         \
	 (DT_PROP(node_id, drive_open_drain) << ENE_KB1200_OPEN_DRAIN_POS) |                       \
	 (DT_PROP(node_id, output_disable) << ENE_KB1200_OUT_DIS_POS) |                            \
	 (DT_PROP(node_id, output_enable) << ENE_KB1200_OUT_EN_POS) |                              \
	 (DT_PROP(node_id, output_high) << ENE_KB1200_OUT_HI_POS) |                                \
	 (DT_PROP(node_id, output_low) << ENE_KB1200_OUT_LO_POS) |                                 \
	 (DT_PROP(node_id, low_power_enable) << ENE_KB1200_PIN_LOW_POWER_POS))

/* initialize pin structure members */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	(Z_PINCTRL_ENE_KB1200_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx)) |              \
	 Z_PINCTRL_STATE_PINCFG_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))),

/* Use DT FOREACH macro to initialize each used pin */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_KB1200_PINCTRL_SOC_H_ */
