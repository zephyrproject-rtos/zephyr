/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Microchip XEC SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Type for MCHP XEC pin. */
struct mec_pinctrl {
	uint32_t pinmux		: 12; /* b[11:0] */
	uint32_t no_pud		: 1; /* b[12] */
	uint32_t pd		: 1; /* b[13] */
	uint32_t pu		: 1; /* b[14] */
	uint32_t obuf_pp	: 1; /* b[15] */
	uint32_t obuf_od	: 1; /* b[16] */
	uint32_t out_dis	: 1; /* b[17] */
	uint32_t out_en		: 1; /* b[18] */
	uint32_t out_hi		: 1; /* b[19] */
	uint32_t out_lo		: 1; /* b[20] */
	uint32_t rsvd21		: 1; /* b[21] unused */
	uint32_t slew_rate	: 2; /* b[23:22] */
	uint32_t drive_str	: 3; /* b[26:24] */
	uint32_t rsvd27		: 1; /* b[27] unused */
	uint32_t finv		: 1; /* b[28] */
	uint32_t lp		: 1; /* b[29] */
};

typedef struct mec_pinctrl pinctrl_soc_pin_t;

#define MCHP_XEC_PIN_INIT(node_id)                                                                 \
	{                                                                                          \
		.pinmux = DT_PROP(node_id, pinmux),                                                \
		.no_pud = DT_PROP(node_id, bias_disable),                                          \
		.pd = DT_PROP(node_id, bias_pull_down),                                            \
		.pu = DT_PROP(node_id, bias_pull_up),                                              \
		.obuf_pp = DT_PROP(node_id, drive_push_pull),                                      \
		.obuf_od = DT_PROP(node_id, drive_open_drain),                                     \
		.out_dis = DT_PROP(node_id, output_disable),                                       \
		.out_en = DT_PROP(node_id, output_enable),                                         \
		.out_hi = DT_PROP(node_id, output_high),                                           \
		.out_lo = DT_PROP(node_id, output_low),                                            \
		.slew_rate = DT_PROP_OR(node_id, slew_rate, 0),                                    \
		.drive_str = DT_PROP_OR(node_id, drive_strength, 0),                               \
		.finv = DT_PROP(node_id, microchip_output_func_invert),                            \
		.lp = DT_PROP(node_id, low_power_enable),                                          \
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) \
	MCHP_XEC_PIN_INIT(DT_PROP_BY_IDX(node_id, prop, idx))

/* Use DT FOREACH macro to initialize each used pin */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_MICROCHIP_XEC_COMMON_PINCTRL_SOC_H_ */
