/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MEDIATEK_MT8188_A55_PINCTRL_SOC_H_
#define ZEPHYR_SOC_MEDIATEK_MT8188_A55_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/mtk-common-pinctrl.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc {
	uint16_t pin;
	uint16_t func;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pin = MTK_GET_PIN_NO(DT_PROP_BY_IDX(node_id, prop, idx)),                         \
		.func = MTK_GET_PIN_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                      \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_MEDIATEK_MT8188_A55_PINCTRL_SOC_H_ */
