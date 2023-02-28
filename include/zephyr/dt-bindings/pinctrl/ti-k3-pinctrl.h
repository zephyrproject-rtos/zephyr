/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_K3_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_K3_PINCTRL_H_

#define PULLUDEN_SHIFT		16
#define PULLTYPESEL_SHIFT	17
#define RXACTIVE_SHIFT		18

#define PULL_DISABLE		(1 << PULLUDEN_SHIFT)
#define PULL_ENABLE		(0 << PULLUDEN_SHIFT)

#define PULL_UP			((1 << PULLTYPESEL_SHIFT) | PULL_ENABLE)
#define PULL_DOWN		((0 << PULLTYPESEL_SHIFT) | PULL_ENABLE)

#define INPUT_ENABLE		(1 << RXACTIVE_SHIFT)
#define INPUT_DISABLE		(0 << RXACTIVE_SHIFT)

/* Only the following macros are intended be used in DTS files */

#define PIN_OUTPUT		(INPUT_DISABLE | PULL_DISABLE)
#define PIN_OUTPUT_PULLUP	(INPUT_DISABLE | PULL_UP)
#define PIN_OUTPUT_PULLDOWN	(INPUT_DISABLE | PULL_DOWN)
#define PIN_INPUT		(INPUT_ENABLE | PULL_DISABLE)
#define PIN_INPUT_PULLUP	(INPUT_ENABLE | PULL_UP)
#define PIN_INPUT_PULLDOWN	(INPUT_ENABLE | PULL_DOWN)

#define MUX_MODE_0		0
#define MUX_MODE_1		1
#define MUX_MODE_2		2
#define MUX_MODE_3		3
#define MUX_MODE_4		4
#define MUX_MODE_5		5
#define MUX_MODE_6		6
#define MUX_MODE_7		7
#define MUX_MODE_8		8
#define MUX_MODE_9		9

#define K3_PINMUX(offset, value, mux_mode)	(((offset) & 0x1fff)) ((value) | (mux_mode))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_K3_PINCTRL_H_ */
