/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_XLNX_ZYNQ_CLKC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_XLNX_ZYNQ_CLKC_H_

/*
 * Clock IDs of the Xilinx Zynq-7000 clock controller (xlnx,ps7-clkc), the
 * output numbering of the Linux binding.
 */

#define ZYNQ_CLK_ARMPLL     0
#define ZYNQ_CLK_DDRPLL     1
#define ZYNQ_CLK_IOPLL      2
#define ZYNQ_CLK_CPU_6OR4X  3
#define ZYNQ_CLK_CPU_3OR2X  4
#define ZYNQ_CLK_CPU_2X     5
#define ZYNQ_CLK_CPU_1X     6
#define ZYNQ_CLK_DDR2X      7
#define ZYNQ_CLK_DDR3X      8
#define ZYNQ_CLK_DCI        9
#define ZYNQ_CLK_LQSPI      10
#define ZYNQ_CLK_SMC        11
#define ZYNQ_CLK_PCAP       12
#define ZYNQ_CLK_GEM0       13
#define ZYNQ_CLK_GEM1       14
#define ZYNQ_CLK_FCLK0      15
#define ZYNQ_CLK_FCLK1      16
#define ZYNQ_CLK_FCLK2      17
#define ZYNQ_CLK_FCLK3      18
#define ZYNQ_CLK_CAN0       19
#define ZYNQ_CLK_CAN1       20
#define ZYNQ_CLK_SDIO0      21
#define ZYNQ_CLK_SDIO1      22
#define ZYNQ_CLK_UART0      23
#define ZYNQ_CLK_UART1      24
#define ZYNQ_CLK_SPI0       25
#define ZYNQ_CLK_SPI1       26
#define ZYNQ_CLK_DMA        27
#define ZYNQ_CLK_USB0_APER  28
#define ZYNQ_CLK_USB1_APER  29
#define ZYNQ_CLK_GEM0_APER  30
#define ZYNQ_CLK_GEM1_APER  31
#define ZYNQ_CLK_SDIO0_APER 32
#define ZYNQ_CLK_SDIO1_APER 33
#define ZYNQ_CLK_SPI0_APER  34
#define ZYNQ_CLK_SPI1_APER  35
#define ZYNQ_CLK_CAN0_APER  36
#define ZYNQ_CLK_CAN1_APER  37
#define ZYNQ_CLK_I2C0_APER  38
#define ZYNQ_CLK_I2C1_APER  39
#define ZYNQ_CLK_UART0_APER 40
#define ZYNQ_CLK_UART1_APER 41
#define ZYNQ_CLK_GPIO_APER  42
#define ZYNQ_CLK_LQSPI_APER 43
#define ZYNQ_CLK_SMC_APER   44
#define ZYNQ_CLK_SWDT       45
#define ZYNQ_CLK_DBG_TRC    46
#define ZYNQ_CLK_DBG_APB    47

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_XLNX_ZYNQ_CLKC_H_ */
