/*
 * Copyright (c) 2024-2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RA_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RENESAS_RA_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef CONFIG_PINCTRL_RENESAS_RA0_PFS
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-ra0.h>
#else
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-ra.h>
#endif /* CONFIG_PINCTRL_RENESAS_RA0_PFS */

#define RA_PINCTRL_PORT_NUM ARRAY_SIZE(((R_PFS_Type *)0)->PORT)
#define RA_PINCTRL_PIN_NUM  ARRAY_SIZE(((R_PFS_PORT_Type *)0)->PIN)
/**
 * @brief Type to hold a renesas ra pin's pinctrl configuration.
 */
struct ra_pinctrl_soc_pin {
	/** Port number 0..9, A, B */
	uint32_t port_num: 4;
	/** Pin number 0..15 */
	uint32_t pin_num: 4;
	/** Register PFS cfg */
	uint32_t cfg;
};

typedef struct ra_pinctrl_soc_pin pinctrl_soc_pin_t;

#ifdef CONFIG_PINCTRL_RENESAS_RA0_PFS
#define RENESAS_RA_PINTCTRL_CFG_GET(node_id, prop, idx)                                            \
	(DT_PROP(node_id, output_high) << R_PFS_PORT_PIN_PmnPFS_PODR_Pos) |                        \
		(DT_PROP(node_id, output_enable) << R_PFS_PORT_PIN_PmnPFS_PDR_Pos) |               \
		(DT_PROP(node_id, ttl_input_buffer) << R_PFS_PORT_PIN_PmnPFS_PIM_Pos) |            \
		(DT_PROP(node_id, analog_enable) << R_PFS_PORT_PIN_PmnPFS_PMC_Pos)
#else
#define RENESAS_RA_PINTCTRL_CFG_GET(node_id, prop, idx)                                            \
	(DT_PROP(node_id, renesas_analog_enable) << R_PFS_PORT_PIN_PmnPFS_ASEL_Pos) |              \
		(DT_ENUM_IDX(node_id, drive_strength) << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos) |         \
		(RA_GET_MODE(DT_PROP_BY_IDX(node_id, prop, idx)) << R_PFS_PORT_PIN_PmnPFS_PMR_Pos)
#endif

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.port_num = RA_GET_PORT_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                   \
		.pin_num = RA_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                     \
		.cfg = (DT_PROP(node_id, bias_pull_up) << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) |         \
		       (DT_PROP(node_id, drive_open_drain) << R_PFS_PORT_PIN_PmnPFS_NCODR_Pos) |   \
		       (RA_GET_PSEL(DT_PROP_BY_IDX(node_id, prop, idx))                            \
			<< R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |                                       \
		       RENESAS_RA_PINTCTRL_CFG_GET(node_id, prop, idx),                            \
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, psels,            \
				Z_PINCTRL_STATE_PIN_INIT)}

#define RA_GET_PORT_NUM(pinctrl) (((pinctrl) >> RA_PORT_NUM_POS) & RA_PORT_NUM_MASK)
#define RA_GET_PIN_NUM(pinctrl)  (((pinctrl) >> RA_PIN_NUM_POS) & RA_PIN_NUM_MASK)

#define RA_GET_MODE(pinctrl) (((pinctrl) >> RA_MODE_POS) & RA_MODE_MASK)
#define RA_GET_PSEL(pinctrl) (((pinctrl) >> RA_PSEL_POS) & RA_PSEL_MASK)

#endif /* ZEPHYR_SOC_RENESAS_RA_COMMON_PINCTRL_SOC_H_ */
