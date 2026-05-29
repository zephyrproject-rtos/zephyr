/*
 * Copyright 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT118X_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT118X_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCUX_IMX_ODE_SHIFT 4
#define MCUX_IMX_PUS_SHIFT 3
#define MCUX_IMX_PUE_SHIFT 2
#define MCUX_IMX_DSE_SHIFT 1
#define MCUX_IMX_SRE_SHIFT 0
#define MCUX_IMX_PULL_SHIFT 2
#define MCUX_IMX_PULL_PULLDOWN 0x2
#define MCUX_IMX_PULL_PULLUP 0x1
#define MCUX_IMX_PDRV_SHIFT 1
#define MCUX_IMX_INPUT_ENABLE_SHIFT 31 /* Shift to a bit not used by IOMUXC_SW_PAD_CTL */
#define MCUX_IMX_INPUT_ENABLE(x) ((x >> MCUX_IMX_INPUT_ENABLE_SHIFT) & 0x1)


/*
 * RT11xx has multiple types of register layouts defined for pin configuration
 * registers. There are four types defined:
 * pdrv_pull: registers lack a slew rate and pus field
 * pue_pus: registers have a slew rate and ode field
 * pue_pus_lpsr: in low power state retention domain, shifted ode field
 * pue_pus_snvs: in SNVS domain, shifted ode field
 */

#define MCUX_IMX_PUS_PUE 0
#define MCUX_IMX_PDRV_PULL 1
#define MCUX_IMX_LPSR 2
#define MCUX_IMX_SNVS 3

/*
 * Macro for MCUX_IMX_PULL_NOPULL, which needs to set field to 0x3 if two
 * properties are false
 */
#define MCUX_IMX_NOPULL(node_id)								\
	((0x2 & ((!DT_PROP(node_id, bias_pull_down) && !DT_PROP(node_id, bias_pull_up)) << 1)) |\
	(0x1 & ((!DT_PROP(node_id, bias_pull_down) && !DT_PROP(node_id, bias_pull_up)) << 0)))	\

#define Z_PINCTRL_MCUX_IMX_PDRV(node_id)							\
	IF_ENABLED(DT_PROP(node_id, bias_pull_down),						\
		(MCUX_IMX_PULL_PULLDOWN << MCUX_IMX_PULL_SHIFT) |)				\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up),						\
		(MCUX_IMX_PULL_PULLUP << MCUX_IMX_PULL_SHIFT) |)				\
	(MCUX_IMX_NOPULL(node_id) << MCUX_IMX_PULL_SHIFT) |					\
	((!DT_ENUM_IDX_OR(node_id, drive_strength, 0)) << MCUX_IMX_PDRV_SHIFT) |		\
	(DT_PROP(node_id, drive_open_drain) << MCUX_IMX_ODE_SHIFT) |				\
	(DT_PROP(node_id, input_enable) << MCUX_IMX_INPUT_ENABLE_SHIFT)

#define Z_PINCTRL_MCUX_IMX_PUE_PUS(node_id)							\
	(DT_PROP(node_id, bias_pull_up) << MCUX_IMX_PUS_SHIFT) |				\
	((DT_PROP(node_id, bias_pull_up) || DT_PROP(node_id, bias_pull_down))			\
		<< MCUX_IMX_PUE_SHIFT) |							\
	(DT_ENUM_IDX_OR(node_id, drive_strength, 0) << MCUX_IMX_DSE_SHIFT) |			\
	(DT_ENUM_IDX_OR(node_id, slew_rate, 0) << MCUX_IMX_SRE_SHIFT) |				\
	(DT_PROP(node_id, drive_open_drain) << MCUX_IMX_ODE_SHIFT) |				\
	(DT_PROP(node_id, input_enable) << MCUX_IMX_INPUT_ENABLE_SHIFT)

/* This struct must be present. It is used by the mcux gpio driver */
struct pinctrl_soc_pinmux {
	uint32_t mux_register; /* IOMUXC SW_PAD_MUX register */
	uint32_t config_register; /* IOMUXC SW_PAD_CTL register */
	uint32_t input_register; /* IOMUXC SELECT_INPUT DAISY register */
	uint8_t mux_mode: 4; /* Mux value for SW_PAD_MUX register */
	uint32_t input_daisy:4; /* Mux value for SELECT_INPUT_DAISY register */
	uint8_t pue_mux: 1; /* Is pinmux reg pue type */
	uint8_t pdrv_mux: 1; /* Is pinmux reg pdrv type */
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t pin_ctrl_flags; /* value to write to IOMUXC_SW_PAD_CTL register */
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

/* This definition must be present. It is used by the mcux gpio driver */
#define MCUX_IMX_PINMUX(node_id)						\
	{									\
	  .mux_register = DT_PROP_BY_IDX(node_id, pinmux, 0),			\
	  .config_register = DT_PROP_BY_IDX(node_id, pinmux, 4),		\
	  .input_register = DT_PROP_BY_IDX(node_id, pinmux, 2),			\
	  .mux_mode = DT_PROP_BY_IDX(node_id, pinmux, 1),			\
	  .input_daisy = DT_PROP_BY_IDX(node_id, pinmux, 3),			\
	  .pue_mux = DT_PROP(node_id, pin_pue),					\
	  .pdrv_mux = DT_PROP(node_id, pin_pdrv),				\
	}

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)				\
	MCUX_IMX_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PIN_INIT(group_id, pin_prop, idx)			\
	{									\
	  .pinmux = Z_PINCTRL_PINMUX(group_id, pin_prop, idx),			\
IF_ENABLED(DT_PROP(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx), pin_pue),	\
	  (.pin_ctrl_flags = Z_PINCTRL_MCUX_IMX_PUE_PUS(group_id),))		\
IF_ENABLED(DT_PROP(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx), pin_pdrv),	\
	  (.pin_ctrl_flags = Z_PINCTRL_MCUX_IMX_PDRV(group_id),))		\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};	\


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_RT118X_H_ */
