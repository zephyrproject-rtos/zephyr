/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_IMX_IMX8ULP_PINCTR_SOC_H_
#define ZEPHYR_SOC_NXP_IMX_IMX8ULP_PINCTR_SOC_H_

#include <zephyr/devicetree.h>
#include "fsl_iomuxc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pinmux {
	uint32_t mux_register;
	uint8_t mux_mode;
	uint32_t input_register;
	uint32_t input_daisy;
	uint32_t config_register;
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t pin_ctrl_flags;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define _PULL_IS_ENABLED(node_id)\
	(DT_PROP(node_id, bias_pull_up) || DT_PROP(node_id, bias_pull_down))

#define _IMX8ULP_PIN_FLAGS(node_id)						\
	((DT_ENUM_IDX_OR(node_id, drive_strength, 0) << IOMUXC_PCR_DSE_SHIFT) |	\
	 (DT_PROP(node_id, drive_open_drain) << IOMUXC_PCR_ODE_SHIFT) |		\
	 (DT_ENUM_IDX_OR(node_id, slew_rate, 0) << IOMUXC_PCR_SRE_SHIFT) |	\
	 (DT_PROP(node_id, bias_pull_up) << IOMUXC_PCR_PS_SHIFT) |		\
	 (_PULL_IS_ENABLED(node_id) << IOMUXC_PCR_PE_SHIFT))

#define _IMX8ULP_PINMUX(node_id)					\
	{								\
		.mux_register = DT_PROP_BY_IDX(node_id, pinmux, 0),	\
		.config_register = DT_PROP_BY_IDX(node_id, pinmux, 4),	\
		.input_register = DT_PROP_BY_IDX(node_id, pinmux, 2),	\
		.mux_mode = DT_PROP_BY_IDX(node_id, pinmux, 1),		\
		.input_daisy = DT_PROP_BY_IDX(node_id, pinmux, 3),	\
	}

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)\
	_IMX8ULP_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PIN_INIT(group_id, pin_prop, idx)		\
	{								\
		.pinmux = Z_PINCTRL_PINMUX(group_id, pin_prop, idx),	\
		.pin_ctrl_flags = _IMX8ULP_PIN_FLAGS(group_id),		\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)\
	{ DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),\
				 DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT) };

#ifdef __cplusplus
{
#endif

#endif /* ZEPHYR_SOC_NXP_IMX_IMX8ULP_PINCTR_SOC_H_ */
