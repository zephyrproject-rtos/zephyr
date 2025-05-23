/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_

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
#define PORT_09 0x0900 /* IO port 9 */
#define PORT_10 0x0A00 /* IO port 10 */
#define PORT_11 0x0B00 /* IO port 11 */
#define PORT_12 0x0C00 /* IO port 12 */
#define PORT_13 0x0D00 /* IO port 13 */
#define PORT_14 0x0E00 /* IO port 14 */
#define PORT_15 0x0F00 /* IO port 15 */
#define PORT_16 0x1000 /* IO port 16 */
#define PORT_17 0x1100 /* IO port 17 */
#define PORT_18 0x1200 /* IO port 18 */

/*
 * Create the value contain port/pin/function information
 *
 * port: port number BSP_IO_PORT_00..BSP_IO_PORT_18
 * pin: pin number
 * func: pin function
 */
#define RZA_PINMUX(port, pin, func) (port | pin | (func << 4))

/* Special purpose port */
#define BSP_IO_NMI 0xFFFF0100 /* NMI */

#define BSP_IO_TMS_SWDIO 0xFFFF0200 /* TMS_SWDIO */

#define BSP_IO_TDO 0xFFFF0300 /* TDO */

#define BSP_IO_AUDIO_CLK1 0xFFFF0400 /* AUDIO_CLK1 */
#define BSP_IO_AUDIO_CLK2 0xFFFF0401 /* AUDIO_CLK2 */

#define BSP_IO_SD0_CLK   0xFFFF0600 /* CD0_CLK */
#define BSP_IO_SD0_CMD   0xFFFF0601 /* CD0_CMD */
#define BSP_IO_SD0_RST_N 0xFFFF0602 /* CD0_RST_N */

#define BSP_IO_SD0_DATA0 0xFFFF0700 /* SD0_DATA0 */
#define BSP_IO_SD0_DATA1 0xFFFF0701 /* SD0_DATA1 */
#define BSP_IO_SD0_DATA2 0xFFFF0702 /* SD0_DATA2 */
#define BSP_IO_SD0_DATA3 0xFFFF0703 /* SD0_DATA3 */
#define BSP_IO_SD0_DATA4 0xFFFF0704 /* SD0_DATA4 */
#define BSP_IO_SD0_DATA5 0xFFFF0705 /* SD0_DATA5 */
#define BSP_IO_SD0_DATA6 0xFFFF0706 /* SD0_DATA6 */
#define BSP_IO_SD0_DATA7 0xFFFF0707 /* SD0_DATA7 */

#define BSP_IO_SD1_CLK 0xFFFF0800 /* SD1_CLK */
#define BSP_IO_SD1_CMD 0xFFFF0801 /* SD1_CMD */

#define BSP_IO_SD1_DATA0 0xFFFF0900 /* SD1_DATA0 */
#define BSP_IO_SD1_DATA1 0xFFFF0901 /* SD1_DATA1 */
#define BSP_IO_SD1_DATA2 0xFFFF0902 /* SD1_DATA2 */
#define BSP_IO_SD1_DATA3 0xFFFF0903 /* SD1_DATA3 */

#define BSP_IO_QSPI0_SPCLK 0xFFFF0A00 /* QSPI0_SPCLK */
#define BSP_IO_QSPI0_IO0   0xFFFF0A01 /* QSPI0_IO0 */
#define BSP_IO_QSPI0_IO1   0xFFFF0A02 /* QSPI0_IO1 */
#define BSP_IO_QSPI0_IO2   0xFFFF0A03 /* QSPI0_IO2 */
#define BSP_IO_QSPI0_IO3   0xFFFF0A04 /* QSPI0_IO3 */
#define BSP_IO_QSPI0_SSL   0xFFFF0A05 /* QSPI0_SSL */

#define BSP_IO_OM_CS1_N 0xFFFF0B00 /* OM_CS1_N */
#define BSP_IO_OM_DQS   0xFFFF0B01 /* OM_DQS */
#define BSP_IO_OM_SIO4  0xFFFF0B02 /* OM_SIO4 */
#define BSP_IO_OM_SIO5  0xFFFF0B03 /* OM_SIO5 */
#define BSP_IO_OM_SIO6  0xFFFF0B04 /* OM_SIO6 */
#define BSP_IO_OM_SIO7  0xFFFF0B05 /* OM_SIO7 */

#define BSP_IO_QSPI_RESET_N 0xFFFF0C00 /* QSPI_RESET_N */
#define BSP_IO_QSPI_WP_N    0xFFFF0C01 /* QSPI_WP_N */

#define BSP_IO_WDTOVF_PERROUT_N 0xFFFF0D00 /* WDTOVF_PERROUT_N */

#define BSP_IO_RIIC0_SDA 0xFFFF0E00 /* RIIC0_SDA */
#define BSP_IO_RIIC0_SCL 0xFFFF0E01 /* RIIC0_SCL */
#define BSP_IO_RIIC1_SDA 0xFFFF0E02 /* RIIC1_SDA */
#define BSP_IO_RIIC1_SCL 0xFFFF0E03 /* RIIC1_SCL */

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_ */
