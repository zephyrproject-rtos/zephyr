/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RCAR_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RCAR_PINCTRL_H_

#define RCAR_PINMUX_IPSR(bank, cell, func) ((bank << 10) | (cell << 4) | (func))
#define RCAR_PINMUX_GPSR(bank, bit, val)   ((bank << 7)  | (bit << 1)  | val)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RCAR_PINCTRL_H_ */
