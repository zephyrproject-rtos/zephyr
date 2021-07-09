/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B91_PINCTRL_COMMON_H_
#define ZEPHYR_B91_PINCTRL_COMMON_H_

/* IDs for GPIO functions */

#define B91_FUNC_A       0x000000
#define B91_FUNC_B       0x010000
#define B91_FUNC_C       0x020000

/* IDs for GPIO Ports  */

#define B91_PORT_A       0x0000
#define B91_PORT_B       0x0100
#define B91_PORT_C       0x0200
#define B91_PORT_D       0x0300
#define B91_PORT_E       0x0400

/* IDs for GPIO Pins */

#define B91_PIN_0        0x01
#define B91_PIN_1        0x02
#define B91_PIN_2        0x04
#define B91_PIN_3        0x08
#define B91_PIN_4        0x10
#define B91_PIN_5        0x20
#define B91_PIN_6        0x40
#define B91_PIN_7        0x80

/* Setters and getters */

#define B91_PINMUX_SET(func, port, pin)   (func | port | pin)
#define B91_PINMUX_GET_FUNC(pinmux)       ((pinmux >> 16) & 0xFF)
#define B91_PINMUX_GET_PIN(pinmux)        (pinmux & 0xFFFF)

#define B91_PINMUX_DT_INST_GET_ELEM(idx, x, inst) \
	DT_PROP_BY_PHANDLE_IDX(DT_DRV_INST(inst), pinctrl_##x, idx, pinmux),

#define B91_PINMUX_DT_INST_GET_ARRAY(inst, x)				 \
	{ COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_##x),		 \
		      (UTIL_LISTIFY(DT_INST_PROP_LEN(inst, pinctrl_##x), \
				    B91_PINMUX_DT_INST_GET_ELEM,	 \
				    x,					 \
				    inst)),				 \
		      ())						 \
	}

#endif  /* ZEPHYR_B91_PINCTRL_COMMON_H_ */
