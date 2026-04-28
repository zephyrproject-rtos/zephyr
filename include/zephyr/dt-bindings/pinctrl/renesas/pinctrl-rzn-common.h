/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZN_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZN_COMMON_H_

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
#define PORT_19 0x1300 /* IO port 19 */
#define PORT_20 0x1400 /* IO port 20 */
#define PORT_21 0x1500 /* IO port 21 */
#define PORT_22 0x1600 /* IO port 22 */
#define PORT_23 0x1700 /* IO port 23 */
#define PORT_24 0x1800 /* IO port 24 */

/*
 * Create the value contain port/pin/function information
 *
 * port: port number BSP_IO_PORT_00..BSP_IO_PORT_34
 * pin: pin number
 * func: pin function
 */
#define RZN_PINMUX(port, pin, func) (port | pin | (func << 4))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZN_COMMON_H_ */
