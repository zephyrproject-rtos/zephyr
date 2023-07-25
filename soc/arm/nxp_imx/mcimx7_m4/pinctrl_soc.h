/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_MCIMX6X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_MCIMX6X_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(x) ((x) & 0xF)
#define IOMUXC_SW_MUX_CTL_PAD_SION(x) (((x) & 0x1) << 4)
#define IOMUXC_SELECT_INPUT_DAISY(x) ((x) & 0x7)

#define MCUX_IMX_BIAS_PULL_UP_SHIFT 5
#define MCUX_IMX_PULL_ENABLE_SHIFT 4
#define MCUX_IMX_INPUT_SCHMITT_ENABLE_SHIFT 3
#define MCUX_IMX_SLEW_RATE_SHIFT 2
#define MCUX_IMX_DRIVE_STRENGTH_SHIFT 0
#define MCUX_IMX_INPUT_ENABLE_SHIFT 31 /* Shift to a bit not used by IOMUXC_SW_PAD_CTL */
#define MCUX_IMX_INPUT_ENABLE(x) ((x >> MCUX_IMX_INPUT_ENABLE_SHIFT) & 0x1)

#define Z_PINCTRL_MCUX_IMX_PINCFG_INIT(node_id)							\
	((DT_PROP(node_id, input_schmitt_enable) << MCUX_IMX_INPUT_SCHMITT_ENABLE_SHIFT) |	\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up), (DT_ENUM_IDX(node_id, bias_pull_up_value)	\
		<< MCUX_IMX_BIAS_PULL_UP_SHIFT) |)						\
	((DT_PROP(node_id, bias_pull_down) | DT_PROP(node_id, bias_pull_up))		\
		<< MCUX_IMX_PULL_ENABLE_SHIFT) |					\
	 (DT_ENUM_IDX(node_id, drive_strength) << MCUX_IMX_DRIVE_STRENGTH_SHIFT) |	\
	 (DT_ENUM_IDX(node_id, slew_rate) << MCUX_IMX_SLEW_RATE_SHIFT) |		\
	 (DT_PROP(node_id, input_enable) << MCUX_IMX_INPUT_ENABLE_SHIFT))


#define Z_PINCTRL_MCUX_IMX_LPSR_PINCFG_INIT(node_id)						\
	(IF_ENABLED(DT_PROP(node_id, bias_pull_up), (DT_ENUM_IDX(node_id, bias_pull_up_value)	\
		<< MCUX_IMX_BIAS_PULL_UP_SHIFT) |)						\
	IF_ENABLED(DT_PROP(node_id, bias_pull_down), (DT_ENUM_IDX(node_id, bias_pull_down_value)\
		<< MCUX_IMX_BIAS_PULL_DOWN_SHIFT) |)						\
	((DT_PROP(node_id, bias_pull_down) | DT_PROP(node_id, bias_pull_up))			\
		<< MCUX_IMX_PULL_ENABLE_SHIFT) |						\
	 (DT_PROP(node_id, input_enable) << MCUX_IMX_INPUT_ENABLE_SHIFT))


/* This struct must be present. It is used by the mcux gpio driver */
struct pinctrl_soc_pinmux {
	uint32_t mux_register; /*!< IOMUXC SW_PAD_MUX register */
	uint32_t config_register; /*!< IOMUXC SW_PAD_CTL register */
	uint32_t input_register; /*!< IOMUXC SELECT_INPUT DAISY register */
	uint8_t mux_mode: 4; /*!< Mux value for SW_PAD_MUX register */
	uint32_t input_daisy:4; /*!< Mux value for SELECT_INPUT_DAISY register */
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t pin_ctrl_flags; /*!< value to write to IOMUXC_SW_PAD_CTL register */
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
	}

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)				\
	MCUX_IMX_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PIN_INIT(group_id, pin_prop, idx)			\
	{									\
	  .pinmux = Z_PINCTRL_PINMUX(group_id, pin_prop, idx),			\
	  .pin_ctrl_flags = COND_CODE_0(					\
		DT_PROP(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx), pin_lpsr),	\
		(Z_PINCTRL_MCUX_IMX_PINCFG_INIT(group_id)),			\
		(Z_PINCTRL_MCUX_IMX_LPSR_PINCFG_INIT(group_id))),		\
	},


#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};	\


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_MCIMX6X_PINCTRL_SOC_H_ */
