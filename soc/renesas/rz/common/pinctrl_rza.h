/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZA_H_
#define ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZA_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include "r_ioport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Porting */
typedef struct pinctrl_cfg_data_t {
	uint32_t reserved: 4;
	uint32_t pupd_reg: 6;
	uint32_t iolh_reg: 6;
	uint32_t pmc_reg: 1;
	uint32_t sr_reg: 1;
	uint32_t ien_reg: 1;
	uint32_t filonoff_reg: 1;
	uint32_t filnum_reg: 2;
	uint32_t filclksel_reg: 2;
	uint32_t pfc_reg: 3;
} pinctrl_cfg_data_t;

typedef struct pinctrl_soc_pin_t {
	bsp_io_port_pin_t port_pin;
	pinctrl_cfg_data_t config;
} pinctrl_soc_pin_t;

/* Iterate over each pinctrl-n phandle child */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	DT_FOREACH_CHILD(DT_PHANDLE_BY_IDX(node_id, state_prop, idx),                              \
			 Z_PINCTRL_STATE_PIN_CHILD_INIT)

/* Iterate over each pinctrl-n phandle child */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, Z_PINCTRL_STATE_PIN_INIT, ())};

#define Z_PINCTRL_STATE_PIN_CHILD_INIT(node_id)                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pinmux), \
		    (DT_FOREACH_PROP_ELEM(node_id, pinmux, Z_PINCTRL_PINMUX_INIT)), \
		    ())                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pins), \
		    (DT_FOREACH_PROP_ELEM(node_id, pins, Z_PINCTRL_SPECIAL_PINS_INIT)), \
		    ())

#define RZA_GET_PORT_PIN(pinmux) (pinmux & ~(0xF << 4))
#define RZA_GET_FUNC(pinmux)     ((pinmux & 0xF0) >> 4)

#define RZA_GET_PU_PD(node_id)                                                                     \
	DT_PROP(node_id, bias_pull_up) == 1 ? 1U : (DT_PROP(node_id, bias_pull_down) == 1 ? 2U : 0U)

#define RZA_GET_FILNUM(node_id) ((DT_PROP(node_id, renesas_filter) >> 2) & 0x3)

#define RZA_GET_FILCLKSEL(node_id) (DT_PROP(node_id, renesas_filter) & 0x3)

#define RZA_FILTER_ON_OFF(node_id) COND_CODE_0(DT_PROP(node_id, renesas_filter), (0), (1))

/* Process pinmux cfg */
#define Z_PINCTRL_PINMUX_INIT(node_id, state_prop, idx)                                            \
	{                                                                                          \
		.port_pin = RZA_GET_PORT_PIN(DT_PROP_BY_IDX(node_id, state_prop, idx)),            \
		.config =                                                                          \
			{                                                                          \
				.reserved = 0,                                                     \
				.pupd_reg = RZA_GET_PU_PD(node_id),                                \
				.iolh_reg = DT_PROP(node_id, drive_strength),                      \
				.pmc_reg = 1,                                                      \
				.sr_reg = DT_ENUM_IDX(node_id, slew_rate),                         \
				.ien_reg = DT_PROP(node_id, input_enable),                         \
				.filonoff_reg = RZA_FILTER_ON_OFF(node_id),                        \
				.filnum_reg = RZA_GET_FILNUM(node_id),                             \
				.filclksel_reg = RZA_GET_FILCLKSEL(node_id),                       \
				.pfc_reg = RZA_GET_FUNC(DT_PROP_BY_IDX(node_id, state_prop, idx)), \
			},                                                                         \
	},

#define Z_PINCTRL_SPECIAL_PINS_INIT(node_id, state_prop, idx)                                      \
	{                                                                                          \
		.port_pin = DT_PROP_BY_IDX(node_id, state_prop, idx),                              \
		.config =                                                                          \
			{                                                                          \
				.reserved = 0,                                                     \
				.pupd_reg = RZA_GET_PU_PD(node_id),                                \
				.iolh_reg = DT_PROP(node_id, drive_strength),                      \
				.pmc_reg = 0,                                                      \
				.sr_reg = DT_ENUM_IDX(node_id, slew_rate),                         \
				.ien_reg = DT_PROP(node_id, input_enable),                         \
				.filonoff_reg = RZA_FILTER_ON_OFF(node_id),                        \
				.filnum_reg = RZA_GET_FILNUM(node_id),                             \
				.filclksel_reg = RZA_GET_FILCLKSEL(node_id),                       \
				.pfc_reg = 0,                                                      \
			},                                                                         \
	},

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_SOC_RENESAS_RZ_COMMON_PINCTRL_RZA_H_ */
