/*
 * Copyright (c) 2021-2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ETHERNET_XLNX_GEM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ETHERNET_XLNX_GEM_H_

/* PHY auto-detection alias */
#define XLNX_GEM_PHY_AUTO_DETECT 0

/*
 * MDC divider values
 *
 * According to the ZynqMP's gem.network_config register documentation (UG1087),
 * divider /32 is the reset value. The network_config[mdc_clock_division]
 * documentation in UG1087 is likely wrong (copied directly from the Zynq-7000),
 * as it claims that the MDC clock division is applied to the cpu_1x clock
 * which the UltraScale doesn't have. Contradicting information is provided in
 * the UltraScale TRM (UG1085), which mentions in chapter 34, section "Configure
 * the PHY", p. 1074, that the MDC clock division is applied to the IOU_SWITCH_CLK.
 * Xilinx's emacps driver doesn't (or no longer does) limit the range of dividers
 * on the UltraScale compared to the Zynq-7000.
 * -> Contrary to earlier revisions of this driver, all dividers are available
 *    to both the UltraScale and the Zynq-7000.
 */

#define XLNX_GEM_MDC_DIVIDER_8   0 /* cpu_1x or IOU_SWITCH_CLK     <  20 MHz */
#define XLNX_GEM_MDC_DIVIDER_16  1 /* cpu_1x or IOU_SWITCH_CLK  20 -  40 MHz */
#define XLNX_GEM_MDC_DIVIDER_32  2 /* cpu_1x or IOU_SWITCH_CLK  40 -  80 MHz */
#define XLNX_GEM_MDC_DIVIDER_48  3 /* cpu_1x or IOU_SWITCH_CLK  80 - 120 MHz */
#define XLNX_GEM_MDC_DIVIDER_64  4 /* cpu_1x or IOU_SWITCH_CLK 120 - 160 MHz */
#define XLNX_GEM_MDC_DIVIDER_96  5 /* cpu_1x or IOU_SWITCH_CLK 160 - 240 MHz */
#define XLNX_GEM_MDC_DIVIDER_128 6 /* cpu_1x or IOU_SWITCH_CLK 240 - 320 MHz */
#define XLNX_GEM_MDC_DIVIDER_224 7 /* cpu_1x or IOU_SWITCH_CLK 320 - 540 MHz */

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
