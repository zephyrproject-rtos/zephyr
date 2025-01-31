/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#ifndef GANYMED_SY1XX_PINCTRL_SOC_H
#define GANYMED_SY1XX_PINCTRL_SOC_H

#include <stdint.h>

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SY1XX_SCHMITT_ENABLE   1U
#define SY1XX_PULL_UP_ENABLE   1U
#define SY1XX_PULL_DOWN_ENABLE 1U
#define SY1XX_TRISTATE_ENABLE  1U
#define SY1XX_OUTPUT_ENABLE    1U

#define SY1XX_PAD_SCHMITT_OFFS   7
#define SY1XX_PAD_PULL_UP_OFFS   5
#define SY1XX_PAD_PULL_DOWN_OFFS 4
#define SY1XX_PAD_DRIVE_OFFS     2
#define SY1XX_PAD_TRISTATE_OFFS  1
#define SY1XX_PAD_DIR_OFFS       0

/** Type for SY1XX pin. */
typedef struct {
	/** address of pin config register */
	uint32_t addr;
	/** intra register offset (8bit cfg per pin) */
	uint32_t iro;
	/** config for pin (8bit), describes pull-up/down, ... */
	uint32_t cfg;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_CFG(node)                                                                        \
	(                                                                                          \
                                                                                                   \
		(SY1XX_SCHMITT_ENABLE * DT_PROP(node, input_schmitt_enable))                       \
			<< SY1XX_PAD_SCHMITT_OFFS |                                                \
		(SY1XX_PULL_UP_ENABLE * DT_PROP(node, bias_pull_up)) << SY1XX_PAD_PULL_UP_OFFS |   \
		(SY1XX_PULL_DOWN_ENABLE * DT_PROP(node, bias_pull_down))                           \
			<< SY1XX_PAD_PULL_DOWN_OFFS |                                              \
		(SY1XX_TRISTATE_ENABLE * DT_PROP(node, bias_high_impedance))                       \
			<< SY1XX_PAD_TRISTATE_OFFS |                                               \
		(SY1XX_OUTPUT_ENABLE & (1 - DT_PROP(node, input_enable))) << SY1XX_PAD_DIR_OFFS    \
                                                                                                   \
	)

#define Z_PINCTRL_STATE_PIN_INIT(node, pr, idx)                                                    \
	{                                                                                          \
                                                                                                   \
		.addr = DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pr, idx), pinmux, 0),               \
		.iro = DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pr, idx), pinmux, 1),                \
		.cfg = Z_PINCTRL_CFG(DT_PHANDLE_BY_IDX(node, pr, idx))                             \
                                                                                                   \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

#ifdef __cplusplus
}
#endif

#endif /* GANYMED_SY1XX_PINCTRL_SOC_H */
