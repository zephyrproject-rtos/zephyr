/*
 * Copyright (c) 2022-2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM64_NXP_IMX9_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM64_NXP_IMX9_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include "fsl_common.h"
#include "soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_MIMX9352_A55
#define MCUX_IMX_INPUT_SCHMITT_ENABLE_SHIFT IOMUXC1_SW_PAD_CTL_PAD_HYS_SHIFT
#define MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT IOMUXC1_SW_PAD_CTL_PAD_OD_SHIFT
#define MCUX_IMX_BIAS_PULL_DOWN_SHIFT IOMUXC1_SW_PAD_CTL_PAD_PD_SHIFT
#define MCUX_IMX_BIAS_PULL_UP_SHIFT IOMUXC1_SW_PAD_CTL_PAD_PU_SHIFT
#define MCUX_IMX_SLEW_RATE_SHIFT IOMUXC1_SW_PAD_CTL_PAD_FSEL1_SHIFT
#define MCUX_IMX_DRIVE_STRENGTH_SHIFT IOMUXC1_SW_PAD_CTL_PAD_DSE_SHIFT

#define IOMUXC_MUX_MODE(x) IOMUXC1_SW_MUX_CTL_PAD_MUX_MODE(x)
#define IOMUXC_SION(x) IOMUXC1_SW_MUX_CTL_PAD_SION(x)

#define IOMUXC_MUXREG   (IOMUXC1_BASE)
#define IOMUXC_CFGREG   (IOMUXC1_BASE + 0x204)
#define IOMUXC_DAISYREG (IOMUXC1_BASE + 0x408)
#elif defined(CONFIG_SOC_MIMX9596)
#define MCUX_IMX_INPUT_SCHMITT_ENABLE_SHIFT IOMUXC_SW_PAD_CTL_PAD_HYS_SHIFT
#define MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT IOMUXC_SW_PAD_CTL_PAD_OD_SHIFT
#define MCUX_IMX_BIAS_PULL_DOWN_SHIFT IOMUXC_SW_PAD_CTL_PAD_PD_SHIFT
#define MCUX_IMX_BIAS_PULL_UP_SHIFT IOMUXC_SW_PAD_CTL_PAD_PU_SHIFT
#define MCUX_IMX_SLEW_RATE_SHIFT IOMUXC_SW_PAD_CTL_PAD_FSEL1_SHIFT
#define MCUX_IMX_DRIVE_STRENGTH_SHIFT IOMUXC_SW_PAD_CTL_PAD_DSE_SHIFT

#define IOMUXC_MUX_MODE(x) IOMUXC_SW_MUX_CTL_PAD_MUX_MODE(x)
#define IOMUXC_SION(x) IOMUXC_SW_MUX_CTL_PAD_SION(x)

#define IOMUXC_MUXREG   (IOMUXC_BASE)
#define IOMUXC_CFGREG   (IOMUXC_BASE + 0x204)
#define IOMUXC_DAISYREG (IOMUXC_BASE + 0x408)

#endif

#define MCUX_IMX_INPUT_ENABLE_SHIFT 23 /* Shift to a bit not used by IOMUXC_SW_PAD_CTL */
#define MCUX_IMX_INPUT_ENABLE(x) ((x >> MCUX_IMX_INPUT_ENABLE_SHIFT) & 0x1)

#define Z_PINCTRL_MCUX_IMX_PINCFG_INIT(node_id)							\
	((DT_PROP(node_id, input_schmitt_enable) << MCUX_IMX_INPUT_SCHMITT_ENABLE_SHIFT) |	\
	 (DT_PROP(node_id, drive_open_drain) << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT) |		\
	 (DT_PROP(node_id, bias_pull_down) << MCUX_IMX_BIAS_PULL_DOWN_SHIFT) |			\
	 (DT_PROP(node_id, bias_pull_up) << MCUX_IMX_BIAS_PULL_UP_SHIFT) |			\
	 (DT_ENUM_IDX(node_id, slew_rate) << MCUX_IMX_SLEW_RATE_SHIFT) |			\
	 ((~(0xff << DT_ENUM_IDX(node_id, drive_strength))) << MCUX_IMX_DRIVE_STRENGTH_SHIFT) |	\
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
	  .pin_ctrl_flags = Z_PINCTRL_MCUX_IMX_PINCFG_INIT(group_id),		\
	},


#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),			\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};	\


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM64_NXP_IMX9_PINCTRL_SOC_H_ */
