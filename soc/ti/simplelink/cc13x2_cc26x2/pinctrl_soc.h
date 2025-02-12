/*
 * Copyright (c) 2022 Vaishnav Achath
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC13XX_CC26XX_SOC_PINCTRL_H_
#define TI_SIMPLELINK_CC13XX_CC26XX_SOC_PINCTRL_H_

#include <zephyr/types.h>

#include <driverlib/ioc.h>

typedef struct pinctrl_soc_pin {
	uint32_t pin;
	uint32_t iofunc;
	uint32_t iomode;
} pinctrl_soc_pin_t;

/* Convert DT flags to SoC flags */
#define CC13XX_CC26XX_PIN_FLAGS(node_id) \
	(DT_PROP(node_id, bias_pull_up) * IOC_IOPULL_UP | \
	DT_PROP(node_id, bias_pull_down) * IOC_IOPULL_DOWN | \
	DT_PROP(node_id, bias_disable) * IOC_NO_IOPULL |  \
	DT_PROP(node_id, drive_open_drain) * IOC_IOMODE_OPEN_DRAIN_NORMAL | \
	DT_PROP(node_id, drive_open_source) * IOC_IOMODE_OPEN_SRC_NORMAL | \
	(DT_PROP(node_id, drive_strength) >> 2) << IOC_IOCFG0_IOCURR_S | \
	DT_PROP(node_id, input_enable) * IOC_INPUT_ENABLE | \
	DT_PROP(node_id, input_schmitt_enable) * IOC_HYST_ENABLE | \
	DT_PROP(node_id, ti_input_edge_detect))

#define CC13XX_CC26XX_DT_PIN(node_id)					\
	{							\
		.pin = DT_PROP_BY_IDX(node_id, pinmux, 0),	\
		.iofunc = DT_PROP_BY_IDX(node_id, pinmux, 1),	\
		.iomode = CC13XX_CC26XX_PIN_FLAGS(node_id)	\
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)		\
	CC13XX_CC26XX_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)		\
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif /* TI_SIMPLELINK_CC13XX_CC26XX_SOC_PINCTRL_H_ */
