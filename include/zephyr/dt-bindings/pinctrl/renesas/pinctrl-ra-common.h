/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA_COMMON_H_

#define PORT4_POS  29
#define PORT4_MASK 0x1
#define PSEL_POS   24
#define PSEL_MASK  0x5
#define PORT_POS   21
#define PORT_MASK  0x7
#define PIN_POS    17
#define PIN_MASK   0xF
#define OPT_POS    0
#define OPT_MASK   0x1B000

#define RA_PINCFG_GPIO   0x00000
#define RA_PINCFG_FUNC   0x10000
#define RA_PINCFG_ANALOG 0x08000

#define RA_PINCFG(port, pin, psel, opt)                                                            \
	((((psel)&PSEL_MASK) << PSEL_POS) | (((pin)&PIN_MASK) << PIN_POS) |                        \
	 (((port)&PORT_MASK) << PORT_POS) | ((((port) >> 3) & PORT4_MASK) << PORT4_POS) |          \
	 (((opt)&OPT_MASK) << OPT_POS))

#if RA_SOC_PINS >= 40
#define RA_PINCFG__40(port, pin, psel, opt) RA_PINCFG(port, pin, psel, opt)
#endif

#if RA_SOC_PINS >= 48
#define RA_PINCFG__48(port, pin, psel, opt) RA_PINCFG(port, pin, psel, opt)
#endif

#if RA_SOC_PINS >= 64
#define RA_PINCFG__64(port, pin, psel, opt) RA_PINCFG(port, pin, psel, opt)
#endif

#if RA_SOC_PINS >= 100
#define RA_PINCFG_100(port, pin, psel, opt) RA_PINCFG(port, pin, psel, opt)
#endif

#endif
