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

#define MCHP_XEC_NO_PUD_POS		12
#define MCHP_XEC_PD_POS			13
#define MCHP_XEC_PU_POS			14
#define MCHP_XEC_PUSH_PULL_POS		15
#define MCHP_XEC_OPEN_DRAIN_POS		16
#define MCHP_XEC_OUT_DIS_POS		17
#define MCHP_XEC_OUT_EN_POS		18
#define MCHP_XEC_OUT_HI_POS		19
#define MCHP_XEC_OUT_LO_POS		20
/* bit[21] unused */
#define MCHP_XEC_SLEW_RATE_POS		22
#define MCHP_XEC_SLEW_RATE_MSK0		0x3
#define MCHP_XEC_SLEW_RATE_SLOW0	0x1
#define MCHP_XEC_SLEW_RATE_FAST0	0x2
#define MCHP_XEC_DRV_STR_POS		24
#define MCHP_XEC_DRV_STR_MSK0		0x7
#define MCHP_XEC_DRV_STR0_1X		0x1 /* 2 or 4(PIO-24) mA */
#define MCHP_XEC_DRV_STR0_2X		0x2 /* 4 or 8(PIO-24) mA */
#define MCHP_XEC_DRV_STR0_4X		0x3 /* 8 or 16(PIO-24) mA */
#define MCHP_XEC_DRV_STR0_6X		0x4 /* 12 or 24(PIO-24) mA */
#define MCHP_XEC_PIN_LOW_POWER_POS	27
#define MCHP_XEC_FUNC_INV_POS		29

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
