/*
 * Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ETHERNET_XLNX_GEM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ETHERNET_XLNX_GEM_H_

/* PHY auto-detection alias */
#define XLNX_GEM_PHY_AUTO_DETECT 0

/* MDC divider values */
#define XLNX_GEM_MDC_DIVIDER_8   0 /* LPD_LSBUS_CLK     <  20 MHz */
#define XLNX_GEM_MDC_DIVIDER_16  1 /* LPD_LSBUS_CLK  20 -  40 MHz */
#define XLNX_GEM_MDC_DIVIDER_32  2 /* LPD_LSBUS_CLK  40 -  80 MHz */
/*
 * According to the ZynqMP's gem.network_config register documentation,
 * divider /32 is to be used for a 100 MHz LPD LSBUS clock.
 */
#define XLNX_GEM_MDC_DIVIDER_48  3 /* LPD_LSBUS_CLK  80 - 120 MHz */

/* Link speed values */
#define XLNX_GEM_LINK_SPEED_10MBIT  1
#define XLNX_GEM_LINK_SPEED_100MBIT 2
#define XLNX_GEM_LINK_SPEED_1GBIT   3

/* AMBA AHB data bus width */
#define XLNX_GEM_AMBA_AHB_DBUS_WIDTH_32BIT  0
#define XLNX_GEM_AMBA_AHB_DBUS_WIDTH_64BIT  1
#define XLNX_GEM_AMBA_AHB_DBUS_WIDTH_128BIT 2

/* AMBA AHB burst length */
#define XLNX_GEM_AMBA_AHB_BURST_SINGLE 1
#define XLNX_GEM_AMBA_AHB_BURST_INCR4  4
#define XLNX_GEM_AMBA_AHB_BURST_INCR8  8
#define XLNX_GEM_AMBA_AHB_BURST_INCR16 16

/* Hardware RX buffer size */
#define XLNX_GEM_HW_RX_BUFFER_SIZE_1KB 0
#define XLNX_GEM_HW_RX_BUFFER_SIZE_2KB 1
#define XLNX_GEM_HW_RX_BUFFER_SIZE_4KB 2
#define XLNX_GEM_HW_RX_BUFFER_SIZE_8KB 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ETHERNET_XLNX_GEM_H_ */
