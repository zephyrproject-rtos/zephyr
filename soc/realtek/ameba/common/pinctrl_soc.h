/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_AMEBA_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_REALTEK_AMEBA_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pin {
	uint32_t pinmux: 15;
	/* bit[14:13] port
	 * bit[12:8] pin
	 * bit[7:0] function ID
	 */
	uint32_t pull_down: 1;
	uint32_t pull_up: 1;
	uint32_t schmitt_disable: 1;
	uint32_t slew_rate_slow: 1;
	uint32_t drive_strength_low: 1;
	uint32_t digital_input_disable: 1;
	uint32_t swd_off: 1;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.pinmux = DT_PROP_BY_IDX(node_id, prop, idx),                                      \
		.pull_down = DT_PROP(node_id, bias_pull_down),                                     \
		.pull_up = DT_PROP(node_id, bias_pull_up),                                         \
		.schmitt_disable = DT_PROP(node_id, input_schmitt_disable),                        \
		.slew_rate_slow = DT_PROP(node_id, slew_rate_slow),                                \
		.drive_strength_low = DT_PROP(node_id, drive_strength_low),                        \
		.digital_input_disable = DT_PROP(node_id, digital_input_disable),                  \
		.swd_off = DT_PROP(node_id, swd_off),                                              \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_REALTEK_AMEBA_COMMON_PINCTRL_SOC_H_ */
