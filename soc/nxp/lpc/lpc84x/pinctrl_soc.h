/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <errno.h>

static inline int lpc84x_iocon_index(uint8_t port, uint8_t pin)
{
	static const uint8_t iocon_idx_p0[32] = {
		17, 11, 6,  5,  4,  3,  16, 15, /* P0.0  – P0.7  */
		14, 13, 8,  7,  2,  1,  18, 10, /* P0.8  – P0.15 */
		9,  0,  30, 29, 28, 27, 26, 25, /* P0.16 – P0.23 */
		24, 23, 22, 21, 20, 50, 51, 35, /* P0.24 – P0.31 */
	};

	static const uint8_t iocon_idx_p1[22] = {
		36, 37, 38, 41, 42, 43, 46, 49, /* P1.0  – P1.7  */
		31, 32, 55, 54, 33, 34, 39, 40, /* P1.8  – P1.15 */
		44, 45, 47, 48, 52, 53,         /* P1.16 – P1.21 */
	};

	if (port == 0 && pin < 32) {
		return iocon_idx_p0[pin];
	} else if (port == 1 && pin < 22) {
		return iocon_idx_p1[pin];
	}

	return -EINVAL;
}

typedef struct {
	uint16_t swm_cfg;
	uint16_t iocon_cfg;
} pinctrl_soc_pin_t;

/* SWM decoding macros */
#define LPC84X_SWM_PIN(cfg)      ((cfg) & 0xFF)
#define LPC84X_SWM_FUNC(cfg)     (((cfg) >> 8) & 0x7F)
#define LPC84X_SWM_IS_FIXED(cfg) (((cfg) & (1U << 15)) != 0)
#define LPC84X_SWM_NONE          0xFFFF

/* IOCON decoding macros */
#define LPC84X_IOCON_INDEX(cfg) ((cfg) & 0xFF)
#define LPC84X_IOCON_CFG(cfg)   (((cfg) >> 8) & 0xFF)

/**
 * @brief Build IOCON configuration bits for storage in pinctrl_soc_pin_t.
 * We store the IOCON register bits 3..10 directly, shifted right by 3.
 * This includes: Bias (MODE), Hysteresis (HYS), Invert (INV), and Open-drain (OD).
 */

#define Z_PINCTRL_IOCON_BIAS(node_id)                                                              \
	((DT_PROP(node_id, bias_pull_down) ? 1U : 0U) | (DT_PROP(node_id, bias_pull_up) ? 2U : 0U))

#define Z_PINCTRL_IOCON_PINCFG(node_id)                                                            \
	((IOCON_PIO_MODE(Z_PINCTRL_IOCON_BIAS(node_id)) |                                          \
	  IOCON_PIO_HYS(DT_PROP(node_id, nxp_hysteresis)) |                                        \
	  IOCON_PIO_INV(DT_PROP(node_id, nxp_invert)) |                                            \
	  IOCON_PIO_OD(DT_PROP(node_id, drive_open_drain))) >>                                     \
	 3)

/**
 * @brief Utility macro to initialize a pinmux + configuration.
 */
#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	{                                                                                          \
		.swm_cfg = (DT_PROP_BY_IDX(group, pin_prop, idx) >> 8) & 0xFFFF,                   \
		.iocon_cfg = (DT_PROP_BY_IDX(group, pin_prop, idx) & 0xFF) |                       \
			     (Z_PINCTRL_IOCON_PINCFG(group) << 8),                                 \
	},

/**
 * @brief Utility macro to initialize state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_ARM_NXP_LPC_LPC84X_PINCTRL_SOC_H_ */
