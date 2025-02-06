/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_

/* Superset list of all possible IO ports. */
#define PORT_00 0x0000 /* IO port 0 */
#define PORT_01 0x1000 /* IO port 1 */
#define PORT_02 0x1100 /* IO port 2 */
#define PORT_03 0x1200 /* IO port 3 */
#define PORT_04 0x1300 /* IO port 4 */
#define PORT_05 0x0100 /* IO port 5 */
#define PORT_06 0x0200 /* IO port 6 */
#define PORT_07 0x1400 /* IO port 7 */
#define PORT_08 0x1500 /* IO port 8 */
#define PORT_09 0x1600 /* IO port 9 */
#define PORT_10 0x1700 /* IO port 10 */
#define PORT_11 0x0300 /* IO port 11 */
#define PORT_12 0x0400 /* IO port 12 */
#define PORT_13 0x0500 /* IO port 13 */
#define PORT_14 0x0600 /* IO port 14 */
#define PORT_15 0x0700 /* IO port 15 */
#define PORT_16 0x0800 /* IO port 16 */
#define PORT_17 0x0900 /* IO port 17 */
#define PORT_18 0x0A00 /* IO port 18 */

/*
 * Create the value contain port/pin/function information
 *
 * port: port number BSP_IO_PORT_00..BSP_IO_PORT_18
 * pin: pin number
 * func: pin function
 */
#define RZG_PINMUX(port, pin, func) (port | pin | (func << 4))

/* Special purpose port */
#define BSP_IO_NMI 0xFFFF0000 /* NMI */

#define BSP_IO_TMS_SWDIO 0xFFFF0100 /* TMS_SWDIO */
#define BSP_IO_TDO       0xFFFF0101 /* TDO */

#define BSP_IO_AUDIO_CLK1 0xFFFF0200 /* AUDIO_CLK1 */
#define BSP_IO_AUDIO_CLK2 0xFFFF0201 /* AUDIO_CLK2 */

#define BSP_IO_XSPI_SPCLK   0xFFFF0400 /* XSPI_SPCLK */
#define BSP_IO_XSPI_RESET_N 0xFFFF0401 /* XSPI_RESET_N */
#define BSP_IO_XSPI_WP_N    0xFFFF0402 /* XSPI_WP_N */
#define BSP_IO_XSPI_DS      0xFFFF0403 /* XSPI_DS */
#define BSP_IO_XSPI_CS0_N   0xFFFF0404 /* XSPI_CS0_N */
#define BSP_IO_XSPI_CS1_N   0xFFFF0405 /* XSPI_CS1_N */

#define BSP_IO_XSPI_IO0 0xFFFF0500 /* XSPI_IO0 */
#define BSP_IO_XSPI_IO1 0xFFFF0501 /* XSPI_IO1 */
#define BSP_IO_XSPI_IO2 0xFFFF0502 /* XSPI_IO2 */
#define BSP_IO_XSPI_IO3 0xFFFF0503 /* XSPI_IO3 */
#define BSP_IO_XSPI_IO4 0xFFFF0504 /* XSPI_IO4 */
#define BSP_IO_XSPI_IO5 0xFFFF0505 /* XSPI_IO5 */
#define BSP_IO_XSPI_IO6 0xFFFF0506 /* XSPI_IO6 */
#define BSP_IO_XSPI_IO7 0xFFFF0507 /* XSPI_IO7 */

#define BSP_IO_WDTOVF_PERROUT 0xFFFF0600 /* WDTOVF_PERROUT */

#define BSP_IO_I3C_SDA 0xFFFF0900 /* I3C_SDA */
#define BSP_IO_I3C_SCL 0xFFFF0901 /* I3C_SCL */

#define BSP_IO_SD0_CLK   0xFFFF1000 /* CD0_CLK */
#define BSP_IO_SD0_CMD   0xFFFF1001 /* CD0_CMD */
#define BSP_IO_SD0_RST_N 0xFFFF1002 /* CD0_RST_N */

#define BSP_IO_SD0_DATA0 0xFFFF1100 /* SD0_DATA0 */
#define BSP_IO_SD0_DATA1 0xFFFF1101 /* SD0_DATA1 */
#define BSP_IO_SD0_DATA2 0xFFFF1102 /* SD0_DATA2 */
#define BSP_IO_SD0_DATA3 0xFFFF1103 /* SD0_DATA3 */
#define BSP_IO_SD0_DATA4 0xFFFF1104 /* SD0_DATA4 */
#define BSP_IO_SD0_DATA5 0xFFFF1105 /* SD0_DATA5 */
#define BSP_IO_SD0_DATA6 0xFFFF1106 /* SD0_DATA6 */
#define BSP_IO_SD0_DATA7 0xFFFF1107 /* SD0_DATA7 */

#define BSP_IO_SD1_CLK 0xFFFF1200 /* SD1_CLK */
#define BSP_IO_SD1_CMD 0xFFFF1201 /* SD1_CMD */

#define BSP_IO_SD1_DATA0 0xFFFF1300 /* SD1_DATA0 */
#define BSP_IO_SD1_DATA1 0xFFFF1301 /* SD1_DATA1 */
#define BSP_IO_SD1_DATA2 0xFFFF1302 /* SD1_DATA2 */
#define BSP_IO_SD1_DATA3 0xFFFF1303 /* SD1_DATA3 */

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_ */
