/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RA_PINCTRL_H__
#define __ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RA_PINCTRL_H__

#define RA_PORT_NUM_POS  0
#define RA_PORT_NUM_MASK 0xf

#define RA_PIN_NUM_POS  4
#define RA_PIN_NUM_MASK 0xf

#define RA_PSEL_HIZ_JTAG_SWD 0x0
#define RA_PSEL_ADC          0x0
#define RA_PSEL_DAC          0x0
#define RA_PSEL_AGT          0x1
#define RA_PSEL_GPT0         0x2
#define RA_PSEL_GPT1         0x3
#define RA_PSEL_SCI_0        0x4
#define RA_PSEL_SCI_2        0x4
#define RA_PSEL_SCI_4        0x4
#define RA_PSEL_SCI_6        0x4
#define RA_PSEL_SCI_8        0x4
#define RA_PSEL_SCI_1        0x5
#define RA_PSEL_SCI_3        0x5
#define RA_PSEL_SCI_5        0x5
#define RA_PSEL_SCI_7        0x5
#define RA_PSEL_SCI_9        0x5
#define RA_PSEL_SPI          0x6
#define RA_PSEL_I2C          0x7
#define RA_PSEL_CLKOUT_RTC   0x9
#define RA_PSEL_CAC_ADC      0xa
#define RA_PSEL_CAC_DAC      0xa
#define RA_PSEL_BUS          0xb
#define RA_PSEL_CANFD        0x10
#define RA_PSEL_QSPI         0x11
#define RA_PSEL_SSIE         0x12
#define RA_PSEL_USBFS        0x13
#define RA_PSEL_USBHS        0x14
#define RA_PSEL_SDHI         0x15
#define RA_PSEL_ETH_MII      0x16
#define RA_PSEL_ETH_RMII     0x17
#define RA_PSEL_GLCDC        0x19
#define RA_PSEL_OSPI         0x1c

#define RA_PSEL_POS  8
#define RA_PSEL_MASK 0x1f

#define RA_MODE_POS  13
#define RA_MODE_MASK 0x1

#define RA_PSEL(psel, port_num, pin_num)                                                           \
	(1 << RA_MODE_POS | psel << RA_PSEL_POS | port_num << RA_PORT_NUM_POS |                    \
	 pin_num << RA_PIN_NUM_POS)

#endif /* __ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RA_PINCTRL_H__ */
