/*
 * Copyright (c) 2022 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NUVOTON_NUMICRO_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NUVOTON_NUMICRO_COMMON_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/gpio/numicro-gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pin {
	uint32_t pinmux : 12;
	uint32_t pull_down : 1;
	uint32_t pull_up : 1;
	uint32_t open_drain : 1;
	uint32_t schmitt_trigger : 1;
	uint32_t slew_rate : 2;
	uint32_t input_disable : 1;
	uint32_t input_debounce : 1;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			\
	{								\
		.pinmux = DT_PROP_BY_IDX(node_id, prop, idx),		\
		.pull_down = DT_PROP(node_id, bias_pull_down),		\
		.pull_up = DT_PROP(node_id, bias_pull_up),		\
		.open_drain = DT_PROP(node_id, drive_open_drain),	\
		.schmitt_trigger = DT_PROP(node_id, input_schmitt_enable),\
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),		\
		.input_disable = DT_PROP(node_id, input_disable),	\
		.input_debounce = DT_PROP(node_id, input_debounce),	\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
				DT_FOREACH_PROP_ELEM, pinmux,		\
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NUVOTON_NUMICRO_COMMON_PINCTRL_SOC_H_ */
