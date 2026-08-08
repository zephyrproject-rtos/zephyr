/*
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_BEKEN_BK7258_PINCTRL_SOC_H_
#define ZEPHYR_SOC_BEKEN_BK7258_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/bk7258-pinctrl.h>
#include <zephyr/types.h>

typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(DT_PHANDLE(node_id, prop), pinmux, \
					 Z_PINCTRL_STATE_PIN_INIT, (,)) }

#endif /* ZEPHYR_SOC_BEKEN_BK7258_PINCTRL_SOC_H_ */
