/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E single-core M4
 * SPDX-License-Identifier: Apache-2.0
 *
 * LPC54S018J4MET180E pin control definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC54S018J4MET180E_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC54S018J4MET180E_PINCTRL_H_

#include <zephyr/dt-bindings/pinctrl/lpc-iocon-pinctrl.h>

/* 
 * Pin function encoding:
 * - [31:16] IOCON configuration (MODE, INVERT, DIGIMODE, etc.)
 * - [15:8]  Pin port number (0-4)
 * - [7:3]   Pin number (0-31)
 * - [2:0]   Pin function (0-7)
 *
 * The final value must be a hexadecimal number for device tree parsing
 */

/* Helper macros - IOCON digital pin with standard settings */
#define LPC54S018_IOCON_DIGITAL_STD	(0x0080)  /* Digital enabled, standard slew */
#define LPC54S018_IOCON_DIGITAL_FAST	(0x0280)  /* Digital enabled, fast slew */

/* Pin encoding macro */
#define LPC54S018_PINMUX(port, pin, func, iocon) \
	(((iocon) << 16) | ((port) << 8) | ((pin) << 3) | (func))

/* Port 0 pins */
#define PIO0_0_GPIO		0x00800000  /* P0.0 GPIO, digital */
#define PIO0_11_GPIO		0x00800058  /* P0.11 GPIO, digital */
#define SWCLK_PIO0_11		0x0080005E  /* P0.11 SWCLK, func 6 */
#define PIO0_12_GPIO		0x00800060  /* P0.12 GPIO, digital */
#define SWDIO_PIO0_12		0x00800066  /* P0.12 SWDIO, func 6 */
#define PIO0_13_GPIO		0x00800068  /* P0.13 GPIO, digital */
#define PIO0_14_GPIO		0x00800070  /* P0.14 GPIO, digital */

/* SPIFI pins - Port 0, Function 6 */
#define SPIFI_CSN_PIO0_23	0x008000BE  /* P0.23 SPIFI_CS, func 6 */
#define SPIFI_DATA0_PIO0_24	0x008000C6  /* P0.24 SPIFI_DATA0, func 6 */
#define SPIFI_DATA1_PIO0_25	0x008000CE  /* P0.25 SPIFI_DATA1, func 6 */
#define SPIFI_CLK_PIO0_26	0x008000D6  /* P0.26 SPIFI_CLK, func 6 */
#define SPIFI_DATA3_PIO0_27	0x008000DE  /* P0.27 SPIFI_DATA3, func 6 */
#define SPIFI_DATA2_PIO0_28	0x008000E6  /* P0.28 SPIFI_DATA2, func 6 */

/* FLEXCOMM0 UART pins - Port 0, Function 1 */
#define FC0_RXD_SDA_MOSI_PIO0_29	0x008000E9  /* P0.29 FC0_RXD, func 1 */
#define FC0_TXD_SCL_MISO_PIO0_30	0x008000F1  /* P0.30 FC0_TXD, func 1 */

/* Port 1 pins */
/* CAN0 - Port 1, Function 5 */
#define CAN0_TD_PIO1_2		0x00800115  /* P1.2 CAN0_TD, func 5 */
#define CAN0_RD_PIO1_3		0x0080011D  /* P1.3 CAN0_RD, func 5 */

/* CAN1 - Port 1, Function 5 */
#define CAN1_TD_PIO1_17		0x0080018D  /* P1.17 CAN1_TD, func 5 */
#define CAN1_RD_PIO1_18		0x00800195  /* P1.18 CAN1_RD, func 5 */

/* Port 2 pins */
/* FLEXCOMM1 SPI */
#define FC1_RXD_SDA_MOSI_PIO2_3	0x00800219  /* P2.3 FC1_MOSI, func 1 */
#define FC1_TXD_SCL_MISO_PIO2_4	0x00800221  /* P2.4 FC1_MISO, func 1 */
#define FC1_CTS_SDA_SSEL0_PIO2_5	0x00800229  /* P2.5 FC1_SSEL0, func 1 */

/* GPIO LED pins */
#define PIO2_26_GPIO		0x008002D0  /* P2.26 GPIO, digital */
#define PIO2_27_GPIO		0x008002D8  /* P2.27 GPIO, digital */

/* Port 3 pins */
/* FLEXCOMM1 SPI clock */
#define FC1_SCK_PIO3_11		0x0080035A  /* P3.11 FC1_SCK, func 2 */

/* FLEXCOMM9 SPI */
#define FC9_CTS_SDA_SSEL0_PIO3_13	0x00800369  /* P3.13 FC9_SSEL0, func 1 */
#define PIO3_14_GPIO		0x00800370  /* P3.14 GPIO, digital */
#define FC9_SCK_PIO3_20		0x008003A3  /* P3.20 FC9_SCK, func 3 */
#define FC9_RXD_SDA_MOSI_PIO3_21	0x008003AB  /* P3.21 FC9_MOSI, func 3 */
#define FC9_TXD_SCL_MISO_PIO3_22	0x008003B3  /* P3.22 FC9_MISO, func 3 */

/* FLEXCOMM4 UART */
#define FC4_RXD_SDA_MOSI_PIO3_26	0x008003D1  /* P3.26 FC4_RXD, func 1 */
#define FC4_TXD_SCL_MISO_PIO3_27	0x008003D9  /* P3.27 FC4_TXD, func 1 */

/* Port 4 pins */
/* FLEXCOMM6 UART */
#define FC6_RXD_SDA_MOSI_PIO4_2	0x00800412  /* P4.2 FC6_RXD, func 2 */
#define FC6_TXD_SCL_MISO_PIO4_3	0x0080041A  /* P4.3 FC6_TXD, func 2 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_LPC54S018J4MET180E_PINCTRL_H_ */