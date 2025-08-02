/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA3M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA3M_H_

/* Superset list of all possible IO ports. */
#define PORT_00 0x0F00 /* IO port 0 */
#define PORT_01 0x1000 /* IO port 1 */
#define PORT_02 0x1100 /* IO port 2 */
#define PORT_03 0x1200 /* IO port 3 */
#define PORT_04 0x1300 /* IO port 4 */
#define PORT_05 0x1400 /* IO port 5 */
#define PORT_06 0x1500 /* IO port 6 */
#define PORT_07 0x1600 /* IO port 7 */
#define PORT_08 0x1700 /* IO port 8 */
#define PORT_09 0x1800 /* IO port 9 */
#define PORT_10 0x1900 /* IO port 10 */
#define PORT_11 0x1A00 /* IO port 11 */
#define PORT_12 0x1B00 /* IO port 12 */
#define PORT_20 0x0000 /* IO port 20 */
#define PORT_21 0x0100 /* IO port 21 */
#define PORT_22 0x0300 /* IO port 22 */
#define PORT_23 0x0400 /* IO port 23 */

/*
 * Create the value contain port/pin/function information
 *
 * port: port number BSP_IO_PORT_00..BSP_IO_PORT_18
 * pin: pin number
 * func: pin function
 */
#define RZA_PINMUX(port, pin, func) (port | pin | (func << 4))

/* Special purpose port */
#define BSP_IO_NMI 0xFFFF0001 /* NMI */

#define BSP_IO_TMS_SWDIO 0xFFFF0200 /* TMS_SWDIO */

#define BSP_IO_SD0_CLK 0xFFFF0400 /* SD0_CLK */

#define BSP_IO_QSPI0_SPCLK 0xFFFF0500 /* QSPI0_SPCLK */
#define BSP_IO_QSPI0_IO0   0xFFFF0501 /* QSPI0_IO0 */
#define BSP_IO_QSPI0_IO1   0xFFFF0502 /* QSPI0_IO1 */
#define BSP_IO_QSPI0_SSL   0xFFFF0505 /* QSPI0_SSL */

#define BSP_IO_WDTOVF_PERROUT 0xFFFF0600 /* WDTOVF_PERROUT */

#define BSP_IO_RIIC0_SDA 0xFFFF0700 /* RIIC0_SDA */
#define BSP_IO_RIIC0_SCL 0xFFFF0701 /* RIIC0_SCL */

/* FILNUM */
#define RZA_FILNUM_4_STAGE  0
#define RZA_FILNUM_8_STAGE  1
#define RZA_FILNUM_12_STAGE 2
#define RZA_FILNUM_16_STAGE 3

/* FILCLKSEL */
#define RZA_FILCLKSEL_NOT_DIV   0
#define RZA_FILCLKSEL_DIV_9000  1
#define RZA_FILCLKSEL_DIV_18000 2
#define RZA_FILCLKSEL_DIV_36000 3

#define RZA_FILTER_SET(filnum, filclksel) (((filnum) & 0x3) << 0x2) | (filclksel & 0x3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA3M_H_ */
