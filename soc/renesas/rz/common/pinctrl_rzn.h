/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZN_H_
#define ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZN_H_

#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include "r_ioport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RZN_GET_PORT_PIN(pinmux) (pinmux & ~(0xF << 4))
#define RZN_GET_FUNC(pinmux)     ((pinmux & 0xF0) >> 4)

/* Porting */
typedef struct pinctrl_cfg_data_t {
	uint32_t p_reg: 1;
	uint32_t pm_reg: 2;
	uint32_t pmc_reg: 1;
	uint32_t pfc_reg: 4;
	uint32_t drct_reg: 6;
	uint32_t rsel_reg: 1;
	uint32_t reserved: 17;
} pinctrl_cfg_data_t;

typedef struct pinctrl_soc_pin_t {
	bsp_io_port_pin_t port_pin;
	pinctrl_cfg_data_t config;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.port_pin = RZN_GET_PORT_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),                  \
		.config =                                                                          \
			{                                                                          \
				.p_reg = DT_PROP(node_id, output_high),                            \
				.pm_reg = DT_PROP(node_id, input_enable) == 1                      \
						  ? 1U                                             \
						  : (DT_PROP(node_id, output_enable) == 1 ? 2U     \
											  : 0U),   \
				.pmc_reg = 1,                                                      \
				.pfc_reg = RZN_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),       \
				.drct_reg =                                                        \
					(DT_ENUM_IDX(node_id, drive_strength)) |                   \
					((DT_PROP(node_id, bias_pull_up) == 1                      \
						  ? 1U                                             \
						  : (DT_PROP(node_id, bias_pull_down) == 1 ? 2U    \
											   : 0))   \
					 << 2) |                                                   \
					(DT_PROP(node_id, input_schmitt_enable) << 4) |            \
					(DT_ENUM_IDX(node_id, slew_rate) << 5),                    \
				.rsel_reg = 1,                                                     \
				.reserved = 0,                                                     \
			},                                                                         \
	},
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZN_H_ */
