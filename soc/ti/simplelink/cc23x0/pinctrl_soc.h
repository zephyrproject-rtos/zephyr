/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC23X0_SOC_PINCTRL_H_
#define TI_SIMPLELINK_CC23X0_SOC_PINCTRL_H_

#include <zephyr/types.h>

/* IO enabling/disabling */
#define IOC_INPUT_ENABLE  BIT(29)
#define IOC_INPUT_DISABLE 0U
#define IOC_HYST_ENABLE   BIT(30)
#define IOC_HYST_DISABLE  0U

/* IO modes */
#define IOC_IOMODE_NORMAL            0U
#define IOC_IOMODE_INV               BIT(24)
#define IOC_IOMODE_OPEN_DRAIN_NORMAL BIT(25)
#define IOC_IOMODE_OPEN_DRAIN_INV    GENMASK(25, 24)
#define IOC_IOMODE_OPEN_SRC_NORMAL   BIT(26)
#define IOC_IOMODE_OPEN_SRC_INV      (BIT(26) | BIT(24))

/* IO pull */
#define IOC_NO_IOPULL   0U
#define IOC_IOPULL_UP   BIT(14)
#define IOC_IOPULL_DOWN BIT(13)

typedef struct pinctrl_soc_pin {
	uint32_t pin;
	uint32_t iofunc;
	uint32_t iomode;
} pinctrl_soc_pin_t;

/* Convert DT flags to SoC flags */
#define CC23X0_PIN_FLAGS(node_id)                                                                  \
	(DT_PROP(node_id, bias_pull_up) * IOC_IOPULL_UP |                                          \
	 DT_PROP(node_id, bias_pull_down) * IOC_IOPULL_DOWN |                                      \
	 DT_PROP(node_id, bias_disable) * IOC_NO_IOPULL |                                          \
	 DT_PROP(node_id, drive_open_drain) * IOC_IOMODE_OPEN_DRAIN_NORMAL |                       \
	 DT_PROP(node_id, drive_open_source) * IOC_IOMODE_OPEN_SRC_NORMAL |                        \
	 DT_PROP(node_id, input_enable) * IOC_INPUT_ENABLE |                                       \
	 DT_PROP(node_id, input_schmitt_enable) * IOC_HYST_ENABLE)

#define CC23X0_DT_PIN(node_id)                                                                     \
	{.pin = DT_PROP_BY_IDX(node_id, pinmux, 0),                                                \
	 .iofunc = DT_PROP_BY_IDX(node_id, pinmux, 1),                                             \
	 .iomode = CC23X0_PIN_FLAGS(node_id)},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	CC23X0_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* TI_SIMPLELINK_CC23X0_SOC_PINCTRL_H_ */
