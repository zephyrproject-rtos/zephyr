/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_ADI_MAX32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_ADI_MAX32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/max32-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc_pin {
	uint32_t pinmux;
	uint32_t pincfg;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_MAX32_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

#define MAX32_NO_PULL     0x0
#define MAX32_PULL_UP     0x1
#define MAX32_PULL_DOWN   0x2
#define MAX32_PUSH_PULL   0x0
#define MAX32_OPEN_DRAIN  0x1
#define MAX32_OUTPUT_LOW  0x0
#define MAX32_OUTPUT_HIGH 0x1

#define MAX32_INPUT_ENABLE_SHIFT   0x00
#define MAX32_BIAS_PULL_UP_SHIFT   0x01
#define MAX32_BIAS_PULL_DOWN_SHIFT 0x02
#define MAX32_OUTPUT_ENABLE_SHIFT  0x03
#define MAX32_POWER_SOURCE_SHIFT   0x04
#define MAX32_OUTPUT_HIGH_SHIFT    0x05

#define Z_PINCTRL_MAX32_PINCFG_INIT(node)                                                          \
	((DT_PROP_OR(node, input_enable, 0) << MAX32_INPUT_ENABLE_SHIFT) |                         \
	 (DT_PROP_OR(node, bias_pull_up, 0) << MAX32_BIAS_PULL_UP_SHIFT) |                         \
	 (DT_PROP_OR(node, bias_pull_down, 0) << MAX32_BIAS_PULL_DOWN_SHIFT) |                     \
	 (DT_PROP_OR(node, output_enable, 0) << MAX32_BIAS_PULL_DOWN_SHIFT) |                      \
	 (DT_PROP_OR(node, power_source, 0) << MAX32_POWER_SOURCE_SHIFT) |                         \
	 (DT_PROP_OR(node, output_high, 0) << MAX32_OUTPUT_HIGH_SHIFT))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	{.pinmux = Z_PINCTRL_MAX32_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx)),          \
	 .pincfg = Z_PINCTRL_MAX32_PINCFG_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_ADI_MAX32_COMMON_PINCTRL_SOC_H_ */
