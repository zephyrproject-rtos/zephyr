/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_

#define RZA2M_PIN_NUM_IN_PORT 8

/* Port names as labeled in the Hardware Manual */
#define PORT_00 0
#define PORT_01 1
#define PORT_02 2
#define PORT_03 3
#define PORT_04 4
#define PORT_05 5
#define PORT_06 6
#define PORT_07 7
#define PORT_08 8
#define PORT_09 9
#define PORT_A  10
#define PORT_B  11
#define PORT_C  12
#define PORT_D  13
#define PORT_E  14
#define PORT_F  15
#define PORT_G  16
#define PORT_H  17
/* No I */
#define PORT_J  18
#define PORT_K  19
#define PORT_L  20
#define PORT_M  21 /* Pins PM_0/1 are labeled JP_0/1 in HW manual */

#define PORT_CKIO 22
#define PORT_PPOC 23 /* Select between 1.8V and 3.3V for SPI and SD/MMC */

#define PIN_POSEL 0 /* Sets function for POSEL0 bits. 00, 01, 10 - 1.8v, 11 - 3.3v */
#define PIN_POC2  1 /* Sets function for SSD host 0, 0 - 1.8v 1 - 3.3v */
#define PIN_POC3  2 /* Sets function for SSD host 1, 0 - 1.8v 1 - 3.3v */

/*
 * Create the pin index from its bank and position numbers and store in
 * the upper 16 bits the alternate function identifier
 */
#define RZA2M_PINMUX(b, p, f) ((b) * RZA2M_PIN_NUM_IN_PORT + (p) | (f << 16))

#define CKIO_DRV RZA2M_PINMUX(PORT_CKIO, 0, 0)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA2M_H_ */
