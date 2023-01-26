/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>

#define MCHP_GPIO	0x0
#define MCHP_AF0	0x0
#define MCHP_AF1	0x1
#define MCHP_AF2	0x2
#define MCHP_AF3	0x3
#define MCHP_AF4	0x4
#define MCHP_AF5	0x5
#define MCHP_AF6	0x6
#define MCHP_AF7	0x7
#define MCHP_AFMAX	0x8

#define MCHP_XEC_PUPDR_POS		0
#define MCHP_XEC_PUPDR_MASK		0x3
#define MCHP_XEC_FUNC_INV_POS		2
#define MCHP_XEC_FUNC_INV_MSK		0x4
#define MCHP_XEC_OTYPER_POS		4
#define MCHP_XEC_OTYPER_MASK		0x1
#define MCHP_XEC_OTYPER_PUSHPULL	0
#define MCHP_XEC_OTYPER_OPENDRAIN	0x1
#define MCHP_XEC_OVAL_POS		6
#define MCHP_XEC_OVAL_MASK		0x3
#define MCHP_XEC_OVAL_DRV_LOW		0x1
#define MCHP_XEC_OVAL_DRV_HIGH		0x2
#define MCHP_XEC_OSPEEDR_POS		8
#define MCHP_XEC_OSPEEDR_MASK		0x3
#define MCHP_XEC_OSPEEDR_SLOW		0x0
#define MCHP_XEC_OSPEEDR_FAST		0x1
#define MCHP_XEC_OSPEEDR_NO_CHG		0x3 /* Do not modify slew rate */
#define MCHP_XEC_ODRVSTR_POS		12
#define MCHP_XEC_ODRVSTR_MASK		0x7
#define MCHP_XEC_ODRVSTR_1X		0x0 /* 2 or 4(PIO-24) mA */
#define MCHP_XEC_ODRVSTR_2X		0x1 /* 4 or 8(PIO-24) mA */
#define MCHP_XEC_ODRVSTR_4X		0x2 /* 8 or 16(PIO-24) mA */
#define MCHP_XEC_ODRVSTR_6X		0x3 /* 12 or 24(PIO-24) mA */
#define MCHP_XEC_ODRVSTR_NO_CHG		0x7 /* Do not modify drive strength */
#define MCHP_XEC_PIN_LOW_POWER_POS	15
#define MCHP_XEC_PIN_LOW_POWER		1

#define MCHP_XEC_PINMUX_PORT_POS	0
#define MCHP_XEC_PINMUX_PORT_MSK	0xf
#define MCHP_XEC_PINMUX_PIN_POS		4
#define MCHP_XEC_PINMUX_PIN_MSK		0x1f
#define MCHP_XEC_PINMUX_FUNC_POS	9
#define MCHP_XEC_PINMUX_FUNC_MSK	0x7

/* n is octal pin number or equivalent in another base.
 * MCHP XEC documentation specifies pin numbers in octal.
 * f is function number
 * b[3:0] = pin bank
 * b[8:4] = pin position in bank
 * b[11:9] = function
 */
#define MCHP_XEC_PINMUX(n, f)							  \
	(((((n) >> 5) & MCHP_XEC_PINMUX_PORT_MSK) << MCHP_XEC_PINMUX_PORT_POS)	| \
	 (((n) & MCHP_XEC_PINMUX_PIN_MSK) << MCHP_XEC_PINMUX_PIN_POS)		| \
	 (((f) & MCHP_XEC_PINMUX_FUNC_MSK) << MCHP_XEC_PINMUX_FUNC_POS))


#define MCHP_XEC_PINMUX_PORT(p)						\
	(((p) >> MCHP_XEC_PINMUX_PORT_POS) & MCHP_XEC_PINMUX_PORT_MSK)

#define MCHP_XEC_PINMUX_PIN(p)						\
	(((p) >> MCHP_XEC_PINMUX_PIN_POS) & MCHP_XEC_PINMUX_PIN_MSK)

#define MCHP_XEC_PINMUX_FUNC(p)						\
	(((p) >> MCHP_XEC_PINMUX_FUNC_POS) & MCHP_XEC_PINMUX_FUNC_MSK)

#define MEC_XEC_PINMUX_PORT_PIN(p)					\
	((p) & ((MCHP_XEC_PINMUX_PORT_MSK << MCHP_XEC_PINMUX_PORT_POS) | \
		(MCHP_XEC_PINMUX_PIN_MSK << MCHP_XEC_PINMUX_PIN_POS)))

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_ */
