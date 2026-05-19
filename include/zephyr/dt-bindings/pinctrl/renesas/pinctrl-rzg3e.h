/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RZ/G3E pin control (pinctrl) definitions for Zephyr.
 *
 * This header provides macro constants for encoding pin function selections
 * and pin indices for Renesas RZ/G3E SoCs. These values are used by the DeviceTree
 * pinctrl subsystem to describe peripheral pin mappings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_

/**
 * @name Renesas RZ/G3E I/O Ports.
 * @{
 */

#define PORT_00 0x0000 /**< IO port 0 */
#define PORT_01 0x0100 /**< IO port 1 */
#define PORT_02 0x0200 /**< IO port 2 */
#define PORT_03 0x0300 /**< IO port 3 */
#define PORT_04 0x0400 /**< IO port 4 */
#define PORT_05 0x0500 /**< IO port 5 */
#define PORT_06 0x0600 /**< IO port 6 */
#define PORT_07 0x0700 /**< IO port 7 */
#define PORT_08 0x0800 /**< IO port 8 */
#define PORT_10 0x0A00 /**< IO port 10 */
#define PORT_11 0x0B00 /**< IO port 11 */
#define PORT_12 0x0C00 /**< IO port 12 */
#define PORT_13 0x0D00 /**< IO port 13 */
#define PORT_14 0x0E00 /**< IO port 14 */
#define PORT_15 0x0F00 /**< IO port 15 */
#define PORT_16 0x1000 /**< IO port 16 */
#define PORT_17 0x1100 /**< IO port 17 */
#define PORT_19 0x1300 /**< IO port 19 */
#define PORT_20 0x1400 /**< IO port 20 */
#define PORT_21 0x1500 /**< IO port 21 */
#define PORT_22 0x1600 /**< IO port 22 */
#define PORT_28 0x1C00 /**< IO port 28 */

/** @} */

/**
 * @name  Renesas RZ/G3E Dedicated Pins
 * @{
 */

#define BSP_IO_WDTUDFCA 0xFFFF0500 /**< WDTUDFCA */
#define BSP_IO_WDTUDFCM 0xFFFF0501 /**< WDTUDFCM */

#define BSP_IO_SCIF_RXD 0xFFFF0600 /**< SCIF_RXD */
#define BSP_IO_SCIF_TXD 0xFFFF0601 /**< SCIF_TXD */

#define BSP_IO_SD0CLK  0xFFFF0900 /**< SD0CLK */
#define BSP_IO_SD0CMD  0xFFFF0901 /**< SD0CMD */
#define BSP_IO_SD0RSTN 0xFFFF0902 /**< SD0RSTN */
#define BSP_IO_SD0PWEN 0xFFFF0903 /**< SD0PWEN */
#define BSP_IO_SD0IOVS 0xFFFF0904 /**< SD0IOVS */

#define BSP_IO_SD0DAT0 0xFFFF0A00 /**< SD0DAT0 */
#define BSP_IO_SD0DAT1 0xFFFF0A01 /**< SD0DAT1 */
#define BSP_IO_SD0DAT2 0xFFFF0A02 /**< SD0DAT2 */
#define BSP_IO_SD0DAT3 0xFFFF0A03 /**< SD0DAT3 */
#define BSP_IO_SD0DAT4 0xFFFF0A04 /**< SD0DAT4 */
#define BSP_IO_SD0DAT5 0xFFFF0A05 /**< SD0DAT5 */
#define BSP_IO_SD0DAT6 0xFFFF0A06 /**< SD0DAT6 */
#define BSP_IO_SD0DAT7 0xFFFF0A07 /**< SD0DAT7 */

/** @} */

/**
 * @name  Renesas RZ/G3E Digital Noise Filter Options
 *
 * Set the number of stages (FILNUM) and the sampling internal (FILCLKSEL)
 * @{
 */

#define RZG_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZG_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZG_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZG_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

#define RZG_FILCLKSEL_DIV_4   0 /**< FILCLKSEL: divided by 4 */
#define RZG_FILCLKSEL_DIV_32  1 /**< FILCLKSEL: divided by 32 */
#define RZG_FILCLKSEL_DIV_128 2 /**< FILCLKSEL: divided by 128 */
#define RZG_FILCLKSEL_DIV_256 3 /**< FILCLKSEL: divided by 256 */

/** @} */

/**
 * @brief Create an encoded value containing port/pin/function information.
 *
 * @param port Port identifier (PORT_00..PORT_28).
 * @param pin Pin number within the port.
 * @param func Pin function selector.
 *
 * @return Encoded pinmux value.
 */
#define RZG_PINMUX(port, pin, func) (port | pin | ((func) << 4))

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZG_FILNUM_*).
 * @param filclksel Filter clock selection (RZG_FILCLKSEL_*).
 *
 * @return Encoded filter configuration value.
 */
#define RZG_FILTER_SET(filnum, filclksel) (((filnum) & 0x3) << 0x2) | (filclksel & 0x3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG3E_H_ */
