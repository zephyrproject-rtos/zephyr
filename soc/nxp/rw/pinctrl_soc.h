/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include "pinctrl_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_IOMUX_PINCFG(node_id)								\
	(IF_ENABLED(DT_PROP(node_id, bias_pull_down),						\
	(IOMUX_PAD_PULL(0x2) |)) /* pull down */						\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up),						\
	(IOMUX_PAD_PULL(0x1) |)) /* pull up */							\
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, sleep_output),					\
	(IOMUX_PAD_SLEEP_FORCE(DT_ENUM_IDX(node_id, sleep_output)) |)) /* force output */	\
	IOMUX_PAD_SLEW(DT_ENUM_IDX(node_id, slew_rate))) /* slew rate */

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)		\
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_IOMUX_PINCFG(group),


#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_SOC_H_ */
