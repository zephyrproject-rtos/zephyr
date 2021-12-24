/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_

#include <devicetree.h>
#include <zephyr/types.h>
#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pinmux {
	uint32_t mux_register;
	uint32_t mux_mode;
	uint32_t input_register;
	uint32_t input_daisy;
	uint32_t config_register;
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t pin_ctrl_flags;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define MCUX_RT_INPUT_SCHMITT_ENABLE_SHIFT IOMUXC_SW_PAD_CTL_PAD_HYS_SHIFT
#define MCUX_RT_BIAS_PULL_DOWN_SHIFT IOMUXC_SW_PAD_CTL_PAD_PUS_SHIFT
#define MCUX_RT_BIAS_PULL_UP_SHIFT IOMUXC_SW_PAD_CTL_PAD_PUS_SHIFT
#define MCUX_RT_BIAS_BUS_HOLD_SHIFT IOMUXC_SW_PAD_CTL_PAD_PUE_SHIFT
#define MCUX_RT_PULL_ENABLE_SHIFT IOMUXC_SW_PAD_CTL_PAD_PKE_SHIFT
#define MCUX_RT_DRIVE_OPEN_DRAIN_SHIFT IOMUXC_SW_PAD_CTL_PAD_ODE_SHIFT
#define MCUX_RT_SPEED_SHIFT IOMUXC_SW_PAD_CTL_PAD_SPEED_SHIFT
#define MCUX_RT_DRIVE_STRENGTH_SHIFT IOMUXC_SW_PAD_CTL_PAD_DSE_SHIFT
#define MCUX_RT_SLEW_RATE_SHIFT IOMUXC_SW_PAD_CTL_PAD_SRE_SHIFT
#define MCUX_RT_INPUT_ENABLE_SHIFT 31 /* Shift to a bit not used by IOMUXC_SW_PAD_CTL */
#define MCUX_RT_INPUT_ENABLE(x) ((x >> MCUX_RT_INPUT_ENABLE_SHIFT) & 0x1)

#define Z_PINCTRL_MCUX_RT_PINCFG_INIT(node_id)							\
	((DT_PROP(node_id, input_schmitt_enable) << MCUX_RT_INPUT_SCHMITT_ENABLE_SHIFT) |	\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up), (DT_ENUM_IDX(node_id, bias_pull_up_value)	\
		<< MCUX_RT_BIAS_PULL_UP_SHIFT) |)						\
	IF_ENABLED(DT_PROP(node_id, bias_pull_down), (DT_ENUM_IDX(node_id, bias_pull_down_value)\
		<< MCUX_RT_BIAS_PULL_DOWN_SHIFT) |)						\
	((DT_PROP(node_id, bias_pull_down) | DT_PROP(node_id, bias_pull_up))			\
		<< MCUX_RT_BIAS_BUS_HOLD_SHIFT) |						\
	 ((!DT_PROP(node_id, bias_disable)) << MCUX_RT_PULL_ENABLE_SHIFT) |			\
	 (DT_PROP(node_id, drive_open_drain) << MCUX_RT_DRIVE_OPEN_DRAIN_SHIFT) |		\
	 (DT_ENUM_IDX(node_id, nxp_speed) << MCUX_RT_SPEED_SHIFT) |				\
	 (DT_ENUM_IDX(node_id, drive_strength) << MCUX_RT_DRIVE_STRENGTH_SHIFT) |		\
	 (DT_ENUM_IDX(node_id, slew_rate) << MCUX_RT_SLEW_RATE_SHIFT) |				\
	 (DT_PROP(node_id, input_enable) << MCUX_RT_INPUT_ENABLE_SHIFT))


#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx, pinmux_idx)			\
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx), pinmux, pinmux_idx)

#define Z_PINCTRL_STATE_PIN_INIT(group_id, pin_prop, idx)				\
	{										\
	  .pinmux.mux_register = Z_PINCTRL_PINMUX(group_id, pin_prop, idx, 0),		\
	  .pinmux.mux_mode = Z_PINCTRL_PINMUX(group_id, pin_prop, idx, 1),		\
	  .pinmux.input_register = Z_PINCTRL_PINMUX(group_id, pin_prop, idx, 2),	\
	  .pinmux.input_daisy = Z_PINCTRL_PINMUX(group_id, pin_prop, idx, 3),		\
	  .pinmux.config_register = Z_PINCTRL_PINMUX(group_id, pin_prop, idx, 4),	\
	  .pin_ctrl_flags = Z_PINCTRL_MCUX_RT_PINCFG_INIT(group_id),			\
	},


#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};	\


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_ */
