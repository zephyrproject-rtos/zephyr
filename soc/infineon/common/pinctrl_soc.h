/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon IOSS pin controller SoC definitions.
 */

#ifndef ZEPHYR_SOC_INFINEON_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_INFINEON_COMMON_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#define IFX_IOSS_PINMUX_PORT_MSK	GENMASK(7, 0)
#define IFX_IOSS_PINMUX_PIN_MSK		GENMASK(15, 8)
#define IFX_IOSS_PINMUX_HSIOM_MSK	GENMASK(23, 16)

#define IFX_IOSS_PINMUX_PORT(x)		FIELD_GET(IFX_IOSS_PINMUX_PORT_MSK, x)
#define IFX_IOSS_PINMUX_PIN(x)		FIELD_GET(IFX_IOSS_PINMUX_PIN_MSK, x)
#define IFX_IOSS_PINMUX_HSIOM(x)	FIELD_GET(IFX_IOSS_PINMUX_HSIOM_MSK, x)

#define CAT1_PINMUX_GET_PORT_NUM(x)	IFX_IOSS_PINMUX_PORT(x)
#define CAT1_PINMUX_GET_PIN_NUM(x)	IFX_IOSS_PINMUX_PIN(x)

#define IFX_IOSS_DM_HIGHZ		0U
#define IFX_IOSS_DM_PULLUP		2U
#define IFX_IOSS_DM_PULLDOWN		3U
#define IFX_IOSS_DM_OD_DRIVESLOW	4U
#define IFX_IOSS_DM_OD_DRIVESHIGH	5U
#define IFX_IOSS_DM_STRONG		6U
#define IFX_IOSS_DM_PULLUP_DOWN		7U

#define IFX_IOSS_PINCFG_DM_MSK		GENMASK(2, 0)
#define IFX_IOSS_PINCFG_IN_EN		BIT(3)

#define IFX_IOSS_PINCFG_SLEW_EN		BIT(4)
#define IFX_IOSS_PINCFG_SLEW_SLOW	BIT(5)
#define IFX_IOSS_PINCFG_DRIVE_EN	BIT(6)
#define IFX_IOSS_PINCFG_DRIVE_MSK	GENMASK(9, 7)

#define IFX_IOSS_DRIVE_FULL		0U
#define IFX_IOSS_DRIVE_SEVEN_EIGHTHS	1U
#define IFX_IOSS_DRIVE_THREE_FOURTHS	2U
#define IFX_IOSS_DRIVE_FIVE_EIGHTHS	3U
#define IFX_IOSS_DRIVE_HALF		4U
#define IFX_IOSS_DRIVE_THREE_EIGHTHS	5U
#define IFX_IOSS_DRIVE_ONE_FOURTH	6U
#define IFX_IOSS_DRIVE_ONE_EIGHTH	7U

#define IFX_IOSS_PINCFG_DM(x)		FIELD_GET(IFX_IOSS_PINCFG_DM_MSK, x)
#define IFX_IOSS_PINCFG_DRIVE(x)	FIELD_GET(IFX_IOSS_PINCFG_DRIVE_MSK, x)

typedef struct {
	uint32_t pinmux;
	uint32_t pincfg;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_IFX_IOSS_DM(node_id)                                         \
	(DT_PROP(node_id, drive_open_drain)  ? IFX_IOSS_DM_OD_DRIVESLOW :      \
	 DT_PROP(node_id, drive_open_source) ? IFX_IOSS_DM_OD_DRIVESHIGH :     \
	 DT_PROP(node_id, drive_push_pull)   ? IFX_IOSS_DM_STRONG :            \
	 (DT_PROP(node_id, bias_pull_up) &&                                    \
	  DT_PROP(node_id, bias_pull_down))  ? IFX_IOSS_DM_PULLUP_DOWN :       \
	 DT_PROP(node_id, bias_pull_up)      ? IFX_IOSS_DM_PULLUP :            \
	 DT_PROP(node_id, bias_pull_down)    ? IFX_IOSS_DM_PULLDOWN :          \
					       IFX_IOSS_DM_HIGHZ)

#define Z_PINCTRL_IFX_IOSS_SLEW(node_id)                                       \
	(DT_NODE_HAS_PROP(node_id, slew_rate)                                  \
	 ? (IFX_IOSS_PINCFG_SLEW_EN |                                          \
	    FIELD_PREP(IFX_IOSS_PINCFG_SLEW_SLOW,                              \
		       DT_ENUM_IDX_OR(node_id, slew_rate, 0)))                 \
	 : 0U)

#if defined(IFX_IOSS_DRIVE_DEFAULT)
#define Z_PINCTRL_IFX_IOSS_DRIVE(node_id)                                      \
	(IFX_IOSS_PINCFG_DRIVE_EN |                                            \
	 FIELD_PREP(IFX_IOSS_PINCFG_DRIVE_MSK,                                 \
		    DT_ENUM_IDX_OR(node_id, drive_strength,                    \
				   IFX_IOSS_DRIVE_DEFAULT)))
#else
#define Z_PINCTRL_IFX_IOSS_DRIVE(node_id)                                      \
	(DT_NODE_HAS_PROP(node_id, drive_strength)                             \
	 ? (IFX_IOSS_PINCFG_DRIVE_EN |                                         \
	    FIELD_PREP(IFX_IOSS_PINCFG_DRIVE_MSK,                              \
		       DT_ENUM_IDX_OR(node_id, drive_strength, 0)))            \
	 : 0U)
#endif /* IFX_IOSS_DRIVE_DEFAULT */

#define Z_PINCTRL_IFX_IOSS_PINCFG(node_id)                                     \
	(Z_PINCTRL_IFX_IOSS_DM(node_id) |                                      \
	 (DT_PROP(node_id, input_enable) ? IFX_IOSS_PINCFG_IN_EN : 0U) |       \
	 Z_PINCTRL_IFX_IOSS_SLEW(node_id) |                                    \
	 Z_PINCTRL_IFX_IOSS_DRIVE(node_id))

#define Z_PINCTRL_IFX_IOSS_PIN_INIT(node_id)                                   \
	{.pinmux = DT_PROP(node_id, pinmux),                                   \
	 .pincfg = Z_PINCTRL_IFX_IOSS_PINCFG(node_id)},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                     \
	Z_PINCTRL_IFX_IOSS_PIN_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                               \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_COMMON_PINCTRL_SOC_H_ */
