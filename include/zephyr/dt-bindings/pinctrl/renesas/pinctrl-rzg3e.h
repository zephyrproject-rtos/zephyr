/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_

/* Superset list of all possible IO ports. */
#define PORT_00 0x0000 /* IO port 0 */
#define PORT_01 0x0100 /* IO port 1 */
#define PORT_02 0x0200 /* IO port 2 */
#define PORT_03 0x0300 /* IO port 3 */
#define PORT_04 0x0400 /* IO port 4 */
#define PORT_05 0x0500 /* IO port 5 */
#define PORT_06 0x0600 /* IO port 6 */
#define PORT_07 0x0700 /* IO port 7 */
#define PORT_08 0x0800 /* IO port 8 */
#define PORT_10 0x0A00 /* IO port 10 */
#define PORT_11 0x0B00 /* IO port 11 */
#define PORT_12 0x0C00 /* IO port 12 */
#define PORT_13 0x0D00 /* IO port 13 */
#define PORT_14 0x0E00 /* IO port 14 */
#define PORT_15 0x0F00 /* IO port 15 */
#define PORT_16 0x1000 /* IO port 16 */
#define PORT_17 0x1100 /* IO port 17 */
#define PORT_19 0x1300 /* IO port 19 */
#define PORT_20 0x1400 /* IO port 20 */
#define PORT_21 0x1500 /* IO port 21 */
#define PORT_22 0x1600 /* IO port 22 */
#define PORT_28 0x1C00 /* IO port 28 */

/*
 * Create the value contain port/pin/function information
 *
 * port: port number PORT_00..PORT_28
 * pin: pin number
 * func: pin function
 */
#define RZG_PINMUX(port, pin, func) (port | pin | (func << 4))

/* Special purpose port */
#define BSP_IO_WDTUDFCA 0xFFFF0500 /* WDTUDFCA */
#define BSP_IO_WDTUDFCM 0xFFFF0501 /* WDTUDFCM */

#define BSP_IO_SCIF_RXD 0xFFFF0600 /* SCIF_RXD */
#define BSP_IO_SCIF_TXD 0xFFFF0601 /* SCIF_TXD */

#define BSP_IO_SD0CLK  0xFFFF0900 /* SD0CLK */
#define BSP_IO_SD0CMD  0xFFFF0901 /* SD0CMD */
#define BSP_IO_SD0RSTN 0xFFFF0902 /* SD0RSTN */
#define BSP_IO_SD0PWEN 0xFFFF0903 /* SD0PWEN */
#define BSP_IO_SD0IOVS 0xFFFF0904 /* SD0IOVS */

#define BSP_IO_SD0DAT0 0xFFFF0A00 /* SD0DAT0 */
#define BSP_IO_SD0DAT1 0xFFFF0A01 /* SD0DAT1 */
#define BSP_IO_SD0DAT2 0xFFFF0A02 /* SD0DAT2 */
#define BSP_IO_SD0DAT3 0xFFFF0A03 /* SD0DAT3 */
#define BSP_IO_SD0DAT4 0xFFFF0A04 /* SD0DAT4 */
#define BSP_IO_SD0DAT5 0xFFFF0A05 /* SD0DAT5 */
#define BSP_IO_SD0DAT6 0xFFFF0A06 /* SD0DAT6 */
#define BSP_IO_SD0DAT7 0xFFFF0A07 /* SD0DAT7 */

/*FILNUM*/
#define RZG_FILNUM_4_STAGE  0
#define RZG_FILNUM_8_STAGE  1
#define RZG_FILNUM_12_STAGE 2
#define RZG_FILNUM_16_STAGE 3

/*FILCLKSEL*/
#define RZG_FILCLKSEL_NOT_DIV   0
#define RZG_FILCLKSEL_DIV_9000  1
#define RZG_FILCLKSEL_DIV_18000 2
#define RZG_FILCLKSEL_DIV_36000 3

#define RZG_FILTER_SET(filnum, filclksel) (((filnum) & 0x3) << 0x2) | (filclksel & 0x3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_ */
