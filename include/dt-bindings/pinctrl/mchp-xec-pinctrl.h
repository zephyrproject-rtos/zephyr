/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_

#include <dt-bindings/pinctrl/mchp-xec-pinctrl-common.h>

/* Adapted from Linux: include/dt-bindings/pinctrl/stm32-pinfunc.h */

/**
 * @brief Pin modes
 */

/*
 * Microchip XEC GPIO pins implement up to 8 functions.
 * Function 0 is GPIO mode.
 */
#define MCHP_XEC_AF0		0x0
#define MCHP_XEC_AF1		0x1
#define MCHP_XEC_AF2		0x2
#define MCHP_XEC_AF3		0x3
#define MCHP_XEC_AF4		0x4
#define MCHP_XEC_AF5		0x5
#define MCHP_XEC_AF6		0x6
#define MCHP_XEC_AF7		0x7

#define MCHP_XEC_FUNC_MAX	(MCHP_XEC_AF7 + 1)

/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is taken from Linux equivalent st,stm32f429-pinctrl binding
 * Microchip XEC documentation does not use letters for port naming instead
 * using pin number ranges such as 000_036, etc. Use the port defines from
 * mchp-xec-pinctrl-common: MCHP_XEC_PORT_000_036, etc.
 * mode is AF0,..,AF7
 * MCHP groups 32 pins per port requiring a 5-bit field for pin position.
 */
#define PIN_POS(pin)		((pin) % 32)
#define PIN_NO(port, line)	((port) * 0x20 + (line))

#define MCHP_XEC_PINMUX(port, pin, mode)				\
	(((PIN_NO(MCHP_XEC_PORT_ ## port, PIN_POS(pin))) << 8) |	\
	 (MCHP_XEC_ ## mode))

/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    Alternate Functions [ 0 : 3 ]
 *    GPIO Input          [ 4 ]
 *    GPIO Output         [ 5 ]
 *    GPIO Output type    [ 6 ]
 *    GPIO slew rate      [ 7 ]
 *    GPIO Drive strength [ 8 : 9 ]
 *    GPIO PUPD config    [ 10 : 11 ]
 *
 * Applicable to Microchip XEC series
 */

/* Alternate functions */
#define MCHP_XEC_FUNC_ALT_0	0
#define MCHP_XEC_FUNC_GPIO	(MCHP_XEC_FUNC_ALT_0)
#define MCHP_XEC_FUNC_ALT_1	1
#define MCHP_XEC_FUNC_ALT_2	2
#define MCHP_XEC_FUNC_ALT_3	3
#define MCHP_XEC_FUNC_ALT_4	4
#define MCHP_XEC_FUNC_ALT_5	5
#define MCHP_XEC_FUNC_ALT_6	6
#define MCHP_XEC_FUNC_ALT_7	7
#define MCHP_XEC_AFR_MASK	0x7
#define MCHP_XEC_AFR_SHIFT	0

/* GPIO Mode flags */
#define MCHP_XEC_MODER_INPUT_SHIFT	4
#define MCHP_XEC_MODER_INPUT		(1 << MCHP_XEC_MODER_INPUT_SHIFT)
#define MCHP_XEC_MODER_OUTPUT_SHIFT	5
#define MCHP_XEC_MODER_OUTPUT		(1 << MCHP_XEC_MODER_OUTPUT_SHIFT)

/* GPIO Output type */
#define MCHP_XEC_OTYPER_PUSH_PULL	(0x0 << MCHP_XEC_OTYPER_SHIFT)
#define MCHP_XEC_OTYPER_OPEN_DRAIN	(0x1 << MCHP_XEC_OTYPER_SHIFT)
#define MCHP_XEC_OTYPER_MASK		0x1
#define MCHP_XEC_OTYPER_SHIFT		6

/* GPIO slew rate */
#define MCHP_XEC_OSLEWR_SLOW		(0x0 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_OSLEWR_FAST		(0x1 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_OSLEWR_MASK		0x1
#define MCHP_XEC_OSLEWR_SHIFT		7

/* GPIO drive strength */
#define MCHP_XEC_ODSR_2MA		(0x0 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_ODSR_4MA		(0x1 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_ODSR_8MA		(0x2 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_ODSR_12MA		(0x3 << MCHP_XEC_OSLEWR_SHIFT)
#define MCHP_XEC_ODSR_MASK		0x3
#define MCHP_XEC_ODSR_SHIFT		8

/* GPIO High impedance/Pull-up/pull-down/repeater */
#define MCHP_XEC_PUPDR_NO_PULL		(0x0 << MCHP_XEC_PUPDR_SHIFT)
#define MCHP_XEC_PUPDR_PULL_UP		(0x1 << MCHP_XEC_PUPDR_SHIFT)
#define MCHP_XEC_PUPDR_PULL_DOWN	(0x2 << MCHP_XEC_PUPDR_SHIFT)
#define MCHP_XEC_PUPDR_REPEATER		(0x3 << MCHP_XEC_PUPDR_SHIFT)
#define MCHP_XEC_PUPDR_MASK		0x3
#define MCHP_XEC_PUPDR_SHIFT		10

/* Alternate functions definitions */
#define MCHP_XEC_PINMUX_GPIO_FUNC	(MCHP_XEC_FUNC_ALT_0)
#define MCHP_XEC_PINMUX_ALT_FUNC_0	(MCHP_XEC_FUNC_ALT_0)
#define MCHP_XEC_PINMUX_ALT_FUNC_1	(MCHP_XEC_FUNC_ALT_1)
#define MCHP_XEC_PINMUX_ALT_FUNC_2	(MCHP_XEC_FUNC_ALT_2)
#define MCHP_XEC_PINMUX_ALT_FUNC_3	(MCHP_XEC_FUNC_ALT_3)
#define MCHP_XEC_PINMUX_ALT_FUNC_4	(MCHP_XEC_FUNC_ALT_4)
#define MCHP_XEC_PINMUX_ALT_FUNC_5	(MCHP_XEC_FUNC_ALT_5)
#define MCHP_XEC_PINMUX_ALT_FUNC_6	(MCHP_XEC_FUNC_ALT_6)
#define MCHP_XEC_PINMUX_ALT_FUNC_7	(MCHP_XEC_FUNC_ALT_7)

/* Useful definitions */
#define MCHP_XEC_PUSHPULL_NOPULL	\
	(MCHP_XEC_OTYPER_PUSH_PULL | MCHP_XEC_PUPDR_NO_PULL)
#define MCHP_XEC_OPENDRAIN_PULLUP	\
	(MCHP_XEC_OTYPER_OPEN_DRAIN | MCHP_XEC_PUPDR_PULL_UP)
#define MCHP_XEC_PUSHPULL_PULLUP	\
	(MCHP_XEC_OTYPER_PUSH_PULL | MCHP_XEC_PUPDR_PULL_UP)
#define MCHP_XEC_PUSHPULL_REPEATER	\
	(MCHP_XEC_OTYPER_PUSH_PULL | MCHP_XEC_PUPDR_REPEATER)

/* default slew rate and drive strength */
#define MCHP_XEC_DFLT_SLEW_DRS	(MCHP_XEC_OSLEWR_SLOW | MCHP_XEC_ODSR_2MA)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MCHP_XEC_PINCTRL_H_ */
