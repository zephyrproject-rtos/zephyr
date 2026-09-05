/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__

#define RX_PORT_NUM_POS  0
#define RX_PORT_NUM_MASK 0xf

#define RX_PIN_NUM_POS  5
#define RX_PIN_NUM_MASK 0xf

#define RX_PSEL_HIZ_JTAG_SWD 0x0
#define RX_PSEL_SCI_0        0xA
#define RX_PSEL_SCI_2        0xA
#define RX_PSEL_SCI_4        0xA
#define RX_PSEL_SCI_6        0xA
#define RX_PSEL_SCI_8        0xA
#define RX_PSEL_SCI_10       0xA
#define RX_PSEL_SCI_12       0xA
#define RX_PSEL_SCI_1        0xD
#define RX_PSEL_SCI_3        0xD
#define RX_PSEL_SCI_5        0xD
#define RX_PSEL_SCI_7        0xD
#define RX_PSEL_SCI_9        0xD
#define RX_PSEL_SCI_11       0xD

#define RX_PSEL_POS  9
#define RX_PSEL_MASK 0x1f

#define RX_MODE_POS  16
#define RX_MODE_MASK 0x1

#define RX_PSEL(psel, port_num, pin_num)                                                           \
	(1 << RX_MODE_POS | psel << RX_PSEL_POS | port_num << RX_PORT_NUM_POS |                    \
	 pin_num << RX_PIN_NUM_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__ */
