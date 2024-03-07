/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_XTENSA_NXP_ADSP_IMX8_PINCTRL_SOC_H_
#define ZEPHYR_SOC_XTENSA_NXP_ADSP_IMX8_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pinmux {
	uint32_t pad;
	uint32_t mux;
};

typedef struct pinctrl_soc_pinmux pinctrl_soc_pin_t;

#define IMX8_PINMUX(n)					\
{							\
	.pad = DT_PROP_BY_IDX(n, pinmux, 0),            \
	.mux = DT_PROP_BY_IDX(n, pinmux, 1),            \
},

#define Z_PINCTRL_PINMUX(group_id, pin_prop, idx)\
	IMX8_PINMUX(DT_PHANDLE_BY_IDX(group_id, pin_prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)		\
	{ DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),     \
				 DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_PINMUX) };

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_XTENSA_NXP_ADSP_IMX8_PINCTRL_SOC_H_ */
