/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC13X4_CC26X4_SOC_PINCTRL_H_
#define TI_SIMPLELINK_CC13X4_CC26X4_SOC_PINCTRL_H_

#include <zephyr/types.h>
#include <zephyr/devicetree.h>

/* Input / Hysteresis */
#define IOC_INPUT_ENABLE 0x20000000U /* bit 29 */
#define IOC_HYST_ENABLE  0x40000000U /* bit 30 */

/* IOMODE field bits[26:24] */
#define IOC_IOMODE_OPEN_DRAIN_NORMAL 0x04000000U
#define IOC_IOMODE_OPEN_SRC_NORMAL   0x06000000U

/* Pull field bits[15:14] — CC13x4 specific */
#define IOC_NO_IOPULL   0x0000C000U
#define IOC_IOPULL_UP   0x00008000U
#define IOC_IOPULL_DOWN 0x00004000U

/* Drive strength: IOCURR field at bits[12:11] */
#define IOC_IOCFG_IOCURR_S 11U

/* Edge detect: EDGE_DET field at bits[17:16] */
#define IOC_EDGE_DET_S 16U

typedef struct pinctrl_soc_pin {
	uint32_t pin;
	uint32_t iofunc;
	uint32_t iomode;
} pinctrl_soc_pin_t;

#define CC13X4_CC26X4_PIN_FLAGS(node_id)                                                           \
	(DT_PROP(node_id, bias_pull_up) * IOC_IOPULL_UP |                                          \
	 DT_PROP(node_id, bias_pull_down) * IOC_IOPULL_DOWN |                                      \
	 DT_PROP(node_id, bias_disable) * IOC_NO_IOPULL |                                          \
	 DT_PROP(node_id, drive_open_drain) * IOC_IOMODE_OPEN_DRAIN_NORMAL |                       \
	 DT_PROP(node_id, drive_open_source) * IOC_IOMODE_OPEN_SRC_NORMAL |                        \
	 ((DT_PROP(node_id, drive_strength) >> 2) & 0x3U) << IOC_IOCFG_IOCURR_S |                  \
	 DT_PROP(node_id, input_enable) * IOC_INPUT_ENABLE |                                       \
	 DT_PROP(node_id, input_schmitt_enable) * IOC_HYST_ENABLE |                                \
	 (DT_PROP(node_id, ti_input_edge_detect) << IOC_EDGE_DET_S))

#define CC13X4_CC26X4_DT_PIN(node_id)                                                              \
	{.pin = DT_PROP_BY_IDX(node_id, pinmux, 0),                                                \
	 .iofunc = DT_PROP_BY_IDX(node_id, pinmux, 1),                                             \
	 .iomode = CC13X4_CC26X4_PIN_FLAGS(node_id)},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	CC13X4_CC26X4_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* TI_SIMPLELINK_CC13X4_CC26X4_SOC_PINCTRL_H_ */
