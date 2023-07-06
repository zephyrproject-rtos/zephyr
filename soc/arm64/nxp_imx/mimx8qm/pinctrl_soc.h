/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM64_NXP_IMX8QM_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM64_NXP_IMX8QM_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMX_PINCTRL_INVALID_PIN_PROP 0xdeadbeef

struct pinctrl_soc_pinmux {
	uint32_t pad;
	uint32_t mux;
	uint32_t sw_config;
	uint32_t lp_config;
	uint32_t drive_strength;
	uint32_t pull_selection;
};

typedef struct pinctrl_soc_pinmux pinctrl_soc_pin_t;

#define PINMUX_PROP_IF_EXISTS(n, idx)				\
	COND_CODE_1(DT_PROP_HAS_IDX(n, pinmux, idx),		\
			(DT_PROP_BY_IDX(n, pinmux, idx)),	\
			(IMX_PINCTRL_INVALID_PIN_PROP))

#define IMX8QM_PINMUX(n)					\
	{							\
		.pad = DT_PROP_BY_IDX(n, pinmux, 0),		\
		.mux = DT_PROP_BY_IDX(n, pinmux, 1),		\
		.sw_config = DT_PROP_BY_IDX(n, pinmux, 2),	\
		.lp_config = DT_PROP_BY_IDX(n, pinmux, 3),	\
		.drive_strength = PINMUX_PROP_IF_EXISTS(n, 4),	\
		.pull_selection = PINMUX_PROP_IF_EXISTS(n, 5),	\
	},

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)\
	IMX8QM_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)		\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),	\
			DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_PINMUX)};

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_SOC_ARM64_NXP_IMX8QM_PINCTRL_SOC_H_ */
